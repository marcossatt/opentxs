// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <type_traits>

#include "opentxs/core/contract/peer/Types.hpp"  // IWYU pragma: keep

namespace opentxs::contract::peer
{
enum class ObjectType : std::underlying_type_t<ObjectType> {
    Error = 0,
    Message = 1,
    Request = 2,
    Response = 3,
    Payment = 4,
    Cash = 5,
};
}  // namespace opentxs::contract::peer
