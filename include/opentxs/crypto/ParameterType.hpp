// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <type_traits>

#include "opentxs/crypto/Types.hpp"  // IWYU pragma: keep

namespace opentxs::crypto
{
enum class ParameterType : std::underlying_type_t<ParameterType> {
    invalid = 0,
    rsa = 1,
    secp256k1 = 2,
    ed25519 = 3
};
}  // namespace opentxs::crypto
