// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "crypto/asymmetric/key/ed25519/Imp.hpp"  // IWYU pragma: associated

#include <utility>

#include "crypto/asymmetric/base/KeyPrivate.hpp"
#include "crypto/asymmetric/key/ellipticcurve/EllipticCurvePrivate.hpp"
#include "crypto/asymmetric/key/hd/HDPrivate.hpp"
#include "internal/util/Mutex.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/crypto/ParameterType.hpp"  // IWYU pragma: keep
#include "opentxs/crypto/Types.hpp"
#include "opentxs/crypto/asymmetric/Algorithm.hpp"  // IWYU pragma: keep
#include "opentxs/crypto/asymmetric/Types.hpp"
#include "opentxs/util/Log.hpp"
#include "opentxs/util/Numbers.hpp"
#include "opentxs/util/Writer.hpp"
#include "util/Sodium.hpp"

namespace opentxs::crypto::asymmetric::key::implementation
{
Ed25519::Ed25519(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const protobuf::AsymmetricKey& serializedKey,
    allocator_type alloc) noexcept(false)
    : KeyPrivate(alloc)
    , EllipticCurvePrivate(alloc)
    , HDPrivate(alloc)
    , Ed25519Private(alloc)
    , HD(api, ecdsa, serializedKey, alloc)
    , self_(this)
{
}

Ed25519::Ed25519(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const crypto::asymmetric::Role role,
    const VersionNumber version,
    symmetric::Key& sessionKey,
    const PasswordPrompt& reason,
    allocator_type alloc) noexcept(false)
    : KeyPrivate(alloc)
    , EllipticCurvePrivate(alloc)
    , HDPrivate(alloc)
    , Ed25519Private(alloc)
    , HD(api,
         ecdsa,
         crypto::asymmetric::Algorithm::ED25519,
         role,
         version,
         sessionKey,
         reason,
         alloc)
    , self_(this)
{
    assert_false(plaintext_key_.empty());
}

Ed25519::Ed25519(
    const api::Session& api,
    const crypto::EcdsaProvider& ecdsa,
    const opentxs::Secret& privateKey,
    const opentxs::Secret& chainCode,
    const Data& publicKey,
    const protobuf::HDPath& path,
    const Bip32Fingerprint parent,
    const crypto::asymmetric::Role role,
    const VersionNumber version,
    symmetric::Key& sessionKey,
    const PasswordPrompt& reason,
    allocator_type alloc) noexcept(false)
    : KeyPrivate(alloc)
    , EllipticCurvePrivate(alloc)
    , HDPrivate(alloc)
    , Ed25519Private(alloc)
    , HD(api,
         ecdsa,
         crypto::asymmetric::Algorithm::ED25519,
         privateKey,
         chainCode,
         publicKey,
         path,
         parent,
         role,
         version,
         sessionKey,
         reason,
         alloc)
    , self_(this)
{
}

Ed25519::Ed25519(const Ed25519& rhs, allocator_type alloc) noexcept
    : KeyPrivate(alloc)
    , EllipticCurvePrivate(alloc)
    , HDPrivate(alloc)
    , Ed25519Private(alloc)
    , HD(rhs, alloc)
    , self_(this)
{
}

Ed25519::Ed25519(
    const Ed25519& rhs,
    const ReadView newPublic,
    allocator_type alloc) noexcept
    : KeyPrivate(alloc)
    , EllipticCurvePrivate(alloc)
    , HDPrivate(alloc)
    , Ed25519Private(alloc)
    , HD(rhs, newPublic, alloc)
    , self_(this)
{
}

Ed25519::Ed25519(
    const Ed25519& rhs,
    Secret&& newSecretKey,
    allocator_type alloc) noexcept
    : KeyPrivate(alloc)
    , EllipticCurvePrivate(alloc)
    , HDPrivate(alloc)
    , Ed25519Private(alloc)
    , HD(rhs, std::move(newSecretKey), alloc)
    , self_(this)
{
}

auto Ed25519::CreateType() const noexcept -> ParameterType
{
    return ParameterType::ed25519;
}

auto Ed25519::replace_public_key(const ReadView newPubkey, allocator_type alloc)
    const noexcept -> EllipticCurve*
{
    return pmr::construct<Ed25519>(alloc, *this, newPubkey);
}

auto Ed25519::replace_secret_key(Secret&& newSecretKey, allocator_type alloc)
    const noexcept -> EllipticCurve*
{
    return pmr::construct<Ed25519>(alloc, *this, std::move(newSecretKey));
}

auto Ed25519::TransportKey(
    Data& publicKey,
    Secret& privateKey,
    const PasswordPrompt& reason) const noexcept -> bool
{
    auto lock = Lock{lock_};

    if (false == HasPublic()) { return false; }
    if (false == has_private(lock)) { return false; }

    return sodium::ToCurveKeypair(
        private_key(lock, reason),
        PublicKey(),
        privateKey.WriteInto(Secret::Mode::Mem),
        publicKey.WriteInto());
}

Ed25519::~Ed25519() { Reset(self_); }
}  // namespace opentxs::crypto::asymmetric::key::implementation
