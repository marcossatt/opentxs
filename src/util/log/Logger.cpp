// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "util/log/Logger.hpp"  // IWYU pragma: associated

#include <cassert>
#include <memory>

#include "internal/network/zeromq/Context.hpp"
#include "internal/network/zeromq/socket/Raw.hpp"
#include "internal/util/Log.hpp"
#include "opentxs/api/Context.internal.hpp"
#include "opentxs/network/zeromq/Context.hpp"
#include "opentxs/network/zeromq/socket/SocketType.hpp"
#include "opentxs/network/zeromq/socket/Types.hpp"
#include "util/log/LogBuffer.hpp"

namespace opentxs
{
auto GetLogger() noexcept -> std::shared_ptr<internal::Logger>
{
    static auto logger = std::make_shared<internal::Logger>();

    return logger;
}
}  // namespace opentxs

namespace opentxs::internal
{
auto Logger::Register(const std::thread::id id) noexcept
    -> std::pair<int, std::shared_ptr<Source>>
{
    auto handle = data_.lock();
    auto& [disabled, session, map] = *handle;

    if (disabled) {

        return std::make_pair(session, nullptr);
    } else if (auto i = map.find(id); map.end() == i) {
        auto weak = get_zeromq();

        if (auto zmq = weak.lock(); zmq) {
            using enum network::zeromq::socket::Type;
            auto [it, rc] = map.try_emplace(
                id,
                std::make_shared<Source>(
                    std::string{}, zmq->Internal().RawSocket(Push)));

            assert(rc);

            auto& source = it->second;

            assert(source);

            auto& [buffer, socket] = *source;

            LogBuffer::Reset(id, buffer);
            rc = socket.SetOutgoingHWM(0);

            assert(rc);

            rc = socket.Connect(internal::Log::Endpoint());

            assert(rc);

            return std::make_pair(session, source);
        } else {

            return std::make_pair(session, nullptr);
        }
    } else {

        return std::make_pair(session, i->second);
    }
}

auto Logger::Session() const noexcept -> int
{
    return data_.lock_shared()->session_counter_;
}

auto Logger::Start() noexcept -> void
{
    auto handle = data_.lock();
    auto& [disabled, session, map] = *handle;
    disabled = false;
    ++session;
}

auto Logger::Stop() noexcept -> void
{
    auto handle = data_.lock();
    auto& [disabled, session, map] = *handle;
    disabled = true;
    map.clear();
}

auto Logger::Unregister(const std::thread::id id) noexcept -> void
{
    auto handle = data_.lock();
    auto& [disabled, session, map] = *handle;
    map.erase(id);
}
}  // namespace opentxs::internal
