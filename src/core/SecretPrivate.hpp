// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>

#include "core/ByteArrayPrivate.hpp"
#include "internal/util/PMR.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/core/Secret.hpp"
#include "opentxs/util/Allocator.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
class Writer;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs
{
class SecretPrivate final : public ByteArrayPrivate
{
public:
    auto Assign(const void* data, const std::size_t size) noexcept
        -> bool final;
    auto AssignText(const ReadView source) noexcept -> bool;
    auto clone(allocator_type alloc) const noexcept -> SecretPrivate*
    {
        return pmr::clone(this, {alloc});
    }
    auto Mode() const noexcept -> Secret::Mode { return mode_; }
    auto size() const noexcept -> std::size_t final;

    auto get_deleter() noexcept -> delete_function final
    {
        return pmr::make_deleter(this);
    }
    auto WriteInto() noexcept -> Writer final;
    auto WriteInto(Secret::Mode mode) noexcept -> Writer;

    SecretPrivate() = delete;
    SecretPrivate(
        const void* data,
        std::size_t size,
        Secret::Mode mode,
        allocator_type alloc) noexcept;
    SecretPrivate(
        std::size_t size,
        Secret::Mode mode,
        allocator_type alloc) noexcept;
    SecretPrivate(const SecretPrivate& rhs, allocator_type alloc) noexcept;
    SecretPrivate(const SecretPrivate&) = delete;
    SecretPrivate(SecretPrivate&&) = delete;
    auto operator=(const SecretPrivate&) -> SecretPrivate& = delete;
    auto operator=(SecretPrivate&&) -> SecretPrivate& = delete;

    ~SecretPrivate() final = default;

private:
    Secret::Mode mode_;

    static auto mem(Secret::Mode mode) noexcept -> bool
    {
        return Secret::Mode::Mem == mode;
    }

    auto assign(
        const void* data,
        std::size_t copy,
        std::size_t reserve) noexcept -> bool;
    auto mem() const noexcept -> bool { return mem(mode_); }
};
}  // namespace opentxs
