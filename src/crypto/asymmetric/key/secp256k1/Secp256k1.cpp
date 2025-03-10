// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_forward_declare opentxs::crypto::asymmetric::Key

#include "opentxs/crypto/asymmetric/key/Secp256k1.hpp"  // IWYU pragma: associated

#include <utility>

#include "crypto/asymmetric/base/KeyPrivate.hpp"
#include "crypto/asymmetric/key/secp256k1/Secp256k1Private.hpp"
#include "internal/util/PMR.hpp"
#include "opentxs/crypto/asymmetric/Key.hpp"
#include "opentxs/util/Allocator.hpp"

namespace opentxs::crypto::asymmetric::key
{
Secp256k1::Secp256k1(KeyPrivate* imp) noexcept
    : HD(imp)
{
}

Secp256k1::Secp256k1(allocator_type alloc) noexcept
    : Secp256k1(Secp256k1Private::Blank(alloc))
{
}

Secp256k1::Secp256k1(const Secp256k1& rhs, allocator_type alloc) noexcept
    : HD(rhs, alloc)
{
}

Secp256k1::Secp256k1(Secp256k1&& rhs) noexcept
    : HD(std::move(rhs))
{
}

Secp256k1::Secp256k1(Secp256k1&& rhs, allocator_type alloc) noexcept
    : HD(std::move(rhs), alloc)
{
}

auto Secp256k1::Blank() noexcept -> Secp256k1&
{
    static auto blank = Secp256k1{allocator_type{alloc::Default()}};

    return blank;
}

auto Secp256k1::operator=(const Secp256k1& rhs) noexcept -> Secp256k1&
{
    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    return pmr::copy_assign_child<Key>(*this, rhs);
}

auto Secp256k1::operator=(Secp256k1&& rhs) noexcept -> Secp256k1&
{
    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    return pmr::move_assign_child<Key>(*this, std::move(rhs));
}

auto Secp256k1::UncompressedPubkey() const noexcept -> ReadView
{
    return imp_->asSecp256k1Private()->UncompressedPubkey();
}

Secp256k1::~Secp256k1() = default;
}  // namespace opentxs::crypto::asymmetric::key
