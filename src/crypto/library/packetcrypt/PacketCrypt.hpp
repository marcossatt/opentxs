// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include "internal/blockchain/block/Validator.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace blockchain
{
namespace node
{
class HeaderOracle;
}  // namespace node

namespace protocol
{
namespace bitcoin
{
namespace base
{
namespace block
{
class Block;
}  // namespace block
}  // namespace base
}  // namespace bitcoin
}  // namespace protocol
}  // namespace blockchain
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::crypto::implementation
{
class PacketCrypt final : public blockchain::block::Validator
{
public:
    auto Validate(const blockchain::protocol::bitcoin::base::block::Block&
                      block) const noexcept -> bool final;

    PacketCrypt(const blockchain::node::HeaderOracle& oracle) noexcept;
    PacketCrypt() = delete;
    PacketCrypt(const PacketCrypt&) = delete;
    PacketCrypt(PacketCrypt&&) = delete;
    auto operator=(const PacketCrypt&) -> PacketCrypt& = delete;
    auto operator=(PacketCrypt&&) -> PacketCrypt& = delete;

    ~PacketCrypt() final;

private:
    struct Imp;

    std::unique_ptr<Imp> imp_;
};
}  // namespace opentxs::crypto::implementation
