// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "internal/util/Log.hpp"  // IWYU pragma: associated

#include <atomic>
#include <memory>

#include "opentxs/network/zeromq/Types.hpp"
#include "util/log/Logger.hpp"

namespace opentxs::internal
{
auto Log::Endpoint() noexcept -> const char*
{
    static const auto output =
        network::zeromq::MakeDeterministicInproc("logsink", -1, 1);

    return output.c_str();
}

auto Log::SetVerbosity(const int level) noexcept -> void
{
    static auto logger = GetLogger();

    if (logger) { logger->verbosity_ = level; }
}

auto Log::Shutdown() noexcept -> void
{
    static auto logger = GetLogger();

    if (logger) { logger->Stop(); }
}

auto Log::Start() noexcept -> void
{
    static auto logger = GetLogger();

    if (logger) { logger->Start(); }
}
}  // namespace opentxs::internal
