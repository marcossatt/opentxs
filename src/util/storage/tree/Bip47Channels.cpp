// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "util/storage/tree/Bip47Channels.hpp"  // IWYU pragma: associated

#include <opentxs/protobuf/Bip47Channel.pb.h>
#include <opentxs/protobuf/BlockchainAccountData.pb.h>
#include <opentxs/protobuf/BlockchainDeterministicAccountData.pb.h>
#include <opentxs/protobuf/StorageBip47ChannelList.pb.h>
#include <opentxs/protobuf/StorageBip47Contexts.pb.h>
#include <atomic>
#include <mutex>
#include <source_location>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "internal/util/DeferredConstruction.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/UnitType.hpp"  // IWYU pragma: keep
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/FixedByteArray.hpp"  // IWYU pragma: keep
#include "opentxs/identifier/Account.hpp"
#include "opentxs/identifier/Generic.hpp"
#include "opentxs/identity/wot/claim/Types.hpp"
#include "opentxs/identity/wot/claim/Types.internal.hpp"
#include "opentxs/protobuf/Types.internal.hpp"
#include "opentxs/protobuf/syntax/Bip47Channel.hpp"
#include "opentxs/protobuf/syntax/StorageBip47Contexts.hpp"
#include "opentxs/protobuf/syntax/Types.internal.tpp"
#include "opentxs/storage/Types.internal.hpp"
#include "opentxs/util/Container.hpp"
#include "opentxs/util/Log.hpp"
#include "util/storage/tree/Node.hpp"

#define CHANNEL_VERSION 1
#define CHANNEL_INDEX_VERSION 1

namespace opentxs::storage::tree
{
using namespace std::literals;

Bip47Channels::Bip47Channels(
    const api::Crypto& crypto,
    const api::session::Factory& factory,
    const driver::Plugin& storage,
    const Hash& hash)
    : Node(
          crypto,
          factory,
          storage,
          hash,
          std::source_location::current().function_name(),
          CHANNEL_VERSION)
    , index_lock_()
    , channel_data_()
    , chain_index_()
    , repair_indices_(false)
{
    if (is_valid(hash)) {
        init(hash);
    } else {
        blank();
    }
}

auto Bip47Channels::Chain(const identifier::Account& channelID) const
    -> UnitType
{
    auto lock = sLock{index_lock_};

    return get_channel_data(lock, channelID);
}

auto Bip47Channels::ChannelsByChain(const UnitType chain) const
    -> Bip47Channels::ChannelList
{
    return extract_set(chain, chain_index_);
}

template <typename I, typename V>
auto Bip47Channels::extract_set(const I& id, const V& index) const ->
    typename V::mapped_type
{
    auto lock = sLock{index_lock_};

    try {
        return index.at(id);

    } catch (...) {

        return {};
    }
}

template <typename L>
auto Bip47Channels::get_channel_data(
    const L& lock,
    const identifier::Account& id) const -> const Bip47Channels::ChannelData&
{
    try {

        return channel_data_.at(id);
    } catch (const std::out_of_range&) {
        static auto blank = ChannelData{UnitType::Error};

        return blank;
    }
}

auto Bip47Channels::index(
    const eLock& lock,
    const identifier::Account& id,
    const protobuf::Bip47Channel& data) -> void
{
    const auto& common = data.deterministic().common();
    auto& chain = channel_data_[id];
    chain = ClaimToUnit(translate(common.chain()));
    chain_index_[chain].emplace(id);
}

auto Bip47Channels::init(const Hash& hash) noexcept(false) -> void
{
    auto p = std::shared_ptr<protobuf::StorageBip47Contexts>{};

    if (LoadProto(hash, p, verbose) && p) {
        const auto& proto = *p;

        switch (set_original_version(proto.version())) {
            case 1u:
            default: {
                init_map(proto.context());

                if (proto.context().size() != proto.index().size()) {
                    repair_indices_ = true;
                } else {
                    for (const auto& index : proto.index()) {
                        auto id =
                            factory_.AccountIDFromBase58(index.channelid());
                        auto& chain = channel_data_[id];
                        chain = ClaimToUnit(translate(index.chain()));
                        chain_index_[chain].emplace(std::move(id));
                    }
                }
            }
        }
    } else {
        throw std::runtime_error{"failed to load root object file in "s.append(
            std::source_location::current().function_name())};
    }
}

auto Bip47Channels::Load(
    const identifier::Account& id,
    std::shared_ptr<protobuf::Bip47Channel>& output,
    ErrorReporting checking) const -> bool
{
    UnallocatedCString alias{""};

    return load_proto<protobuf::Bip47Channel>(id, output, alias, checking);
}

auto Bip47Channels::repair_indices() noexcept -> void
{
    {
        auto lock = eLock{index_lock_};

        for (const auto& [strid, alias] : List()) {
            const auto id = factory_.AccountIDFromBase58(strid);
            auto data = std::shared_ptr<protobuf::Bip47Channel>{};
            using enum ErrorReporting;
            const auto loaded = Load(id, data, verbose);

            assert_true(loaded);
            assert_false(nullptr == data);

            index(lock, id, *data);
        }
    }
}

auto Bip47Channels::save(const std::unique_lock<std::mutex>& lock) const -> bool
{
    if (!verify_write_lock(lock)) {
        LogError()()("Lock failure.").Flush();
        LogAbort()().Abort();
    }

    auto serialized = serialize();

    if (!protobuf::syntax::check(LogError(), serialized)) { return false; }

    return StoreProto(serialized, root_);
}

auto Bip47Channels::serialize() const -> protobuf::StorageBip47Contexts
{
    auto serialized = protobuf::StorageBip47Contexts{};
    serialized.set_version(version_);

    for (const auto& item : item_map_) {
        const bool goodID = !item.first.empty();
        const bool goodHash = is_valid(std::get<0>(item.second));
        const bool good = goodID && goodHash;

        if (good) {
            serialize_index(item.first, item.second, *serialized.add_context());
        }
    }

    auto lock = sLock{index_lock_};

    for (const auto& [id, data] : channel_data_) {
        const auto& chain = data;
        auto& index = *serialized.add_index();
        index.set_version(CHANNEL_INDEX_VERSION);
        index.set_channelid(id.asBase58(crypto_));
        index.set_chain(translate(UnitToClaim(chain)));
    }

    return serialized;
}

auto Bip47Channels::Store(
    const identifier::Account& id,
    const protobuf::Bip47Channel& data) -> bool
{
    {
        auto lock = eLock{index_lock_};
        index(lock, id, data);
    }

    return store_proto(data, id, "");
}

auto Bip47Channels::upgrade(const Lock& lock) noexcept -> bool
{
    auto changed = Node::upgrade(lock);

    switch (original_version_.get()) {
        case 1u:
        default: {
            if (repair_indices_) {
                repair_indices();
                changed = true;
            }
        }
    }

    return changed;
}
}  // namespace opentxs::storage::tree
