// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/api/internal.factory.hpp"  // IWYU pragma: associated

#include "opentxs/api/Paths.internal.hpp"
#include "opentxs/api/PathsPrivate.hpp"

namespace opentxs::factory
{
auto Paths(const std::filesystem::path& home) noexcept -> api::internal::Paths
{
    using ReturnType = opentxs::api::internal::PathsPrivate;

    return std::make_unique<ReturnType>(home);
}
}  // namespace opentxs::factory
