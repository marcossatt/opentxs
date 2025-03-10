// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "blockchain/node/wallet/subchain/statemachine/Job.hpp"  // IWYU pragma: associated

#include <boost/system/error_code.hpp>  // IWYU pragma: keep
#include <frozen/bits/algorithms.h>
#include <frozen/unordered_map.h>
#include <algorithm>
#include <chrono>
#include <iterator>
#include <string_view>
#include <utility>

#include "blockchain/node/wallet/subchain/SubchainStateData.hpp"
#include "internal/api/network/Asio.hpp"
#include "internal/blockchain/block/Parser.hpp"
#include "internal/blockchain/node/wallet/Reorg.hpp"
#include "internal/blockchain/node/wallet/subchain/statemachine/Types.hpp"
#include "internal/network/zeromq/Pipeline.hpp"
#include "internal/network/zeromq/socket/Pipeline.hpp"
#include "internal/network/zeromq/socket/Raw.hpp"
#include "internal/util/P0330.hpp"
#include "opentxs/WorkType.internal.hpp"
#include "opentxs/api/Network.hpp"
#include "opentxs/api/Session.hpp"
#include "opentxs/api/Session.internal.hpp"
#include "opentxs/api/network/Asio.hpp"
#include "opentxs/api/session/Crypto.hpp"
#include "opentxs/blockchain/block/Block.hpp"
#include "opentxs/blockchain/block/Position.hpp"
#include "opentxs/blockchain/block/Types.hpp"
#include "opentxs/blockchain/cfilter/Types.hpp"
#include "opentxs/blockchain/node/FilterOracle.hpp"
#include "opentxs/blockchain/node/Manager.hpp"
#include "opentxs/network/zeromq/message/Frame.hpp"
#include "opentxs/network/zeromq/message/Message.hpp"
#include "opentxs/network/zeromq/socket/Direction.hpp"   // IWYU pragma: keep
#include "opentxs/network/zeromq/socket/Policy.hpp"      // IWYU pragma: keep
#include "opentxs/network/zeromq/socket/SocketType.hpp"  // IWYU pragma: keep
#include "opentxs/util/Allocator.hpp"
#include "opentxs/util/Log.hpp"

namespace opentxs::blockchain::node::wallet
{
using namespace std::literals;

auto print(JobState in) noexcept -> std::string_view
{
    using enum JobState;
    static constexpr auto map =
        frozen::make_unordered_map<JobState, std::string_view>({
            {normal, "normal"sv},
            {reorg, "reorg"sv},
            {shutdown, "shutdown"sv},
        });

    if (const auto* i = map.find(in); map.end() != i) {

        return i->second;
    } else {
        LogAbort()(__FUNCTION__)(": invalid SubchainJobs: ")(
            static_cast<int>(in))
            .Abort();
    }
}

auto print(JobType in) noexcept -> std::string_view
{
    using enum JobType;
    static constexpr auto map =
        frozen::make_unordered_map<JobType, std::string_view>({
            {scan, "scan"sv},
            {process, "process"sv},
            {index, "index"sv},
            {rescan, "rescan"sv},
            {progress, "progress"sv},
        });

    if (const auto* i = map.find(in); map.end() != i) {

        return i->second;
    } else {
        LogAbort()(__FUNCTION__)(": invalid SubchainJobs: ")(
            static_cast<unsigned int>(in))
            .Abort();
    }
}
}  // namespace opentxs::blockchain::node::wallet

