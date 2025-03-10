// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <span>

#include "internal/network/blockchain/bitcoin/message/Message.hpp"
#include "internal/util/PMR.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace network
{
namespace blockchain
{
namespace bitcoin
{
namespace message
{
namespace internal
{
class MessagePrivate;
}  // namespace internal
}  // namespace message

class Inventory;
}  // namespace bitcoin
}  // namespace blockchain
}  // namespace network
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::network::blockchain::bitcoin::message::internal
{
class Getdata final : virtual public Message
{
public:
    using value_type = Inventory;

    static auto Blank() noexcept -> Getdata&;

    auto get() const noexcept -> std::span<const value_type>;

    auto get() noexcept -> std::span<value_type>;
    auto get_deleter() noexcept -> delete_function final
    {
        return pmr::make_deleter(this);
    }

    Getdata(MessagePrivate* imp) noexcept;
    Getdata(allocator_type alloc = {}) noexcept;
    Getdata(const Getdata& rhs, allocator_type alloc = {}) noexcept;
    Getdata(Getdata&& rhs) noexcept;
    Getdata(Getdata&& rhs, allocator_type alloc) noexcept;
    auto operator=(const Getdata& rhs) noexcept -> Getdata&;
    auto operator=(Getdata&& rhs) noexcept -> Getdata&;

    ~Getdata() final;
};
}  // namespace opentxs::network::blockchain::bitcoin::message::internal
