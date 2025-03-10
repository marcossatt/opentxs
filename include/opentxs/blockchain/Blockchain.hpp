// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/Export.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/util/Container.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace api
{
class Crypto;
}  // namespace api

namespace crypto
{
class Hasher;
}  // namespace crypto

class Data;
class Writer;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::blockchain
{
OPENTXS_EXPORT auto BlockHash(
    const api::Crypto& crypto,
    const Type chain,
    const ReadView input,
    Writer&& output) noexcept -> bool;
OPENTXS_EXPORT auto BlockHasher(
    const api::Crypto& crypto,
    const Type chain) noexcept -> opentxs::crypto::Hasher;
OPENTXS_EXPORT auto FilterHash(
    const api::Crypto& crypto,
    const Type chain,
    const ReadView input,
    Writer&& output) noexcept -> bool;
OPENTXS_EXPORT auto FilterHasher(
    const api::Crypto& crypto,
    const Type chain) noexcept -> opentxs::crypto::Hasher;
OPENTXS_EXPORT auto HashToNumber(ReadView hex) noexcept -> UnallocatedCString;
OPENTXS_EXPORT auto HashToNumber(const Data& hash) noexcept
    -> UnallocatedCString;
OPENTXS_EXPORT auto MerkleHash(
    const api::Crypto& crypto,
    const Type chain,
    const ReadView input,
    Writer&& output) noexcept -> bool;
OPENTXS_EXPORT auto MerkleHasher(
    const api::Crypto& crypto,
    const Type chain) noexcept -> opentxs::crypto::Hasher;
OPENTXS_EXPORT auto NumberToHash(HexType, ReadView hex, Writer&& out) noexcept
    -> bool;
OPENTXS_EXPORT auto P2PMessageHash(
    const api::Crypto& crypto,
    const Type chain,
    const ReadView input,
    Writer&& output) noexcept -> bool;
OPENTXS_EXPORT auto ProofOfWorkHash(
    const api::Crypto& crypto,
    const Type chain,
    const ReadView input,
    Writer&& output) noexcept -> bool;
OPENTXS_EXPORT auto PubkeyHash(
    const api::Crypto& crypto,
    const Type chain,
    const ReadView input,
    Writer&& output) noexcept -> bool;
OPENTXS_EXPORT auto ScriptHash(
    const api::Crypto& crypto,
    const Type chain,
    const ReadView input,
    Writer&& output) noexcept -> bool;
OPENTXS_EXPORT auto ScriptHasher(
    const api::Crypto& crypto,
    const Type chain) noexcept -> opentxs::crypto::Hasher;
OPENTXS_EXPORT auto ScriptHashSegwit(
    const api::Crypto& crypto,
    const Type chain,
    const ReadView input,
    Writer&& output) noexcept -> bool;
OPENTXS_EXPORT auto ScriptHasherSegwit(
    const api::Crypto& crypto,
    const Type chain) noexcept -> opentxs::crypto::Hasher;
OPENTXS_EXPORT auto TransactionHash(
    const api::Crypto& crypto,
    const Type chain,
    const ReadView input,
    Writer&& output) noexcept -> bool;
OPENTXS_EXPORT auto TransactionHasher(
    const api::Crypto& crypto,
    const Type chain) noexcept -> opentxs::crypto::Hasher;
}  // namespace opentxs::blockchain