namespace opentxs::blockchain::node::wallet::statemachine
{
using enum opentxs::network::zeromq::socket::Direction;
using enum opentxs::network::zeromq::socket::Policy;
using enum opentxs::network::zeromq::socket::Type;

Job::Job(
    tag_t,
    const Log& logger,
    const std::shared_ptr<const SubchainStateData>& parent,
    const network::zeromq::BatchID batch,
    const JobType type,
    allocator_type alloc,
    Vector<network::zeromq::socket::EndpointRequest> subscribe,
    network::zeromq::socket::EndpointRequests pull,
    network::zeromq::socket::EndpointRequests dealer,
    Vector<network::zeromq::socket::SocketRequest> extra,
    Set<Work>&& neverDrop) noexcept
    : Actor(
          parent->api_,
          logger,
          [&] {
              using namespace std::literals;
              auto out = CString{alloc};
              out.append(print(type));
              out.append(" job for "sv);
              out.append(parent->name_);

              return out;
          }(),
          1ms,
          batch,
          alloc,
          {subscribe},
          std::move(pull),
          std::move(dealer),
          {extra},
          {},
          {},
          std::move(neverDrop))
    , parent_p_(parent)
    , api_p_(parent_p_->api_p_)
    , node_p_(parent_p_->node_p_)
    , api_(api_p_->Self())
    , node_(*node_p_)
    , parent_(*parent_p_)
    , reorg_(parent_.GetReorg().GetSlave(pipeline_, name_, alloc))
    , job_type_(type)
    , to_parent_(pipeline_.Internal().ExtraSocket(0))
    , pending_state_(State::normal)
    , state_(State::normal)
    , reorgs_(alloc)
    , watchdog_(api_.Network().Asio().Internal().GetTimer())
{
    assert_false(nullptr == parent_p_);
    assert_false(nullptr == api_p_);
    assert_false(nullptr == node_p_);
}

Job::Job(
    const Log& logger,
    const std::shared_ptr<const SubchainStateData>& parent,
    const network::zeromq::BatchID batch,
    const JobType type,
    allocator_type alloc,
    network::zeromq::socket::EndpointRequests subscribe,
    network::zeromq::socket::EndpointRequests pull,
    network::zeromq::socket::EndpointRequests dealer,
    network::zeromq::socket::SocketRequests extra,
    Set<Work>&& neverDrop) noexcept
    : Job(
          tag_t{},
          logger,
          parent,
          batch,
          type,
          alloc,
          [&] {
              const auto sub = subscribe.get();
              auto out =
                  Vector<network::zeromq::socket::EndpointRequest>{alloc};
              out.reserve(sub.size() + 2_uz);
              out.clear();
              out.emplace_back(parent->from_parent_, Connect);
              out.emplace_back(parent->from_ssd_endpoint_, Connect);
              std::ranges::copy(sub, std::back_inserter(out));

              return out;
          }(),
          pull,
          dealer,
          [&] {
              const auto ex = extra.get();
              auto out = Vector<network::zeromq::socket::SocketRequest>{alloc};
              out.reserve(ex.size() + 1_uz);
              out.clear();
              out.emplace_back(
                  Push,
                  Internal,
                  network::zeromq::socket::EndpointRequests{
                      {parent->to_ssd_endpoint_, Connect},
                  });
              std::ranges::copy(ex, std::back_inserter(out));

              return out;
          }(),
          std::move(neverDrop))
{
}

auto Job::add_last_reorg(Message& out) const noexcept -> void
{
    if (const auto epoc = last_reorg(); epoc.has_value()) {
        out.AddFrame(epoc.value());
    } else {
        out.AddFrame();
    }
}

auto Job::do_process_update(Message&& msg, allocator_type) noexcept -> void
{
    LogAbort()()(name_)(": unhandled message type").Abort();
}

auto Job::do_reorg(
    const node::HeaderOracle& oracle,
    const node::internal::HeaderOraclePrivate& data,
    Reorg::Params& params) noexcept -> bool
{
    return true;
}

auto Job::do_shutdown() noexcept -> void
{
    state_ = State::shutdown;
    reorg_.Stop();
    node_p_.reset();
    api_p_.reset();
    parent_p_.reset();
}

auto Job::do_startup(allocator_type monotonic) noexcept -> bool
{
    if (reorg_.Start()) { return true; }

    do_startup_internal(monotonic);

    return false;
}

auto Job::last_reorg() const noexcept -> std::optional<StateSequence>
{
    if (0_uz == reorgs_.size()) {

        return std::nullopt;
    } else {

        return *reorgs_.crbegin();
    }
}

auto Job::pipeline(
    const Work work,
    Message&& msg,
    allocator_type monotonic) noexcept -> void
{
    switch (state_) {
        case State::normal: {
            state_normal(work, std::move(msg), monotonic);
        } break;
        case State::reorg: {
            state_reorg(work, std::move(msg));
        } break;
        case State::pre_shutdown: {
            state_pre_shutdown(work, std::move(msg));
        } break;
        case State::shutdown: {
            // NOTE do nothing
        } break;
        default: {
            LogAbort()()(name_)(": invalid state").Abort();
        }
    }

    process_watchdog();
}

auto Job::process_block(Message&& in, allocator_type monotonic) noexcept -> void
{
    const auto body = in.Payload();
    const auto frames = body.size();

    assert_true(1_uz < frames);

    auto alloc = alloc::Strategy{get_allocator(), monotonic};
    auto blocks = Vector<block::Block>{alloc.work_};
    using block::Parser;
    const auto& crypto = api_.Crypto();

    if (false == Parser::Construct(
                     crypto, parent_.chain_, in, blocks, alloc.WorkOnly())) {
        LogAbort()()(name_)(": received invalid block(s) from block oracle")
            .Abort();
    }

    process_blocks(blocks, monotonic);
}

auto Job::process_blocks(std::span<block::Block>, allocator_type) noexcept
    -> void
{
    LogAbort()()(name_)(": unhandled message type").Abort();
}

auto Job::process_filter(Message&& in, allocator_type monotonic) noexcept
    -> void
{
    const auto body = in.Payload();

    assert_true(3_uz < body.size());

    const auto type = body[1].as<cfilter::Type>();

    if (type != node_.FilterOracle().DefaultType()) { return; }

    auto position =
        block::Position{body[2].as<block::Height>(), body[3].Bytes()};
    process_filter(std::move(in), std::move(position), monotonic);
}

auto Job::process_filter(Message&&, block::Position&&, allocator_type) noexcept
    -> void
{
    LogAbort()()(name_)(": unhandled message type").Abort();
}

auto Job::process_key(Message&& in, allocator_type) noexcept -> void
{
    LogAbort()()(name_)(": unhandled message type").Abort();
}

auto Job::process_prepare_reorg(Message&& in) noexcept -> void
{
    const auto body = in.Payload();

    assert_true(1u < body.size());

    transition_state_reorg(body[1].as<StateSequence>());
}

auto Job::process_process(Message&& in, allocator_type monotonic) noexcept
    -> void
{
    const auto body = in.Payload();

    assert_true(2_uz < body.size());

    process_process(
        block::Position{body[1].as<block::Height>(), body[2].Bytes()},
        monotonic);
}

auto Job::process_process(block::Position&&, allocator_type) noexcept -> void
{
    LogAbort()()(name_)(": unhandled message type").Abort();
}

auto Job::process_reprocess(Message&&, allocator_type) noexcept -> void
{
    LogAbort()()(name_)(": unhandled message type").Abort();
}

auto Job::process_start_scan(Message&&, allocator_type) noexcept -> void
{
    LogAbort()()(name_)(": unhandled message type").Abort();
}

auto Job::process_mempool(Message&&, allocator_type) noexcept -> void
{
    LogAbort()()(name_)(": unhandled message type").Abort();
}

auto Job::process_update(Message&& msg, allocator_type monotonic) noexcept
    -> void
{
    const auto body = msg.Payload();

    assert_true(1_uz < body.size());

    const auto& epoc = body[1];
    const auto expected = last_reorg();

    if (0_uz == epoc.size()) {
        if (expected.has_value()) {
            log_()(name_)(" ignoring stale update").Flush();

            return;
        }
    } else {
        if (expected.has_value()) {
            const auto reorg = epoc.as<StateSequence>();

            if (reorg != expected.value()) {
                log_()(name_)(" ignoring stale update").Flush();

                return;
            }
        } else {
            log_()(name_)(" ignoring stale update").Flush();

            return;
        }
    }

    do_process_update(std::move(msg), monotonic);
}

auto Job::process_watchdog() noexcept -> void
{
    to_parent_.SendDeferred([&] {
        auto out = MakeWork(Work::watchdog_ack);
        out.AddFrame(job_type_);

        return out;
    }());
    using namespace std::literals;
    reset_timer(10s, watchdog_, Work::watchdog);
}

auto Job::state_normal(
    const Work work,
    Message&& msg,
    allocator_type monotonic) noexcept -> void
{
    switch (work) {
        case Work::filter: {
            process_filter(std::move(msg), monotonic);
        } break;
        case Work::mempool: {
            process_mempool(std::move(msg), monotonic);
        } break;
        case Work::start_scan: {
            process_start_scan(std::move(msg), monotonic);
        } break;
        case Work::prepare_reorg: {
            process_prepare_reorg(std::move(msg));
        } break;
        case Work::update: {
            process_update(std::move(msg), monotonic);
        } break;
        case Work::process: {
            process_process(std::move(msg), monotonic);
        } break;
        case Work::rescan: {
            // NOTE ignore message
        } break;
        case Work::do_rescan: {
            process_do_rescan(std::move(msg));
        } break;
        case Work::watchdog: {
            process_watchdog();
        } break;
        case Work::reprocess: {
            process_reprocess(std::move(msg), monotonic);
        } break;
        case Work::block: {
            process_block(std::move(msg), monotonic);
        } break;
        case Work::key: {
            process_key(std::move(msg), monotonic);
        } break;
        case Work::prepare_shutdown: {
            transition_state_pre_shutdown();
        } break;
        case Work::finish_reorg: {
            LogAbort()()(name_)(" wrong state for ")(print(work))(" message")
                .Abort();
        }
        case Work::shutdown:
        case Work::watchdog_ack:
        case Work::init:
        case Work::statemachine: {
            unhandled_type(work);
        }
        default: {
            unknown_type(work);
        }
    }
}

auto Job::state_pre_shutdown(const Work work, Message&& msg) noexcept -> void
{
    switch (work) {
        case Work::filter:
        case Work::mempool:
        case Work::start_scan:
        case Work::update:
        case Work::process:
        case Work::rescan:
        case Work::do_rescan:
        case Work::watchdog:
        case Work::reprocess:
        case Work::block:
        case Work::key: {
            // NOTE ignore message
        } break;
        case Work::prepare_reorg:
        case Work::finish_reorg:
        case Work::prepare_shutdown: {
            LogAbort()()(name_)(" wrong state for ")(print(work))(" message")
                .Abort();
        }
        case Work::shutdown:
        case Work::watchdog_ack:
        case Work::init:
        case Work::statemachine: {
            unhandled_type(work);
        }
        default: {
            unknown_type(work);
        }
    }
}

auto Job::state_reorg(const Work work, Message&& msg) noexcept -> void
{
    switch (work) {
        case Work::filter:
        case Work::update: {
            // NOTE ignore message
        } break;
        case Work::mempool:
        case Work::start_scan:
        case Work::prepare_reorg:
        case Work::process:
        case Work::reprocess:
        case Work::rescan:
        case Work::do_rescan:
        case Work::block:
        case Work::key: {
            log_()(name_)(" deferring ")(print(work))(
                " message processing until reorg is complete")
                .Flush();
            defer(std::move(msg));
        } break;
        case Work::finish_reorg: {
            transition_state_normal();
        } break;
        case Work::prepare_shutdown: {
            LogAbort()()(name_)(" wrong state for ")(print(work))(" message")
                .Abort();
        }
        case Work::watchdog: {
            process_watchdog();
        } break;
        case Work::shutdown:
        case Work::watchdog_ack:
        case Work::init:
        case Work::statemachine: {
            unhandled_type(work);
        }
        default: {
            unknown_type(work);
        }
    }
}

auto Job::transition_state_normal() noexcept -> void
{
    state_ = State::normal;
    log_()(name_)(" transitioned to normal state ").Flush();
    trigger();
}

auto Job::transition_state_pre_shutdown() noexcept -> void
{
    watchdog_.Cancel();
    reorg_.AcknowledgeShutdown();
    state_ = State::pre_shutdown;
    log_()(name_)(": transitioned to pre_shutdown state").Flush();
}

auto Job::transition_state_reorg(StateSequence id) noexcept -> void
{
    assert_true(0_uz < id);

    if (0_uz == reorgs_.count(id)) {
        reorgs_.emplace(id);
        state_ = State::reorg;
        log_()(name_)(" ready to process reorg ")(id).Flush();
        reorg_.AcknowledgePrepareReorg(
            [this](const auto& header, const auto& lock, auto& params) {
                return do_reorg(header, lock, params);
            });
    } else {
        LogAbort()()(name_)(" reorg ")(id)(" already handled").Abort();
    }
}

auto Job::work(allocator_type monotonic) noexcept -> bool
{
    process_watchdog();

    return false;
}

Job::~Job() = default;
}  // namespace opentxs::blockchain::node::wallet::statemachine
