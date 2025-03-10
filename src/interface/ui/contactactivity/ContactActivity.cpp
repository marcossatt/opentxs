// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "interface/ui/contactactivity/ContactActivity.hpp"  // IWYU pragma: associated

#include <opentxs/protobuf/StorageThread.pb.h>
#include <opentxs/protobuf/StorageThreadItem.pb.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <span>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include "interface/ui/base/Widget.hpp"
#include "internal/api/session/Storage.hpp"
#include "internal/blockchain/Blockchain.hpp"
#include "internal/core/contract/Unit.hpp"
#include "internal/network/zeromq/Pipeline.hpp"
#include "internal/util/Mutex.hpp"
#include "opentxs/Time.hpp"
#include "opentxs/api/crypto/Blockchain.hpp"
#include "opentxs/api/session/Activity.hpp"
#include "opentxs/api/session/Client.hpp"
#include "opentxs/api/session/Contacts.hpp"
#include "opentxs/api/session/Crypto.hpp"
#include "opentxs/api/session/Endpoints.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/api/session/Storage.hpp"
#include "opentxs/api/session/Wallet.hpp"
#include "opentxs/api/session/Wallet.internal.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/block/Transaction.hpp"
#include "opentxs/blockchain/protocol/bitcoin/base/block/Transaction.hpp"
#include "opentxs/core/ByteArray.hpp"
#include "opentxs/core/Contact.hpp"
#include "opentxs/display/Definition.hpp"
#include "opentxs/identifier/Generic.hpp"
#include "opentxs/identifier/Nym.hpp"
#include "opentxs/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/network/zeromq/message/Frame.hpp"
#include "opentxs/otx/LastReplyStatus.hpp"  // IWYU pragma: keep
#include "opentxs/otx/Types.hpp"
#include "opentxs/otx/client/Messagability.hpp"  // IWYU pragma: keep
#include "opentxs/otx/client/Types.hpp"
#include "opentxs/protobuf/Types.internal.hpp"
#include "opentxs/util/Container.hpp"
#include "opentxs/util/Log.hpp"
#include "opentxs/util/PasswordPrompt.hpp"  // IWYU pragma: keep

// NOLINTBEGIN(cert-dcl58-cpp)
template class std::tuple<
    opentxs::identifier::Generic,
    opentxs::otx::client::StorageBox,
    opentxs::identifier::Generic>;
// NOLINTEND(cert-dcl58-cpp)

namespace opentxs::factory
{
auto ContactActivityModel(
    const api::session::Client& api,
    const identifier::Nym& nymID,
    const identifier::Generic& threadID,
    const SimpleCallback& cb) noexcept
    -> std::unique_ptr<ui::internal::ContactActivity>
{
    using ReturnType = ui::implementation::ContactActivity;

    return std::make_unique<ReturnType>(api, nymID, threadID, cb);
}
}  // namespace opentxs::factory

