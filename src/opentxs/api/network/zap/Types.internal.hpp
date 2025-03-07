// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

namespace opentxs::api::network::zap
{
enum class Mechanism : int {
    Unknown = 0,
    Null = 1,
    Plain = 2,
    Curve = 3,
};  // IWYU pragma: export

enum class Status : int {
    Unknown = 0,
    Success = 200,
    TemporaryError = 300,
    AuthFailure = 400,
    SystemError = 500,
};  // IWYU pragma: export
}  // namespace opentxs::api::network::zap
