// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "util/storage/tree/Servers.hpp"  // IWYU pragma: associated

#include <opentxs/protobuf/ServerContract.pb.h>
#include <opentxs/protobuf/StorageServers.pb.h>
#include <atomic>
#include <source_location>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "internal/util/DeferredConstruction.hpp"
#include "opentxs/api/Factory.internal.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/core/FixedByteArray.hpp"  // IWYU pragma: keep
#include "opentxs/identifier/Generic.hpp"
#include "opentxs/identifier/Notary.hpp"
#include "opentxs/protobuf/syntax/ServerContract.hpp"
#include "opentxs/protobuf/syntax/StorageServers.hpp"
#include "opentxs/protobuf/syntax/Types.internal.tpp"
#include "opentxs/storage/Types.internal.hpp"
#include "opentxs/util/Container.hpp"
#include "opentxs/util/Log.hpp"
#include "util/storage/tree/Node.hpp"

namespace opentxs::storage::tree
{
using namespace std::literals;

Servers::Servers(
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
          2)
{
    if (is_valid(hash)) {
        init(hash);
    } else {
        blank();
    }
}

auto Servers::Alias(const identifier::Notary& id) const -> UnallocatedCString
{
    return get_alias(id);
}

auto Servers::Delete(const identifier::Notary& id) -> bool
{
    return delete_item(id);
}

auto Servers::init(const Hash& hash) noexcept(false) -> void
{
    auto p = std::shared_ptr<protobuf::StorageServers>{};

    if (LoadProto(hash, p, verbose) && p) {
        const auto& proto = *p;

        switch (set_original_version(proto.version())) {
            case 2u:
            case 1u:
            default: {
                init_map(proto.server());
            }
        }
    } else {
        throw std::runtime_error{"failed to load root object file in "s.append(
            std::source_location::current().function_name())};
    }
}

auto Servers::Load(
    const identifier::Notary& id,
    std::shared_ptr<protobuf::ServerContract>& output,
    UnallocatedCString& alias,
    ErrorReporting checking) const -> bool
{
    return load_proto<protobuf::ServerContract>(id, output, alias, checking);
}

auto Servers::save(const std::unique_lock<std::mutex>& lock) const -> bool
{
    if (!verify_write_lock(lock)) { LogAbort()()("Lock failure").Abort(); }

    auto serialized = serialize();

    if (!protobuf::syntax::check(LogError(), serialized)) { return false; }

    return StoreProto(serialized, root_);
}

auto Servers::serialize() const -> protobuf::StorageServers
{
    protobuf::StorageServers serialized;
    serialized.set_version(version_);

    for (const auto& item : item_map_) {
        const bool goodID = !item.first.empty();
        const bool goodHash = is_valid(std::get<0>(item.second));
        const bool good = goodID && goodHash;

        if (good) {
            serialize_index(item.first, item.second, *serialized.add_server());
        }
    }

    return serialized;
}

auto Servers::SetAlias(const identifier::Notary& id, std::string_view alias)
    -> bool
{
    return set_alias(id, alias);
}

auto Servers::Store(
    const protobuf::ServerContract& data,
    std::string_view alias,
    UnallocatedCString& plaintext) -> bool
{
    const auto id = factory_.Internal().NotaryID(data.id());

    return store_proto(data, id, alias, plaintext);
}

auto Servers::upgrade(const Lock& lock) noexcept -> bool
{
    auto changed = Node::upgrade(lock);

    switch (original_version_.get()) {
        case 1u:
        case 2u:
        default: {
        }
    }

    return changed;
}
}  // namespace opentxs::storage::tree
