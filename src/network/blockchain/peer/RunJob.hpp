// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <variant>

#include "internal/network/blockchain/Peer.hpp"
#include "network/blockchain/peer/Imp.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace blockchain
{
namespace node
{
namespace internal
{
class BlockBatch;
class HeaderJob;
}  // namespace internal
}  // namespace node
}  // namespace blockchain
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::network::blockchain::internal
{
class Peer::Imp::RunJob
{
public:
    auto operator()(std::monostate& job) noexcept -> void;
    auto operator()(
        opentxs::blockchain::node::internal::HeaderJob& job) noexcept -> void;
    auto operator()(
        opentxs::blockchain::node::internal::BlockBatch& job) noexcept -> void;

    RunJob(Imp& parent, allocator_type monotonic) noexcept;

private:
    Imp& parent_;
    allocator_type monotonic_;
};
}  // namespace opentxs::network::blockchain::internal