namespace opentxs::ui::implementation
{
using namespace std::literals;

ContactActivity::ContactActivity(
    const api::session::Client& api,
    const identifier::Nym& nymID,
    const identifier::Generic& threadID,
    const SimpleCallback& cb) noexcept
    : ContactActivityList(api, nymID, cb, false)
    , Worker(api, 100ms, "ui::ContactActivity")
    , thread_id_(threadID)
    , self_contact_(api.Contacts().NymToContact(primary_id_))
    , contacts_()
    , participants_()
    , me_(api.Contacts().ContactName(self_contact_))
    , display_name_()
    , payment_codes_()
    , can_message_(std::nullopt)
    , draft_()
    , draft_tasks_()
    , callbacks_()
{
    init_executor({
        api.Activity().ThreadPublisher(primary_id_),
        UnallocatedCString{api.Endpoints().ContactUpdate()},
        UnallocatedCString{api.Endpoints().Messagability()},
        UnallocatedCString{api.Endpoints().MessageLoaded()},
        UnallocatedCString{api.Endpoints().TaskComplete()},
    });
    pipeline_.Push(MakeWork(Work::init));
}

auto ContactActivity::calculate_display_name() const noexcept
    -> UnallocatedCString
{
    auto names = UnallocatedSet<UnallocatedCString>{};

    for (const auto& contactID : contacts_) {
        names.emplace(api_.Contacts().ContactName(contactID));
    }

    return comma(names);
}

auto ContactActivity::calculate_participants() const noexcept
    -> UnallocatedCString
{
    auto ids = UnallocatedSet<UnallocatedCString>{};

    for (const auto& id : contacts_) {
        ids.emplace(id.asBase58(api_.Crypto()));
    }

    return comma(ids);
}

auto ContactActivity::CanMessage() const noexcept -> bool
{
    return can_message();
}

auto ContactActivity::can_message() const noexcept -> bool
{
    wait_for_startup();
    auto lock = rLock{recursive_lock_};

    if (false == can_message_.has_value()) {
        trigger();

        return false;
    }

    const auto value = can_message_.value();

    return otx::client::Messagability::READY == value;
}

auto ContactActivity::ClearCallbacks() const noexcept -> void
{
    Widget::ClearCallbacks();
    auto lock = rLock{recursive_lock_};
    callbacks_ = std::nullopt;
}

auto ContactActivity::comma(const UnallocatedSet<UnallocatedCString>& list)
    const noexcept -> UnallocatedCString
{
    auto stream = std::ostringstream{};

    for (const auto& item : list) {
        stream << item;
        stream << ", ";
    }

    auto output = stream.str();

    if (0 < output.size()) { output.erase(output.size() - 2, 2); }

    return output;
}

auto ContactActivity::construct_row(
    const ContactActivityRowID& id,
    const ContactActivitySortKey& index,
    CustomData& custom) const noexcept -> RowPointer
{
    const auto& box = std::get<1>(id);

    switch (box) {
        case otx::client::StorageBox::MAILINBOX:
        case otx::client::StorageBox::MAILOUTBOX:
        case otx::client::StorageBox::DRAFT: {
            return factory::MailItem(
                *this, api_, primary_id_, id, index, custom);
        }
        case otx::client::StorageBox::INCOMINGCHEQUE:
        case otx::client::StorageBox::OUTGOINGCHEQUE: {
            return factory::PaymentItem(
                *this, api_, primary_id_, id, index, custom);
        }
        case otx::client::StorageBox::PENDING_SEND: {
            return factory::PendingSend(
                *this, api_, primary_id_, id, index, custom);
        }
        case otx::client::StorageBox::BLOCKCHAIN: {
            return factory::BlockchainContactActivityItem(
                *this, api_, primary_id_, id, index, custom);
        }
        case otx::client::StorageBox::SENTPEERREQUEST:
        case otx::client::StorageBox::INCOMINGPEERREQUEST:
        case otx::client::StorageBox::SENTPEERREPLY:
        case otx::client::StorageBox::INCOMINGPEERREPLY:
        case otx::client::StorageBox::FINISHEDPEERREQUEST:
        case otx::client::StorageBox::FINISHEDPEERREPLY:
        case otx::client::StorageBox::PROCESSEDPEERREQUEST:
        case otx::client::StorageBox::PROCESSEDPEERREPLY:
        case otx::client::StorageBox::RESERVED_1:
        case otx::client::StorageBox::OUTGOINGTRANSFER:
        case otx::client::StorageBox::INCOMINGTRANSFER:
        case otx::client::StorageBox::INTERNALTRANSFER:
        case otx::client::StorageBox::UNKNOWN:
        default: {
            LogAbort()().Abort();
        }
    }
}

auto ContactActivity::DisplayName() const noexcept -> UnallocatedCString
{
    wait_for_startup();
    auto lock = rLock{recursive_lock_};

    return display_name_;
}

auto ContactActivity::from(bool outgoing) const noexcept -> UnallocatedCString
{
    return outgoing ? me_ : display_name_;
}

auto ContactActivity::GetDraft() const noexcept -> UnallocatedCString
{
    auto lock = rLock{recursive_lock_};

    return draft_;
}

auto ContactActivity::load_contacts(
    const protobuf::StorageThread& thread) noexcept -> void
{
    auto& contacts =
        const_cast<UnallocatedSet<identifier::Generic>&>(contacts_);

    for (const auto& id : thread.participant()) {
        contacts.emplace(api_.Factory().IdentifierFromBase58(id));
    }
}

auto ContactActivity::load_thread(
    const protobuf::StorageThread& thread) noexcept -> void
{
    LogDetail()()("Loading ")(thread.item().size())(" items.").Flush();

    for (const auto& item : thread.item()) {
        try {
            process_item(item);
        } catch (...) {
            continue;
        }
    }
}

auto ContactActivity::new_thread() noexcept -> void
{
    auto& contacts =
        const_cast<UnallocatedSet<identifier::Generic>&>(contacts_);
    contacts.emplace(thread_id_);
}

auto ContactActivity::Participants() const noexcept -> UnallocatedCString
{
    wait_for_startup();
    auto lock = rLock{recursive_lock_};

    return participants_;
}

auto ContactActivity::Pay(
    const UnallocatedCString& amount,
    const identifier::Account& sourceAccount,
    const UnallocatedCString& memo,
    const otx::client::PaymentType type) const noexcept -> bool
{
    const auto& unitID =
        api_.Storage().Internal().AccountContract(sourceAccount);

    if (unitID.empty()) {
        LogError()()("Invalid account: (")(sourceAccount, api_.Crypto())(")")
            .Flush();

        return false;
    }

    try {
        const auto contract = api_.Wallet().Internal().UnitDefinition(unitID);
        const auto& definition =
            display::GetDefinition(contract->UnitOfAccount());

        try {
            if (auto value = definition.Import(amount); value) {

                return Pay(*value, sourceAccount, memo, type);
            } else {

                throw std::runtime_error{""};
            }
        } catch (...) {
            LogError()()("Error parsing amount (")(amount)(")").Flush();

            return false;
        }
    } catch (...) {
        LogError()()("Missing unit definition (")(unitID, api_.Crypto())(")")
            .Flush();

        return false;
    }
}

auto ContactActivity::Pay(
    const Amount amount,
    const identifier::Account& sourceAccount,
    const UnallocatedCString& memo,
    const otx::client::PaymentType type) const noexcept -> bool
{
    wait_for_startup();

    if (0 >= amount) {
        const auto contract = api_.Wallet().Internal().UnitDefinition(
            api_.Storage().Internal().AccountContract(sourceAccount));
        LogError()()("Invalid amount: (")(amount, contract->UnitOfAccount())(
            ")")
            .Flush();

        return false;
    }

    switch (type) {
        case otx::client::PaymentType::Cheque: {
            return send_cheque(amount, sourceAccount, memo);
        }
        case otx::client::PaymentType::Error:
        case otx::client::PaymentType::Voucher:
        case otx::client::PaymentType::Transfer:
        case otx::client::PaymentType::Blinded:
        default: {
            LogError()()("Unsupported payment type: (")(static_cast<int>(type))(
                ")")
                .Flush();

            return false;
        }
    }
}

auto ContactActivity::PaymentCode(const UnitType currency) const noexcept
    -> UnallocatedCString
{
    wait_for_startup();
    auto lock = rLock{recursive_lock_};

    try {

        return payment_codes_.at(currency);
    } catch (...) {

        return {};
    }
}

auto ContactActivity::pipeline(Message&& in) noexcept -> void
{
    if (false == running_.load()) { return; }

    const auto body = in.Payload();

    if (1 > body.size()) {
        LogError()()("Invalid message").Flush();

        LogAbort()().Abort();
    }

    const auto work = [&] {
        try {

            return body[0].as<Work>();
        } catch (...) {

            LogAbort()().Abort();
        }
    }();

    if ((false == startup_complete()) && (Work::init != work)) {
        pipeline_.Push(std::move(in));

        return;
    }

    switch (work) {
        case Work::shutdown: {
            if (auto previous = running_.exchange(false); previous) {
                shutdown(shutdown_promise_);
            }
        } break;
        case Work::contact: {
            process_contact(in);
        } break;
        case Work::thread: {
            process_thread(in);
        } break;
        case Work::message_loaded: {
            process_message_loaded(in);
        } break;
        case Work::otx: {
            process_otx(in);
        } break;
        case Work::messagability: {
            process_messagability(in);
        } break;
        case Work::init: {
            startup();
        } break;
        case Work::statemachine: {
            do_work();
        } break;
        default: {
            LogError()()("Unhandled type").Flush();

            LogAbort()().Abort();
        }
    }
}

auto ContactActivity::process_contact(const Message& in) noexcept -> void
{
    const auto body = in.Payload();

    assert_true(1 < body.size());

    const auto contactID =
        api_.Factory().IdentifierFromProtobuf(body[1].Bytes());
    auto changed{false};

    assert_false(contactID.empty());

    if (self_contact_ == contactID) {
        auto name = api_.Contacts().ContactName(self_contact_);

        if (auto lock = rLock{recursive_lock_}; me_ != name) {
            std::swap(me_, name);
            changed = true;
        }
    } else if (contacts_.contains(contactID)) {
        changed = update_display_name();
    } else {

        return;
    }

    if (changed) {
        refresh_thread();
        UpdateNotify();
    }

    trigger();
}

auto ContactActivity::process_item(
    const protobuf::StorageThreadItem& item) noexcept(false)
    -> ContactActivityRowID
{
    const auto id = ContactActivityRowID{
        api_.Factory().IdentifierFromBase58(item.id()),
        static_cast<otx::client::StorageBox>(item.box()),
        api_.Factory().AccountIDFromBase58(item.account())};
    const auto& [itemID, box, account] = id;
    const auto key =
        ContactActivitySortKey{std::chrono::seconds(item.time()), item.index()};
    auto custom = CustomData{
        new UnallocatedCString{},
        new UnallocatedCString{},
    };
    auto& sender = *static_cast<UnallocatedCString*>(custom.at(0));
    auto& text = *static_cast<UnallocatedCString*>(custom.at(1));
    auto& loading = *static_cast<bool*>(custom.emplace_back(new bool{false}));
    custom.emplace_back(new bool{false});
    auto& outgoing = *static_cast<bool*>(custom.emplace_back(new bool{false}));

    switch (box) {
        case otx::client::StorageBox::OUTGOINGCHEQUE:
        case otx::client::StorageBox::OUTGOINGTRANSFER:
        case otx::client::StorageBox::INTERNALTRANSFER:
        case otx::client::StorageBox::PENDING_SEND:
        case otx::client::StorageBox::DRAFT: {
            outgoing = true;
        } break;
        case otx::client::StorageBox::MAILOUTBOX: {
            outgoing = true;
            [[fallthrough]];
        }
        case otx::client::StorageBox::MAILINBOX: {
            auto reason = api_.Factory().PasswordPrompt("Decrypting messages");
            auto message =
                api_.Activity().MailText(primary_id_, itemID, box, reason);
            static constexpr auto none = 0ms;
            using Status = std::future_status;

            if (const auto s = message.wait_for(none); Status::ready == s) {
                text = message.get();
                loading = false;
            } else {
                text = "Loading";
                loading = true;
            }
        } break;
        case otx::client::StorageBox::BLOCKCHAIN: {
            const auto txid = api_.Factory().DataFromBytes(item.txid());
            const auto tx = api_.Crypto()
                                .Blockchain()
                                .LoadTransaction(txid.asHex())
                                .asBitcoin();
            const auto chain = static_cast<blockchain::Type>(item.chain());

            if (false == tx.IsValid()) {
                throw std::runtime_error{"transaction not found"};
            }

            text = api_.Crypto().Blockchain().ActivityDescription(
                primary_id_, chain, tx);
            const auto amount =
                tx.NetBalanceChange(api_.Crypto().Blockchain(), primary_id_);
            custom.emplace_back(new UnallocatedCString{item.txid()});
            custom.emplace_back(new opentxs::Amount{amount});
            custom.emplace_back(new UnallocatedCString{
                blockchain::internal::Format(chain, amount)});
            custom.emplace_back(
                new UnallocatedCString{tx.Memo(api_.Crypto().Blockchain())});

            outgoing = (0 > amount);
        } break;
        default: {
        }
    }

    sender = from(outgoing);
    add_item(id, key, custom);

    assert_true(verify_empty(custom));

    return id;
}

auto ContactActivity::process_messagability(const Message& message) noexcept
    -> void
{
    const auto body = message.Payload();

    assert_true(3 < body.size());

    const auto nym = api_.Factory().NymIDFromHash(body[1].Bytes());

    if (nym != primary_id_) { return; }

    const auto contact = api_.Factory().IdentifierFromHash(body[2].Bytes());

    if (false == contacts_.contains(contact)) { return; }

    if (update_messagability(body[3].as<otx::client::Messagability>())) {
        UpdateNotify();
    }
}

auto ContactActivity::process_message_loaded(const Message& message) noexcept
    -> void
{
    const auto body = message.Payload();

    assert_true(4 < body.size());

    const auto id = ContactActivityRowID{
        api_.Factory().IdentifierFromHash(body[2].Bytes()),
        body[3].as<otx::client::StorageBox>(),
        identifier::Account{}};
    const auto& [itemID, box, account] = id;

    if (const auto index = find_index(id); !index.has_value()) { return; }

    const auto key = [&] {
        auto lock = rLock{recursive_lock_};

        return sort_key(lock, id);
    }();
    const auto outgoing{box == otx::client::StorageBox::MAILOUTBOX};
    auto custom = CustomData{
        new UnallocatedCString{from(outgoing)},
        new UnallocatedCString{body[4].Bytes()},
        new bool{false},
        new bool{false},
        new bool{outgoing},
    };
    add_item(id, key, custom);

    assert_true(verify_empty(custom));
}

auto ContactActivity::process_otx(const Message& in) noexcept -> void
{
    const auto body = in.Payload();

    assert_true(2 < body.size());

    const auto id = body[1].as<api::session::OTX::TaskID>();
    auto done = [&] {
        auto output = std::optional<ContactActivityRowID>{};
        auto lock = rLock{recursive_lock_};
        auto it = draft_tasks_.find(id);

        if (draft_tasks_.end() == it) { return output; }

        auto& [taskID, task] = *it;
        auto& [rowID, backgroundTask] = task;
        auto& future = std::get<1>(backgroundTask);
        const auto [status, reply] = future.get();

        if (otx::LastReplyStatus::MessageSuccess == status) {
            LogDebug()()("Task ")(taskID)(" completed successfully").Flush();
        } else {
            // TODO consider taking some action in response to failed sends.
        }

        output = rowID;
        draft_tasks_.erase(it);

        return output;
    }();

    if (done.has_value()) {
        auto lock = rLock{recursive_lock_};
        delete_item(lock, done.value());
    }
}

auto ContactActivity::process_thread(const Message& message) noexcept -> void
{
    const auto body = message.Payload();

    assert_true(1 < body.size());

    const auto threadID = api_.Factory().IdentifierFromHash(body[1].Bytes());

    assert_false(threadID.empty());

    if (thread_id_ != threadID) { return; }

    refresh_thread();
}

auto ContactActivity::refresh_thread() noexcept -> void
{
    auto thread = protobuf::StorageThread{};
    auto loaded = api_.Activity().Thread(primary_id_, thread_id_, thread);

    if (false == loaded) { return; }

    auto active = UnallocatedSet<ContactActivityRowID>{};

    for (const auto& item : thread.item()) {
        try {
            const auto itemID = process_item(item);
            active.emplace(itemID);
        } catch (...) {

            continue;
        }
    }

    const auto drafts = [&] {
        auto lock = rLock{recursive_lock_};

        return draft_tasks_;
    }();

    for (const auto& [taskID, job] : drafts) {
        const auto& [rowid, future] = job;
        active.emplace(rowid);
    }

    delete_inactive(active);
}

auto ContactActivity::send_cheque(
    const Amount amount,
    const identifier::Account& sourceAccount,
    const UnallocatedCString& memo) const noexcept -> bool
{
    if (false == validate_account(sourceAccount)) { return false; }

    if (1 < contacts_.size()) {
        LogError()()("Sending to multiple recipient not yet supported.")
            .Flush();

        return false;
    }

    auto displayAmount = UnallocatedCString{};

    try {
        const auto contract = api_.Wallet().Internal().UnitDefinition(
            api_.Storage().Internal().AccountContract(sourceAccount));
        const auto& definition =
            display::GetDefinition(contract->UnitOfAccount());
        displayAmount = definition.Format(amount);
    } catch (...) {
        LogError()()("Failed to load unit definition contract").Flush();

        return false;
    }

    auto task = make_blank<DraftTask>::value(api_);
    auto& [id, otx] = task;
    otx = api_.OTX().SendCheque(
        primary_id_, sourceAccount, *contacts_.cbegin(), amount, memo);
    const auto taskID = std::get<0>(otx);

    if (0 == taskID) {
        LogError()()("Failed to queue payment for sending.").Flush();

        return false;
    }

    id = ContactActivityRowID{
        api_.Factory().IdentifierFromRandom(),
        otx::client::StorageBox::PENDING_SEND,
        {}};
    const auto key = ContactActivitySortKey{Clock::now(), 0};
    static constexpr auto outgoing{true};
    auto custom = CustomData{
        new UnallocatedCString{from(outgoing)},
        new UnallocatedCString{"Sending cheque"},
        new bool{false},
        new bool{true},
        new bool{outgoing},
        new Amount{amount},
        new UnallocatedCString{displayAmount},
        new UnallocatedCString{memo}};
    {
        auto lock = rLock{recursive_lock_};
        const_cast<ContactActivity&>(*this).add_item(id, key, custom);
        draft_tasks_.try_emplace(taskID, std::move(task));
    }

    assert_true(verify_empty(custom));

    return true;
}

auto ContactActivity::SendDraft() const noexcept -> bool
{
    wait_for_startup();
    {
        auto lock = rLock{recursive_lock_};

        if (draft_.empty()) {
            LogDetail()()("No draft message to send.").Flush();

            return false;
        }

        auto task = make_blank<DraftTask>::value(api_);
        auto& [id, otx] = task;
        otx =
            api_.OTX().MessageContact(primary_id_, *contacts_.begin(), draft_);
        const auto taskID = std::get<0>(otx);

        if (0 == taskID) {
            LogError()()("Failed to queue message for sending.").Flush();

            return false;
        }

        id = ContactActivityRowID{
            api_.Factory().IdentifierFromRandom(),
            otx::client::StorageBox::DRAFT,
            {}};
        const ContactActivitySortKey key{Clock::now(), 0};
        static constexpr auto outgoing{true};
        auto custom = CustomData{
            new UnallocatedCString{from(outgoing)},
            new UnallocatedCString{draft_},
            new bool{false},
            new bool{true},
            new bool{outgoing},
        };
        const_cast<ContactActivity&>(*this).add_item(id, key, custom);
        draft_tasks_.try_emplace(taskID, std::move(task));
        draft_.clear();

        assert_true(verify_empty(custom));
    }

    return true;
}

auto ContactActivity::SendFaucetRequest(const UnitType currency) const noexcept
    -> bool
{
    try {
        const auto contact = [&, this] {
            auto out = api_.Contacts().Contact(thread_id_);

            if (out) { return out; }

            throw std::runtime_error{
                "unable to load contact "s +
                thread_id_.asBase58(api_.Crypto())};
        }();
        const auto nym = [&, this] {
            for (const auto& id : contact->Nyms()) {
                auto out = api_.Wallet().Nym(id);

                if (out) { return out; }
            }

            throw std::runtime_error{
                "unable to load any nym for contact "s +
                thread_id_.asBase58(api_.Crypto())};
        }();
        const auto [task, _] =
            api_.OTX().InitiateFaucet(primary_id_, nym->ID(), currency, "");

        return 0 != task;
    } catch (const std::exception& e) {
        LogError()()(e.what()).Flush();

        return false;
    }
}

auto ContactActivity::SetCallbacks(Callbacks&& cb) noexcept -> void
{
    auto lock = rLock{recursive_lock_};
    callbacks_ = std::move(cb);

    if (callbacks_ && callbacks_.value().general_) {
        SetCallback(callbacks_.value().general_);
    }
}

auto ContactActivity::SetDraft(const UnallocatedCString& draft) const noexcept
    -> bool
{
    if (draft.empty()) { return false; }

    wait_for_startup();
    auto lock = rLock{recursive_lock_};

    if (false == can_message()) { return false; }

    draft_ = draft;

    if (callbacks_ && callbacks_.value().draft_) {
        callbacks_.value().draft_();
    }

    return true;
}

auto ContactActivity::set_participants() noexcept -> void
{
    auto& participants = const_cast<UnallocatedCString&>(participants_);
    participants = calculate_participants();
}

auto ContactActivity::startup() noexcept -> void
{
    auto thread = protobuf::StorageThread{};
    auto loaded = api_.Activity().Thread(primary_id_, thread_id_, thread);

    if (loaded) {
        load_contacts(thread);
    } else {
        new_thread();
    }

    set_participants();
    auto changed = update_display_name();
    changed |= update_payment_codes();

    if (loaded) { load_thread(thread); }

    if (changed) { UpdateNotify(); }

    finish_startup();
    trigger();
}

auto ContactActivity::state_machine() noexcept -> bool
{
    auto again{false};

    for (const auto& id : contacts_) {
        const auto value = api_.OTX().CanMessage(primary_id_, id, false);

        switch (value) {
            case otx::client::Messagability::READY: {

                return false;
            }
            case otx::client::Messagability::UNREGISTERED: {

                again |= true;
            } break;
            default: {
            }
        }
    }

    return again;
}

auto ContactActivity::ThreadID() const noexcept -> UnallocatedCString
{
    return thread_id_.asBase58(api_.Crypto());
}

auto ContactActivity::update_display_name() noexcept -> bool
{
    auto name = calculate_display_name();
    auto changed{false};

    {
        auto lock = rLock{recursive_lock_};
        changed = (display_name_ != name);
        display_name_.swap(name);

        if (changed && callbacks_ && callbacks_.value().display_name_) {
            callbacks_.value().display_name_();
        }
    }

    return changed;
}

auto ContactActivity::update_messagability(
    otx::client::Messagability value) noexcept -> bool
{
    const auto changed = [&] {
        auto lock = rLock{recursive_lock_};
        auto output = !can_message_.has_value();
        const auto old = can_message_.value_or(value);
        can_message_ = value;
        output |= (old != value);

        return output;
    }();

    {
        auto lock = rLock{recursive_lock_};

        if (changed && callbacks_ && callbacks_.value().messagability_) {
            callbacks_.value().messagability_(
                otx::client::Messagability::READY == can_message_.value());
        }
    }

    return changed;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-pointer="  // NOLINT
auto ContactActivity::update_payment_codes() noexcept -> bool
{
    auto map = UnallocatedMap<UnitType, UnallocatedCString>{};

    if (1 != contacts_.size()) { LogAbort()().Abort(); }

    const auto contact = api_.Contacts().Contact(*contacts_.cbegin());

    assert_false(nullptr == contact);

    for (const auto chain : blockchain::defined_chains()) {
        auto type = blockchain_to_unit(chain);
        auto code = contact->PaymentCode(type);

        if (code.empty()) { continue; }

        map.try_emplace(type, std::move(code));
    }

    auto changed{false};
    {
        auto lock = rLock{recursive_lock_};
        changed = (payment_codes_ != map);
        payment_codes_.swap(map);
    }

    return changed;
}
#pragma GCC diagnostic pop

auto ContactActivity::validate_account(
    const identifier::Account& sourceAccount) const noexcept -> bool
{
    const auto owner = api_.Storage().Internal().AccountOwner(sourceAccount);

    if (owner.empty()) {
        LogError()()("Invalid account id: (")(sourceAccount, api_.Crypto())(")")
            .Flush();

        return false;
    }

    if (primary_id_ != owner) {
        LogError()()("Account ")(sourceAccount, api_.Crypto())(
            " is not owned by nym ")(primary_id_, api_.Crypto())
            .Flush();

        return false;
    }

    return true;
}

ContactActivity::~ContactActivity()
{
    wait_for_startup();
    ClearCallbacks();
    signal_shutdown().get();
}
}  // namespace opentxs::ui::implementation
