// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "network/blockchain/bitcoin/message/base/MessagePrivate.hpp"

#include <span>

#include "internal/network/blockchain/bitcoin/message/Getheaders.hpp"
#include "internal/util/PMR.hpp"
#include "opentxs/util/Allocator.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace blockchain
{
namespace block
{
class Hash;
}  // namespace block
}  // namespace blockchain
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::network::blockchain::bitcoin::message::getheaders
{
class MessagePrivate : virtual public internal::MessagePrivate
{
public:
    [[nodiscard]] static auto Blank(allocator_type alloc) noexcept
        -> MessagePrivate*
    {
        return pmr::default_construct<MessagePrivate>({alloc});
    }

    auto asGetheadersPrivate() const noexcept
        -> const getheaders::MessagePrivate* final
    {
        return this;
    }
    auto asGetheadersPublic() const noexcept
        -> const internal::Getheaders& final
    {
        return self_;
    }
    [[nodiscard]] auto clone(allocator_type alloc) const noexcept
        -> internal::MessagePrivate* override
    {
        return pmr::clone_as<internal::MessagePrivate>(this, {alloc});
    }
    virtual auto get() const noexcept
        -> std::span<const internal::Getheaders::value_type>;
    virtual auto Stop() const noexcept
        -> const opentxs::blockchain::block::Hash&;

    auto asGetheadersPrivate() noexcept -> getheaders::MessagePrivate* final
    {
        return this;
    }
    auto asGetheadersPublic() noexcept -> internal::Getheaders& final
    {
        return self_;
    }
    virtual auto get() noexcept -> std::span<internal::Getheaders::value_type>;
    [[nodiscard]] auto get_deleter() noexcept -> delete_function override
    {
        return pmr::make_deleter(this);
    }

    MessagePrivate(allocator_type alloc) noexcept;
    MessagePrivate() = delete;
    MessagePrivate(const MessagePrivate& rhs, allocator_type alloc) noexcept;
    MessagePrivate(const MessagePrivate&) = delete;
    MessagePrivate(MessagePrivate&&) = delete;
    auto operator=(const MessagePrivate&) -> MessagePrivate& = delete;
    auto operator=(MessagePrivate&&) -> MessagePrivate& = delete;

    ~MessagePrivate() override;

protected:
    internal::Getheaders self_;
};
}  // namespace opentxs::network::blockchain::bitcoin::message::getheaders
