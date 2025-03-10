// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "internal/blockchain/block/Transaction.hpp"
#include "internal/blockchain/protocol/bitcoin/base/Bitcoin.hpp"
#include "internal/blockchain/protocol/bitcoin/base/block/Types.hpp"
#include "opentxs/Time.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/block/Types.hpp"
#include "opentxs/blockchain/block/Types.internal.hpp"
#include "opentxs/blockchain/cfilter/Types.hpp"
#include "opentxs/blockchain/crypto/Types.hpp"
#include "opentxs/blockchain/protocol/bitcoin/base/block/Types.hpp"
#include "opentxs/util/Allocator.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace api
{
namespace crypto
{
class Blockchain;
}  // namespace crypto

class Factory;
class Session;
}  // namespace api

namespace blockchain
{
namespace block
{
class Position;
}  // namespace block

namespace protocol
{
namespace bitcoin
{
namespace base
{
namespace block
{
class Input;
class Output;
}  // namespace block
}  // namespace base
}  // namespace bitcoin
}  // namespace protocol
}  // namespace blockchain

namespace identifier
{
class Generic;
}  // namespace identifier

namespace protobuf
{
class BlockchainTransaction;
}  // namespace protobuf

class Log;
class Writer;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::blockchain::protocol::bitcoin::base::block::internal
{
class Transaction : virtual public blockchain::block::internal::Transaction
{
public:
    using SerializeType = protobuf::BlockchainTransaction;
    using SigHash = blockchain::protocol::bitcoin::base::SigOption;

    static auto Blank() noexcept -> Transaction&;

    auto asBitcoin() const noexcept
        -> const protocol::bitcoin::base::block::internal::Transaction& final
    {
        return *this;
    }
    virtual auto CalculateSize() const noexcept -> std::size_t;
    virtual auto ConfirmationHeight() const noexcept -> block::Height;
    virtual auto ExtractElements(const cfilter::Type style, Elements& out)
        const noexcept -> void;
    virtual auto FindMatches(
        const api::Session& api,
        const cfilter::Type type,
        const Patterns& txos,
        const ParsedPatterns& elements,
        const Log& log,
        Matches& out,
        alloc::Default monotonic) const noexcept -> void;
    virtual auto FindMatches(
        const api::Session& api,
        const cfilter::Type type,
        const Patterns& txos,
        const ParsedPatterns& elements,
        const Log& log,
        alloc::Default alloc,
        alloc::Default monotonic) const noexcept -> Matches;
    virtual auto GetPreimageBTC(
        const std::size_t index,
        const blockchain::protocol::bitcoin::base::SigHash& hashType)
        const noexcept -> Space;
    virtual auto IDNormalized(const api::Factory& factory) const noexcept
        -> const identifier::Generic&;
    virtual auto IndexElements(const api::Session& api, alloc::Default alloc)
        const noexcept -> ElementHashes;
    virtual auto Inputs() const noexcept -> std::span<const block::Input>;
    virtual auto IsGeneration() const noexcept -> bool;
    virtual auto Locktime() const noexcept -> std::uint32_t;
    // WARNING do not call this function if another thread has a non-const
    // reference to this object
    virtual auto MinedPosition() const noexcept -> const block::Position&;
    virtual auto Outputs() const noexcept -> std::span<const block::Output>;
    virtual auto SegwitFlag() const noexcept -> std::byte;
    virtual auto Serialize(EncodedTransaction& out) const noexcept -> bool;
    virtual auto Serialize(Writer&& destination) const noexcept
        -> std::optional<std::size_t>;
    virtual auto Serialize(const api::Session& api) const noexcept
        -> std::optional<SerializeType>;
    virtual auto Timestamp() const noexcept -> Time;
    virtual auto Version() const noexcept -> std::int32_t;
    virtual auto vBytes(blockchain::Type chain) const noexcept -> std::size_t;

    auto asBitcoin() noexcept
        -> protocol::bitcoin::base::block::internal::Transaction& final
    {
        return *this;
    }
    virtual auto AssociatePreviousOutput(
        const std::size_t inputIndex,
        const block::Output& output) noexcept -> bool;
    virtual auto ForTestingOnlyAddKey(
        const std::size_t index,
        const blockchain::crypto::Key& key) noexcept -> bool;
    virtual auto MergeMetadata(
        const api::crypto::Blockchain& crypto,
        const blockchain::Type chain,
        const Transaction& rhs,
        const Log& log) noexcept -> void;
    virtual auto RefreshContacts(const api::crypto::Blockchain& api) noexcept
        -> void;
    virtual auto SetKeyData(const KeyData& data) noexcept -> void;
    virtual auto SetMemo(const std::string_view memo) noexcept -> void;
    virtual auto SetPosition(std::size_t position) noexcept -> void;

    ~Transaction() override = default;
};
}  // namespace opentxs::blockchain::protocol::bitcoin::base::block::internal
