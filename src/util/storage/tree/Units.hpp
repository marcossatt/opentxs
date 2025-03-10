// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <opentxs/protobuf/StorageUnits.pb.h>
#include <memory>
#include <mutex>
#include <string_view>

#include "internal/util/Mutex.hpp"
#include "opentxs/storage/Types.hpp"
#include "opentxs/storage/Types.internal.hpp"
#include "opentxs/util/Container.hpp"
#include "util/storage/tree/Node.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace api
{
namespace session
{
class Factory;
}  // namespace session

class Crypto;
}  // namespace api

namespace identifier
{
class UnitDefinition;
}  // namespace identifier

namespace protobuf
{
class UnitDefinition;
}  // namespace protobuf

namespace storage
{
namespace driver
{
class Plugin;
}  // namespace driver

namespace tree
{
class Trunk;
}  // namespace tree
}  // namespace storage
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::storage::tree
{
class Units final : public Node
{
public:
    auto Alias(const identifier::UnitDefinition& id) const
        -> UnallocatedCString;
    auto Load(
        const identifier::UnitDefinition& id,
        std::shared_ptr<protobuf::UnitDefinition>& output,
        UnallocatedCString& alias,
        ErrorReporting checking) const -> bool;

    auto Delete(const identifier::UnitDefinition& id) -> bool;
    auto SetAlias(const identifier::UnitDefinition& id, std::string_view alias)
        -> bool;
    auto Store(
        const protobuf::UnitDefinition& data,
        std::string_view alias,
        UnallocatedCString& plaintext) -> bool;

    Units() = delete;
    Units(const Units&) = delete;
    Units(Units&&) = delete;
    auto operator=(const Units&) -> Units = delete;
    auto operator=(Units&&) -> Units = delete;

    ~Units() final = default;

private:
    friend Trunk;

    auto init(const Hash& hash) noexcept(false) -> void final;
    auto save(const std::unique_lock<std::mutex>& lock) const -> bool final;
    auto serialize() const -> protobuf::StorageUnits;
    auto upgrade(const Lock& lock) noexcept -> bool final;

    Units(
        const api::Crypto& crypto,
        const api::session::Factory& factory,
        const driver::Plugin& storage,
        const Hash& key);
};
}  // namespace opentxs::storage::tree
