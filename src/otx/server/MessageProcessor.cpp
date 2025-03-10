// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "otx/server/MessageProcessor.hpp"  // IWYU pragma: associated

#include <opentxs/protobuf/OTXPush.pb.h>
#include <opentxs/protobuf/ServerReply.pb.h>
#include <opentxs/protobuf/ServerRequest.pb.h>
#include <chrono>
#include <memory>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

#include "internal/api/session/Endpoints.hpp"
#include "internal/core/Armored.hpp"
#include "internal/core/String.hpp"
#include "internal/network/zeromq/Batch.hpp"
#include "internal/network/zeromq/Context.hpp"
#include "internal/network/zeromq/ListenCallback.hpp"
#include "internal/network/zeromq/message/Message.hpp"  // IWYU pragma: keep
#include "internal/network/zeromq/socket/Raw.hpp"
#include "internal/otx/common/Message.hpp"
#include "internal/otx/server/Types.hpp"
#include "internal/util/Mutex.hpp"
#include "internal/util/Pimpl.hpp"
#include "internal/util/Size.hpp"
#include "internal/util/Thread.hpp"
#include "opentxs/Time.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Factory.internal.hpp"
#include "opentxs/api/Network.hpp"
#include "opentxs/api/Session.internal.hpp"
#include "opentxs/api/network/ZeroMQ.hpp"
#include "opentxs/api/session/Crypto.hpp"
#include "opentxs/api/session/Endpoints.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/api/session/Factory.internal.hpp"
#include "opentxs/api/session/Notary.hpp"
#include "opentxs/api/session/Notary.internal.hpp"
#include "opentxs/api/session/Wallet.hpp"
#include "opentxs/core/ByteArray.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/identifier/Nym.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/Types.hpp"
#include "opentxs/network/zeromq/message/Envelope.hpp"
#include "opentxs/network/zeromq/message/Frame.hpp"
#include "opentxs/network/zeromq/message/Message.hpp"
#include "opentxs/network/zeromq/socket/SocketType.hpp"  // IWYU pragma: keep
#include "opentxs/network/zeromq/socket/Types.hpp"
#include "opentxs/otx/Reply.hpp"
#include "opentxs/otx/Request.hpp"
#include "opentxs/otx/ServerReplyType.hpp"  // IWYU pragma: keep
#include "opentxs/otx/Types.hpp"
#include "opentxs/protobuf/Types.internal.tpp"
#include "opentxs/protobuf/syntax/ServerRequest.hpp"  // IWYU pragma: keep
#include "opentxs/protobuf/syntax/Types.internal.tpp"
#include "opentxs/util/Container.hpp"
#include "opentxs/util/Log.hpp"
#include "otx/server/Server.hpp"
#include "otx/server/UserCommandProcessor.hpp"

namespace opentxs::server
{
auto print(Jobs job) noexcept -> std::string_view
{
    try {
        using Job = Jobs;
        static const auto map = Map<Job, CString>{
            {Job::shutdown, "shutdown"},
            {Job::init, "init"},
            {Job::statemachine, "statemachine"},
        };

        return map.at(job);
    } catch (...) {
        LogError()(__FUNCTION__)("invalid Jobs: ")(
            static_cast<OTZMQWorkType>(job))
            .Flush();

        LogAbort()().Abort();
    }
}
}  // namespace opentxs::server

