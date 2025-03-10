// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_forward_declare opentxs::crypto::asymmetric::Key

#include "opentxs/crypto/asymmetric/key/Ed25519.hpp"  // IWYU pragma: associated

#include <utility>

#include "crypto/asymmetric/key/ed25519/Ed25519Private.hpp"
#include "internal/util/PMR.hpp"
#include "opentxs/crypto/asymmetric/Key.hpp"
#include "opentxs/util/Allocator.hpp"

namespace opentxs::crypto::asymmetric::key
{
Ed25519::Ed25519(KeyPrivate* imp) noexcept
    : HD(imp)
{
}

Ed25519::Ed25519(allocator_type alloc) noexcept
    : Ed25519(Ed25519Private::Blank(alloc))
{
}

Ed25519::Ed25519(const Ed25519& rhs, allocator_type alloc) noexcept
    : HD(rhs, alloc)
{
}

Ed25519::Ed25519(Ed25519&& rhs) noexcept
    : HD(std::move(rhs))
{
}

Ed25519::Ed25519(Ed25519&& rhs, allocator_type alloc) noexcept
    : HD(std::move(rhs), alloc)
{
}

auto Ed25519::Blank() noexcept -> Ed25519&
{
    static auto blank = Ed25519{allocator_type{alloc::Default()}};

    return blank;
}

auto Ed25519::operator=(const Ed25519& rhs) noexcept -> Ed25519&
{
    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    return pmr::copy_assign_child<Key>(*this, rhs);
}

auto Ed25519::operator=(Ed25519&& rhs) noexcept -> Ed25519&
{
    // NOLINTNEXTLINE(misc-unconventional-assign-operator)
    return pmr::move_assign_child<Key>(*this, std::move(rhs));
}

Ed25519::~Ed25519() = default;
}  // namespace opentxs::crypto::asymmetric::key
