// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_include "opentxs/crypto/symmetric/Key.hpp"

#pragma once

#include <cstddef>
#include <cstdint>

#include "internal/api/crypto/Symmetric.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/crypto/symmetric/Types.hpp"
#include "opentxs/util/Allocator.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace api
{
namespace internal
{
class Session;
}  // namespace internal
}  // namespace api

namespace protobuf
{
class SymmetricKey;
}  // namespace protobuf

class PasswordPrompt;
class Secret;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::api::crypto::imp
{
class Symmetric final : public internal::Symmetric
{
public:
    auto IvSize(const opentxs::crypto::symmetric::Algorithm mode) const noexcept
        -> std::size_t final;
    auto Key(
        opentxs::crypto::symmetric::Algorithm mode,
        const PasswordPrompt& password,
        alloc::Default alloc) const noexcept
        -> opentxs::crypto::symmetric::Key final;
    auto Key(const PasswordPrompt& password, alloc::Default alloc)
        const noexcept -> opentxs::crypto::symmetric::Key final;
    auto Key(
        ReadView serializedCiphertext,
        opentxs::crypto::symmetric::Algorithm mode,
        alloc::Default alloc) const noexcept
        -> opentxs::crypto::symmetric::Key final;
    auto Key(
        const Secret& seed,
        const opentxs::crypto::symmetric::Algorithm mode,
        const opentxs::crypto::symmetric::Source type,
        const std::uint64_t operations,
        const std::uint64_t difficulty,
        alloc::Default alloc) const noexcept
        -> opentxs::crypto::symmetric::Key final;
    auto Key(
        const Secret& seed,
        const opentxs::crypto::symmetric::Source type,
        const std::uint64_t operations,
        const std::uint64_t difficulty,
        alloc::Default alloc) const noexcept
        -> opentxs::crypto::symmetric::Key final;
    auto Key(
        const Secret& seed,
        const opentxs::crypto::symmetric::Algorithm mode,
        const std::uint64_t operations,
        const std::uint64_t difficulty,
        alloc::Default alloc) const noexcept
        -> opentxs::crypto::symmetric::Key final;
    auto Key(
        const Secret& seed,
        const std::uint64_t operations,
        const std::uint64_t difficulty,
        alloc::Default alloc) const noexcept
        -> opentxs::crypto::symmetric::Key final;
    auto Key(
        const Secret& seed,
        ReadView salt,
        std::uint64_t operations,
        std::uint64_t difficulty,
        std::uint64_t parallel,
        std::size_t bytes,
        opentxs::crypto::symmetric::Source type,
        alloc::Default alloc) const noexcept
        -> opentxs::crypto::symmetric::Key final;
    auto Key(
        const protobuf::SymmetricKey& serialized,
        const opentxs::crypto::symmetric::Algorithm mode,
        alloc::Default alloc) const noexcept
        -> opentxs::crypto::symmetric::Key final;

    Symmetric(const api::internal::Session& api) noexcept;
    Symmetric() = delete;
    Symmetric(const Symmetric&) = delete;
    Symmetric(Symmetric&&) = delete;
    auto operator=(const Symmetric&) -> Symmetric& = delete;
    auto operator=(Symmetric&&) -> Symmetric& = delete;

    ~Symmetric() final = default;

private:
    const api::internal::Session& api_;
};
}  // namespace opentxs::api::crypto::imp