namespace opentxs::server
{
MessageProcessor::Imp::Imp(
    Server& server,
    const PasswordPrompt& reason) noexcept
    : api_(server.API())
    , server_(server)
    , reason_(reason)
    , running_(true)
    , zmq_handle_(api_.Network().ZeroMQ().Context().Internal().MakeBatch(
          {
              zmq::socket::Type::Router,  // NOTE frontend_
              zmq::socket::Type::Pull,    // NOTE notification_
          },
          "server::MessageProcessor"))
    , zmq_batch_(zmq_handle_.batch_)
    , frontend_([&]() -> auto& {
        auto& out = zmq_batch_.sockets_.at(0);
        const auto rc = out.SetExposedUntrusted();

        assert_true(rc);

        return out;
    }())
    , notification_(zmq_batch_.sockets_.at(1))
    , zmq_thread_(nullptr)
    , frontend_id_(frontend_.ID())
    , thread_()
    , counter_lock_()
    , drop_incoming_(0)
    , drop_outgoing_(0)
    , active_connections_()
    , connection_map_lock_()
{
    zmq_batch_.listen_callbacks_.emplace_back(zmq::ListenCallback::Factory(
        [this](auto&& m) { old_pipeline(std::move(m)); }));
    auto rc = notification_.Bind(
        api_.Endpoints().Internal().PushNotification().data());

    assert_true(rc);

    zmq_thread_ = api_.Network().ZeroMQ().Context().Internal().Start(
        zmq_batch_.id_,
        {
            {frontend_.ID(),
             &frontend_,
             [id = frontend_.ID(),
              &cb = zmq_batch_.listen_callbacks_.at(0).get()](auto&& m) {
                 m.Internal().Prepend(id);
                 cb.Process(std::move(m));
             }},
            {notification_.ID(),
             &notification_,
             [id = notification_.ID(),
              &cb = zmq_batch_.listen_callbacks_.at(0).get()](auto&& m) {
                 m.Internal().Prepend(id);
                 cb.Process(std::move(m));
             }},
        });

    assert_false(nullptr == zmq_thread_);

    LogTrace()()("using ZMQ batch ")(zmq_batch_.id_).Flush();
}

auto MessageProcessor::Imp::associate_connection(
    const bool oldFormat,
    const identifier::Nym& nym,
    const network::zeromq::Envelope& connection) noexcept -> void
{
    if (nym.empty()) { return; }
    if (false == connection.IsValid()) { return; }

    const auto changed = [&] {
        auto& map = active_connections_;
        auto lock = eLock{connection_map_lock_};
        auto output{false};

        if (auto it = map.find(nym); map.end() == it) {
            map.try_emplace(nym, connection, oldFormat);
            output = true;
        } else {
            auto& [id, format] = it->second;

            if (id != connection) {
                output = true;
                id = connection;
            }

            if (format != oldFormat) {
                output = true;
                format = oldFormat;
            }
        }

        return output;
    }();

    if (changed) {
        LogDetail()()("Nym ")(nym, api_.Crypto())(
            " is available via connection ")
            .asHex(connection.get()[0].Bytes())
            .Flush();
    }
}

auto MessageProcessor::Imp::cleanup() noexcept -> void
{
    running_ = false;
    zmq_handle_.Release();
}

auto MessageProcessor::Imp::DropIncoming(const int count) const noexcept -> void
{
    const auto lock = Lock{counter_lock_};
    drop_incoming_ = count;
}

auto MessageProcessor::Imp::DropOutgoing(const int count) const noexcept -> void
{
    const auto lock = Lock{counter_lock_};
    drop_outgoing_ = count;
}

auto MessageProcessor::Imp::extract_proto(
    const zmq::Frame& incoming) const noexcept -> protobuf::ServerRequest
{
    return protobuf::Factory<protobuf::ServerRequest>(incoming);
}

auto MessageProcessor::Imp::init(
    const bool inproc,
    const int port,
    const Secret& privkey) noexcept(false) -> void
{
    if (port == 0) { LogAbort()().Abort(); }

    api_.Network().ZeroMQ().Context().Internal().Modify(
        frontend_id_,
        [this, inproc, port, key = Secret{privkey}](auto& socket) {
            auto set = socket.SetPrivateKey(key.Bytes());

            assert_true(set);

            set = socket.SetZAPDomain(zap_domain_);

            assert_true(set);

            auto endpoint = std::stringstream{};

            if (inproc) {
                endpoint << api_.Internal().asNotary().InprocEndpoint();
                endpoint << ':';
            } else {
                endpoint << UnallocatedCString("tcp://*:");
            }

            endpoint << std::to_string(port);
            const auto bound = socket.Bind(endpoint.str().c_str());

            if (false == bound) {
                throw std::invalid_argument("Cannot bind to endpoint");
            }

            LogConsole()("Bound to endpoint: ")(endpoint.str()).Flush();
        });
}

auto MessageProcessor::Imp::old_pipeline(zmq::Message&& message) noexcept
    -> void
{
    const auto isFrontend = [&] {
        const auto header = message.Envelope();

        assert_true(header.IsValid());

        const auto socket = message.ExtractFront();
        const auto output = (frontend_id_ == socket.as<zmq::SocketID>());

        return output;
    }();

    if (isFrontend) {
        process_frontend(std::move(message));
    } else {
        process_notification(std::move(message));
    }
}

auto MessageProcessor::Imp::process_backend(
    const bool tagged,
    zmq::Message&& incoming) noexcept -> network::zeromq::Message
{
    auto reply = UnallocatedCString{};
    const auto error = [&] {
        // ProcessCron and process_backend must not run simultaneously
        auto lock = Lock{lock_};
        const auto request = [&] {
            auto out = UnallocatedCString{};
            const auto body = incoming.Payload();

            if (0u < body.size()) { out = body[0].Bytes(); }

            return out;
        }();

        return process_message(request, reply);
    }();

    if (error) { reply = ""; }

    auto output = network::zeromq::reply_to_message(std::move(incoming));

    if (tagged) { output.AddFrame(WorkType::OTXLegacyXML); }

    output.AddFrame(reply);

    return output;
}

auto MessageProcessor::Imp::process_command(
    const protobuf::ServerRequest& serialized,
    identifier::Nym& nymID) noexcept -> bool
{
    const auto allegedNymID = api_.Factory().Internal().NymID(serialized.nym());
    const auto nym = api_.Wallet().Nym(allegedNymID);

    if (false == bool(nym)) {
        LogError()()("Nym is not yet registered.").Flush();

        return true;
    }

    auto request = otx::Request::Factory(api_, serialized);

    if (request.Validate()) {
        nymID.Assign(request.Initiator());
    } else {
        LogError()()("Invalid request.").Flush();

        return false;
    }

    // TODO look at the request type and do some stuff

    return true;
}

auto MessageProcessor::Imp::process_frontend(zmq::Message&& message) noexcept
    -> void
{
    const auto drop = [&] {
        auto lock = Lock{counter_lock_};

        if (0 < drop_incoming_) {
            LogConsole()()("Dropping incoming message for testing.").Flush();
            --drop_incoming_;

            return true;
        } else {

            return false;
        }
    }();

    if (drop) { return; }

    const auto id = message.Envelope();
    const auto body = message.Payload();

    if (2u > body.size()) {
        process_legacy(id, false, std::move(message));

        return;
    }

    try {
        auto oldProtoFormat = false;
        const auto type = [&] {
            if ((2u == body.size()) && (0u == body[1].size())) {
                oldProtoFormat = true;

                return WorkType::OTXRequest;
            }

            try {

                return body[0].as<WorkType>();
            } catch (...) {
                throw std::runtime_error{"Invalid message type"};
            }
        }();

        switch (type) {
            case WorkType::OTXRequest: {
                process_proto(id, oldProtoFormat, std::move(message));
            } break;
            case WorkType::OTXLegacyXML: {
                process_legacy(id, true, std::move(message));
            } break;
            default: {
                throw std::runtime_error{"Unsupported message type"};
            }
        }
    } catch (const std::exception& e) {
        LogConsole()(e.what())(" from ").asHex(id.get()[0].Bytes()).Flush();
    }
}

auto MessageProcessor::Imp::process_internal(zmq::Message&& message) noexcept
    -> void
{
    const auto drop = [&] {
        auto lock = Lock{counter_lock_};

        if (0 < drop_outgoing_) {
            LogConsole()()("Dropping outgoing message for testing.").Flush();
            --drop_outgoing_;

            return true;
        } else {

            return false;
        }
    }();

    if (drop) { return; }

    const auto sent = frontend_.SendExternal(std::move(message));

    if (sent) {
        LogTrace()()("Reply message delivered.").Flush();
    } else {
        LogError()()("Failed to send reply message.").Flush();
    }
}

auto MessageProcessor::Imp::process_legacy(
    const network::zeromq::Envelope& id,
    const bool tagged,
    network::zeromq::Message&& incoming) noexcept -> void
{
    LogTrace()()("Processing request via ").asHex(id.get()[0].Bytes()).Flush();
    process_internal(process_backend(tagged, std::move(incoming)));
}

auto MessageProcessor::Imp::process_message(
    const UnallocatedCString& messageString,
    UnallocatedCString& reply) noexcept -> bool
{
    if (messageString.size() < 1) { return true; }

    try {
        auto armored = Armored::Factory(api_.Crypto());
        armored->MemSet(messageString.data(), shorten(messageString.size()));
        auto serialized = String::Factory();
        armored->GetString(serialized);
        auto request{api_.Factory().Internal().Session().Message()};

        if (false == serialized->Exists()) {
            LogError()()("Empty serialized request.").Flush();

            return true;
        }

        if (false == request->LoadContractFromString(serialized)) {
            LogError()()("Failed to deserialized request.").Flush();

            return true;
        }

        auto replymsg{api_.Factory().Internal().Session().Message()};

        assert_true(false != bool(replymsg));

        const bool processed =
            server_.CommandProcessor().ProcessUserCommand(*request, *replymsg);

        if (false == processed) {
            LogDetail()()("Failed to process user command ")(
                request->command_.get())
                .Flush();
            LogVerbose()()(String::Factory(*request).get()).Flush();
        } else {
            LogDetail()()("Successfully processed user command ")(
                request->command_.get())
                .Flush();
        }

        auto serializedReply = String::Factory(*replymsg);

        if (false == serializedReply->Exists()) {
            LogError()()("Failed to serialize reply.").Flush();

            return true;
        }

        auto armoredReply = Armored::Factory(api_.Crypto(), serializedReply);

        if (false == armoredReply->Exists()) {
            LogError()()("Failed to armor reply.").Flush();

            return true;
        }

        reply.assign(armoredReply->Get(), armoredReply->GetLength());

        return false;
    } catch (...) {

        return true;
    }
}

auto MessageProcessor::Imp::process_notification(
    zmq::Message&& incoming) noexcept -> void
{
    if (2 != incoming.Payload().size()) {
        LogError()()("Invalid message.").Flush();

        return;
    }

    const auto body = incoming.Payload();
    const auto nymID = api_.Factory().NymIDFromBase58(body[0].Bytes());
    const auto& data = query_connection(nymID);
    const auto& [connection, oldFormat] = data;

    if (false == connection.IsValid()) {
        LogDebug()()("No notification channel available for ")(
            nymID, api_.Crypto())(".")
            .Flush();

        return;
    }

    const auto nym = api_.Wallet().Nym(server_.GetServerNym().ID());

    assert_false(nullptr == nym);

    const auto& payload = body[1];
    auto message = otx::Reply::Factory(
        api_,
        nym,
        nymID,
        server_.GetServerID(),
        otx::ServerReplyType::Push,
        true,
        false,
        reason_,
        protobuf::DynamicFactory<protobuf::OTXPush>(payload));

    assert_true(message.Validate());

    auto serialized = protobuf::ServerReply{};

    if (false == message.Serialize(serialized)) {
        LogVerbose()()("Failed to serialize reply.").Flush();

        return;
    }

    const auto reply = api_.Factory().Internal().Data(serialized);
    const auto sent = frontend_.SendExternal([&] {
        auto out = reply_to_message(data.first, true);

        if (data.second) {
            out.AddFrame(reply);
            out.AddFrame();
        } else {
            out.AddFrame(WorkType::OTXPush);
            out.AddFrame(reply);
        }

        return out;
    }());

    if (sent) {
        LogVerbose()()("Push notification for ")(nymID, api_.Crypto())(
            " delivered via ")
            .asHex(connection.get()[0].Bytes())
            .Flush();
    } else {
        LogError()()("Failed to deliver push notifcation "
                     "for ")(nymID, api_.Crypto())(" via ")
            .asHex(connection.get()[0].Bytes())
            .Flush();
    }
}

auto MessageProcessor::Imp::process_proto(
    const network::zeromq::Envelope& id,
    const bool oldFormat,
    network::zeromq::Message&& incoming) noexcept -> void
{
    LogTrace()()("Processing request via ").asHex(id.get()[0].Bytes()).Flush();
    const auto body = incoming.Payload();
    const auto& payload = [&]() -> auto& {
        if (oldFormat) {

            return body[0];
        } else {

            return body[1];
        }
    }();
    const auto command = extract_proto(payload);

    if (false == protobuf::syntax::check(LogError(), command)) {
        LogError()()("Invalid otx request.").Flush();

        return;
    }

    auto nymID = identifier::Nym{};
    const auto valid = process_command(command, nymID);

    if (valid && (id.IsValid())) { associate_connection(oldFormat, nymID, id); }
}

auto MessageProcessor::Imp::query_connection(const identifier::Nym& id) noexcept
    -> const ConnectionData&
{
    auto lock = sLock{connection_map_lock_};

    try {

        return active_connections_.at(id);
    } catch (...) {
        static const auto blank = ConnectionData{{}, true};

        return blank;
    }
}

auto MessageProcessor::Imp::run() noexcept -> void
{
    SetThisThreadsName("MessageProcessor");

    while (running_.load()) {
        // timeout is the time left until the next cron should execute.
        const auto timeout = server_.ComputeTimeout();

        if (timeout.count() <= 0) {
            // ProcessCron and process_backend must not run simultaneously
            auto lock = Lock{lock_};
            server_.ProcessCron();
        }

        sleep(50ms);
    }
}

auto MessageProcessor::Imp::Start() noexcept -> void
{
    thread_ = std::thread(&Imp::run, this);
}

MessageProcessor::Imp::~Imp()
{
    cleanup();

    if (thread_.joinable()) { thread_.join(); }
}
}  // namespace opentxs::server

namespace opentxs::server
{
MessageProcessor::MessageProcessor(
    Server& server,
    const PasswordPrompt& reason) noexcept
    : imp_(std::make_unique<Imp>(server, reason).release())
{
    assert_true(imp_);
}

auto MessageProcessor::cleanup() noexcept -> void { imp_->cleanup(); }

auto MessageProcessor::DropIncoming(const int count) const noexcept -> void
{
    imp_->DropIncoming(count);
}

auto MessageProcessor::DropOutgoing(const int count) const noexcept -> void
{
    imp_->DropOutgoing(count);
}

auto MessageProcessor::init(
    const bool inproc,
    const int port,
    const Secret& privkey) noexcept(false) -> void
{
    imp_->init(inproc, port, privkey);
}

auto MessageProcessor::Start() noexcept -> void { imp_->Start(); }

MessageProcessor::~MessageProcessor()
{
    if (nullptr != imp_) {
        delete imp_;
        imp_ = nullptr;
    }
}
}  // namespace opentxs::server
