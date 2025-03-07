// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "blockchain/node/blockoracle/Shared.hpp"  // IWYU pragma: associated

#include "internal/blockchain/block/Validator.hpp"
#include "opentxs/util/Writer.hpp"

namespace opentxs::blockchain::node::internal
{
auto BlockOracle::Shared::get_validator(
    const blockchain::Type,
    const node::HeaderOracle&) noexcept
    -> std::unique_ptr<const block::Validator>
{
    return std::make_unique<block::Validator>();
}
}  // namespace opentxs::blockchain::node::internal
