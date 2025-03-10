// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_include <boost/unordered/detail/foa/flat_map_types.hpp>
// IWYU pragma: no_include <boost/unordered/detail/foa/table.hpp>

#include "opentxs/Types.hpp"  // IWYU pragma: associated

#include <boost/json.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <cs_plain_guarded.h>
#include <frozen/bits/algorithms.h>
#include <frozen/unordered_map.h>
#include <exception>
#include <functional>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>

#include "opentxs/AccountType.hpp"        // IWYU pragma: keep
#include "opentxs/AddressType.hpp"        // IWYU pragma: keep
#include "opentxs/BlockchainProfile.hpp"  // IWYU pragma: keep
#include "opentxs/WorkType.hpp"           // IWYU pragma: keep
#include "opentxs/WorkType.internal.hpp"
#include "opentxs/identity/wot/claim/Types.hpp"
#include "opentxs/identity/wot/claim/Types.internal.hpp"
#include "opentxs/protobuf/syntax/VerifyContacts.hpp"
#include "opentxs/util/Container.hpp"
#include "opentxs/util/Log.hpp"

namespace opentxs
{
using namespace std::literals;

auto print(AccountType in) noexcept -> std::string_view
{
    using enum AccountType;
    static constexpr auto map =
        frozen::make_unordered_map<AccountType, std::string_view>({
            {Blockchain, "blockchain"sv},
            {Custodial, "custodial"sv},
        });

    if (const auto* i = map.find(in); map.end() != i) {

        return i->second;
    } else {

        return "unknown opentxs::AccountType"sv;
    }
}

auto print(AddressType in) noexcept -> std::string_view
{
    using enum AddressType;
    static constexpr auto map =
        frozen::make_unordered_map<AddressType, std::string_view>({
            {IPV4, "ipv4"sv},
            {IPV6, "ipv6"sv},
            {Onion2, "onion"sv},
            {EEP, "eep"sv},
            {Inproc, "inproc"sv},
        });

    if (const auto* i = map.find(in); map.end() != i) {

        return i->second;
    } else {

        return "unknown opentxs::AddressType"sv;
    }
}

auto print(BlockchainProfile in) noexcept -> std::string_view
{
    using enum BlockchainProfile;
    static constexpr auto map =
        frozen::make_unordered_map<BlockchainProfile, std::string_view>({
            {mobile, "mobile"sv},
            {desktop, "desktop"sv},
            {desktop_native, "desktop_native"sv},
            {server, "server"sv},
        });

    if (const auto* i = map.find(in); map.end() != i) {

        return i->second;
    } else {

        return "unknown opentxs::BlockchainProfile"sv;
    }
}

auto print(OTZMQWorkType in) noexcept -> std::string_view
{
    static const auto map = boost::unordered_flat_map<
        OTZMQWorkType,
        std::string_view>{
        {value(WorkType::Shutdown), "WorkType::Shutdown"sv},
        {value(WorkType::NymCreated), "WorkType::NymCreated"sv},
        {value(WorkType::NymUpdated), "WorkType::NymUpdated"sv},
        {value(WorkType::NotaryUpdated), "WorkType::NotaryUpdated"sv},
        {value(WorkType::UnitDefinitionUpdated),
         "WorkType::UnitDefinitionUpdated"sv},
        {value(WorkType::ContactUpdated), "WorkType::ContactUpdated"sv},
        {value(WorkType::AccountUpdated), "WorkType::AccountUpdated"sv},
        {value(WorkType::IssuerUpdated), "WorkType::IssuerUpdated"sv},
        {value(WorkType::ContactActivityUpdated),
         "WorkType::ContactActivityUpdated"sv},
        {value(WorkType::UIModelUpdated), "WorkType::UIModelUpdated"sv},
        {value(WorkType::WorkflowAccountUpdate),
         "WorkType::WorkflowAccountUpdate"sv},
        {value(WorkType::MessageLoaded), "WorkType::MessageLoaded"sv},
        {value(WorkType::SeedUpdated), "WorkType::SeedUpdated"sv},
        {value(WorkType::BlockchainAccountCreated),
         "WorkType::BlockchainAccountCreated"sv},
        {value(WorkType::BlockchainBalance), "WorkType::BlockchainBalance"sv},
        {value(WorkType::BlockchainNewHeader),
         "WorkType::BlockchainNewHeader"sv},
        {value(WorkType::BlockchainNewTransaction),
         "WorkType::BlockchainNewTransaction"sv},
        {value(WorkType::BlockchainPeerAdded),
         "WorkType::BlockchainPeerAdded"sv},
        {value(WorkType::BlockchainReorg), "WorkType::BlockchainReorg"sv},
        {value(WorkType::BlockchainStateChange),
         "WorkType::BlockchainStateChange"sv},
        {value(WorkType::BlockchainSyncProgress),
         "WorkType::BlockchainSyncProgress"sv},
        {value(WorkType::BlockchainWalletScanProgress),
         "WorkType::BlockchainWalletScanProgress"sv},
        {value(WorkType::BlockchainNewFilter),
         "WorkType::BlockchainNewFilter"sv},
        {value(WorkType::BlockchainBlockDownloadQueue),
         "WorkType::BlockchainBlockDownloadQueue"sv},
        {value(WorkType::BlockchainPeerConnected),
         "WorkType::BlockchainPeerConnected"sv},
        {value(WorkType::BlockchainWalletUpdated),
         "WorkType::BlockchainWalletUpdated"sv},
        {value(WorkType::SyncServerUpdated), "WorkType::SyncServerUpdated"sv},
        {value(WorkType::BlockchainMempoolUpdated),
         "WorkType::BlockchainMempoolUpdated"sv},
        {value(WorkType::BlockchainBlockAvailable),
         "WorkType::BlockchainBlockAvailable"sv},
        {value(WorkType::OTXConnectionStatus),
         "WorkType::OTXConnectionStatus"sv},
        {value(WorkType::OTXTaskComplete), "WorkType::OTXTaskComplete"sv},
        {value(WorkType::OTXSearchNym), "WorkType::OTXSearchNym"sv},
        {value(WorkType::OTXSearchServer), "WorkType::OTXSearchServer"sv},
        {value(WorkType::OTXSearchUnit), "WorkType::OTXSearchUnit"sv},
        {value(WorkType::OTXMessagability), "WorkType::OTXMessagability"sv},
        {value(WorkType::reserved1), "WorkType::reserved1"sv},
        {value(WorkType::reserved2), "WorkType::reserved2"sv},
        {value(WorkType::reserved3), "WorkType::reserved3"sv},
        {value(WorkType::P2PBlockchainSyncRequest),
         "WorkType::P2PBlockchainSyncRequest"sv},
        {value(WorkType::P2PBlockchainSyncAck),
         "WorkType::P2PBlockchainSyncAck"sv},
        {value(WorkType::P2PBlockchainSyncReply),
         "WorkType::P2PBlockchainSyncReply"sv},
        {value(WorkType::P2PBlockchainNewBlock),
         "WorkType::P2PBlockchainNewBlock"sv},
        {value(WorkType::P2PBlockchainSyncQuery),
         "WorkType::P2PBlockchainSyncQuery"sv},
        {value(WorkType::P2PResponse), "WorkType::P2PResponse"sv},
        {value(WorkType::P2PPublishContract), "WorkType::P2PPublishContract"sv},
        {value(WorkType::P2PQueryContract), "WorkType::P2PQueryContract"sv},
        {value(WorkType::P2PPushTransaction), "WorkType::P2PPushTransaction"sv},
        {value(WorkType::AsioRegister), "WorkType::AsioRegister"sv},
        {value(WorkType::AsioConnect), "WorkType::AsioConnect"sv},
        {value(WorkType::AsioDisconnect), "WorkType::AsioDisconnect"sv},
        {value(WorkType::AsioSendResult), "WorkType::AsioSendResult"sv},
        {value(WorkType::AsioResolve), "WorkType::AsioResolve"sv},
        {value(WorkType::BlockchainOverZeroMQ),
         "WorkType::BlockchainOverZeroMQ"sv},
        {value(WorkType::OTXRequest), "WorkType::OTXRequest"sv},
        {value(WorkType::OTXResponse), "WorkType::OTXResponse"sv},
        {value(WorkType::OTXPush), "WorkType::OTXPush"sv},
        {value(WorkType::OTXLegacyXML), "WorkType::OTXLegacyXML"sv},
        {OT_ZMQ_BLOCKCHAIN_BROADCAST_TX, "OT_ZMQ_BLOCKCHAIN_BROADCAST_TX"sv},
        {OT_ZMQ_BLOCKCHAIN_SYNC_CHECKSUM_FAILURE,
         "OT_ZMQ_BLOCKCHAIN_SYNC_CHECKSUM_FAILURE"sv},
        {OT_ZMQ_BLOCKCHAIN_REPORT_STATUS, "OT_ZMQ_BLOCKCHAIN_REPORT_STATUS"sv},
        {OT_ZMQ_OTDHT_PEER_LIST, "OT_ZMQ_OTDHT_PEER_LIST"sv},
        {OT_ZMQ_HEADER_ORACLE_JOB_READY, "OT_ZMQ_HEADER_ORACLE_JOB_READY"sv},
        {OT_ZMQ_REORG_SIGNAL, "OT_ZMQ_REORG_SIGNAL"sv},
        {OT_ZMQ_NEW_BLOCK_HEADER_SIGNAL, "OT_ZMQ_NEW_BLOCK_HEADER_SIGNAL"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_PREPARE_REORG,
         "OT_ZMQ_BLOCKCHAIN_WALLET_PREPARE_REORG"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_UPDATE, "OT_ZMQ_BLOCKCHAIN_WALLET_UPDATE"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_PROCESS,
         "OT_ZMQ_BLOCKCHAIN_WALLET_PROCESS"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_WATCHDOG,
         "OT_ZMQ_BLOCKCHAIN_WALLET_WATCHDOG"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_WATCHDOG_ACK,
         "OT_ZMQ_BLOCKCHAIN_WALLET_WATCHDOG_ACK"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_REPROCESS,
         "OT_ZMQ_BLOCKCHAIN_WALLET_REPROCESS"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_RESCAN, "OT_ZMQ_BLOCKCHAIN_WALLET_RESCAN"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_DO_RESCAN,
         "OT_ZMQ_BLOCKCHAIN_WALLET_DO_RESCAN"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_PERFORM_REORG,
         "OT_ZMQ_BLOCKCHAIN_WALLET_PERFORM_REORG"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_FINISH_REORG,
         "OT_ZMQ_BLOCKCHAIN_WALLET_FINISH_REORG"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_REORG_READY,
         "OT_ZMQ_BLOCKCHAIN_WALLET_REORG_READY"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_SHUTDOWN_READY,
         "OT_ZMQ_BLOCKCHAIN_WALLET_SHUTDOWN_READY"sv},
        {OT_ZMQ_BALANCE_ORACLE_SUBMIT, "OT_ZMQ_BALANCE_ORACLE_SUBMIT"sv},
        {OT_ZMQ_FEE_ORACLE_READY, "OT_ZMQ_FEE_ORACLE_READY"sv},
        {OT_ZMQ_BLOCKCHAIN_WALLET_READY, "OT_ZMQ_BLOCKCHAIN_WALLET_READY"sv},
        {OT_ZMQ_PEER_MANAGER_READY, "OT_ZMQ_PEER_MANAGER_READY"sv},
        {OT_ZMQ_FILTER_ORACLE_HEADER_DOWNLOADER_READY,
         "OT_ZMQ_FILTER_ORACLE_HEADER_DOWNLOADER_READY"sv},
        {OT_ZMQ_PEER_MANAGER_READY, "OT_ZMQ_PEER_MANAGER_READY"sv},
        {OT_ZMQ_FILTER_ORACLE_HEADER_DOWNLOADER_READY,
         "OT_ZMQ_FILTER_ORACLE_HEADER_DOWNLOADER_READY"sv},
        {OT_ZMQ_FILTER_ORACLE_FILTER_DOWNLOADER_READY,
         "OT_ZMQ_FILTER_ORACLE_FILTER_DOWNLOADER_READY"sv},
        {OT_ZMQ_FILTER_ORACLE_INDEXER_READY,
         "OT_ZMQ_FILTER_ORACLE_INDEXER_READY"sv},
        {OT_ZMQ_FILTER_ORACLE_READY, "OT_ZMQ_FILTER_ORACLE_READY"sv},
        {OT_ZMQ_BLOCK_ORACLE_BLOCK_READY, "OT_ZMQ_BLOCK_ORACLE_BLOCK_READY"sv},
        {OT_ZMQ_BLOCK_ORACLE_JOB_AVAILABLE,
         "OT_ZMQ_BLOCK_ORACLE_JOB_AVAILABLE"sv},
        {OT_ZMQ_BLOCK_ORACLE_DOWNLOADER_READY,
         "OT_ZMQ_BLOCK_ORACLE_DOWNLOADER_READY"sv},
        {OT_ZMQ_BLOCK_ORACLE_READY, "OT_ZMQ_BLOCK_ORACLE_READY"sv},
        {OT_ZMQ_SYNC_SERVER_BACKEND_READY,
         "OT_ZMQ_SYNC_SERVER_BACKEND_READY"sv},
        {OT_ZMQ_BLOCKCHAIN_NODE_READY, "OT_ZMQ_BLOCKCHAIN_NODE_READY"sv},
        {OT_ZMQ_UNBIND_SIGNAL, "OT_ZMQ_UNBIND_SIGNAL"sv},
        {OT_ZMQ_BIND_SIGNAL, "OT_ZMQ_BIND_SIGNAL"sv},
        {OT_ZMQ_DISCONNECT_SIGNAL, "OT_ZMQ_DISCONNECT_SIGNAL"sv},
        {OT_ZMQ_CONNECT_SIGNAL, "OT_ZMQ_CONNECT_SIGNAL"sv},
        {OT_ZMQ_UNREGISTER_SIGNAL, "OT_ZMQ_UNREGISTER_SIGNAL"sv},
        {OT_ZMQ_REGISTER_SIGNAL, "OT_ZMQ_REGISTER_SIGNAL"sv},
        {OT_ZMQ_HEARTBEAT_SIGNAL, "OT_ZMQ_HEARTBEAT_SIGNAL"sv},
        {OT_ZMQ_SYNC_DATA_SIGNAL, "OT_ZMQ_SYNC_DATA_SIGNAL"sv},
        {OT_ZMQ_NEW_FULL_BLOCK_SIGNAL, "OT_ZMQ_NEW_FULL_BLOCK_SIGNAL"sv},
        {OT_ZMQ_INIT_SIGNAL, "OT_ZMQ_INIT_SIGNAL"sv},
        {OT_ZMQ_NEW_BLOCKCHAIN_WALLET_KEY_SIGNAL,
         "OT_ZMQ_NEW_BLOCKCHAIN_WALLET_KEY_SIGNAL"sv},
        {OT_ZMQ_NEW_FILTER_SIGNAL, "OT_ZMQ_NEW_FILTER_SIGNAL"sv},
        {OT_ZMQ_RECEIVE_SIGNAL, "OT_ZMQ_RECEIVE_SIGNAL"sv},
        {OT_ZMQ_SEND_SIGNAL, "OT_ZMQ_SEND_SIGNAL"sv},
        {OT_ZMQ_ACKNOWLEDGE_SHUTDOWN, "OT_ZMQ_ACKNOWLEDGE_SHUTDOWN"sv},
        {OT_ZMQ_PREPARE_SHUTDOWN, "OT_ZMQ_PREPARE_SHUTDOWN"sv},
        {OT_ZMQ_STATE_MACHINE_SIGNAL, "OT_ZMQ_STATE_MACHINE_SIGNAL"sv},
    };

    if (const auto i = map.find(in); map.end() != i) {

        return i->second;
    } else {
        using CustomMap = UnallocatedMap<OTZMQWorkType, UnallocatedCString>;
        using GuardedMap = libguarded::plain_guarded<CustomMap>;
        static auto custom = GuardedMap{};
        auto handle = custom.lock();
        auto& alt = *handle;

        if (auto j = alt.find(in); alt.end() != j) {

            return j->second;
        } else {

            return alt
                .try_emplace(
                    in,
                    [&] {
                        auto out = std::stringstream{};
                        out << "unknown ";

                        if (in < 16384u) {
                            out << "public";
                        } else if (in < OT_ZMQ_INTERNAL_SIGNAL) {
                            out << "reserved";
                        } else {
                            out << "internal";
                        }

                        out << " value: " << std::to_string(in);

                        return out.str();
                    }())
                .first->second;
        }
    }
}

auto print(UnitType in) noexcept -> std::string_view
{
    return protobuf::TranslateItemType(translate(UnitToClaim(in)));
}

auto print(WorkType in) noexcept -> std::string_view
{
    return print(value(in));
}

// https://www.boost.org/doc/libs/1_80_0/libs/json/doc/html/json/examples.html#json.examples.pretty
auto print(
    const boost::json::value& jv,
    std::ostream& os,
    std::string* indent) noexcept -> bool
{
    try {
        auto indent_ = std::string{};

        if (nullptr == indent) { indent = std::addressof(indent_); }

        switch (jv.kind()) {
            case boost::json::kind::object: {
                os << "{\n";
                indent->append(4, ' ');
                auto const& obj = jv.get_object();

                if (!obj.empty()) {
                    const auto* it = obj.begin();

                    for (;;) {
                        os << *indent << boost::json::serialize(it->key())
                           << ": ";

                        if (false == print(it->value(), os, indent)) {

                            return false;
                        }

                        if (++it == obj.end()) { break; }

                        os << ",\n";
                    }
                }

                os << "\n";
                indent->resize(indent->size() - 4);
                os << *indent << "}";
            } break;
            case boost::json::kind::array: {
                os << "[\n";
                indent->append(4, ' ');
                auto const& arr = jv.get_array();

                if (!arr.empty()) {
                    const auto* it = arr.begin();

                    for (;;) {
                        os << *indent;

                        if (false == print(*it, os, indent)) { return false; }

                        if (++it == arr.end()) { break; }

                        os << ",\n";
                    }
                }

                os << "\n";
                indent->resize(indent->size() - 4);
                os << *indent << "]";
            } break;
            case boost::json::kind::string: {
                os << boost::json::serialize(jv.get_string());
            } break;
            case boost::json::kind::uint64: {
                os << jv.get_uint64();
            } break;
            case boost::json::kind::int64: {
                os << jv.get_int64();
            } break;
            case boost::json::kind::double_: {
                os << jv.get_double();
            } break;
            case boost::json::kind::bool_: {
                if (jv.get_bool()) {
                    os << "true";
                } else {
                    os << "false";
                }
            } break;
            case boost::json::kind::null: {
                os << "null";
            } break;
            default: {
                LogAbort()()("unknown json value type").Abort();
            }
        }

        if (indent->empty()) { os << "\n"; }

        return true;
    } catch (const std::exception& e) {
        LogError()()("")(e.what()).Flush();

        return false;
    }
}
}  // namespace opentxs
