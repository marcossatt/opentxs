// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/Types.hpp"
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/asymmetric/Types.hpp"
#include "opentxs/util/Allocator.hpp"
#include "opentxs/util/Numbers.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace api
{
class Session;
}  // namespace api

namespace crypto
{
namespace asymmetric
{
namespace key
{
class Ed25519;
class RSA;
class Secp256k1;
}  // namespace key
}  // namespace asymmetric

class AsymmetricProvider;
class EcdsaProvider;
class Parameters;
}  // namespace crypto

namespace protobuf
{
class AsymmetricKey;
class HDPath;
}  // namespace protobuf

class Data;
class PasswordPrompt;
class Secret;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::factory
{
auto Ed25519Key(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const protobuf::AsymmetricKey& serializedKey,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::Ed25519;
auto Ed25519Key(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::asymmetric::Role role,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::Ed25519;
auto Ed25519Key(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const opentxs::Secret& privateKey,
    const opentxs::Secret& chainCode,
    const Data& publicKey,
    const protobuf::HDPath& path,
    const crypto::Bip32Fingerprint parent,
    const crypto::asymmetric::Role role,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::Ed25519;
auto RSAKey(
    const api::Session& api,
    const crypto::AsymmetricProvider& engine,
    const protobuf::AsymmetricKey& input,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::RSA;
auto RSAKey(
    const api::Session& api,
    const crypto::AsymmetricProvider& engine,
    const crypto::asymmetric::Role input,
    const VersionNumber version,
    const crypto::Parameters& options,
    const opentxs::PasswordPrompt& reason,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::RSA;
auto Secp256k1Key(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const protobuf::AsymmetricKey& serializedKey,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::Secp256k1;
auto Secp256k1Key(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::asymmetric::Role role,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::Secp256k1;
auto Secp256k1Key(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const opentxs::Secret& privateKey,
    const Data& publicKey,
    const crypto::asymmetric::Role role,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::Secp256k1;
auto Secp256k1Key(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const opentxs::Secret& privateKey,
    const opentxs::Secret& chainCode,
    const Data& publicKey,
    const protobuf::HDPath& path,
    const crypto::Bip32Fingerprint parent,
    const crypto::asymmetric::Role role,
    const VersionNumber version,
    const opentxs::PasswordPrompt& reason,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::Secp256k1;
auto Secp256k1Key(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const opentxs::Secret& privateKey,
    const opentxs::Secret& chainCode,
    const Data& publicKey,
    const protobuf::HDPath& path,
    const crypto::Bip32Fingerprint parent,
    const crypto::asymmetric::Role role,
    const VersionNumber version,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::Secp256k1;
auto Secp256k1Key(
    const api::Session& api,
    const ReadView key,
    const ReadView chaincode,
    alloc::Default alloc) noexcept -> crypto::asymmetric::key::Secp256k1;
}  // namespace opentxs::factory
