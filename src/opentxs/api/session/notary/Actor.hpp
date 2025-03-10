// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include "internal/util/PMR.hpp"
#include "opentxs/api/session/notary/Types.internal.hpp"
#include "opentxs/identifier/UnitDefinition.hpp"
#include "opentxs/util/Container.hpp"
#include "util/Actor.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace api
{
namespace session
{
namespace internal
{
class Notary;
}  // namespace internal

namespace notary
{
class Actor;  // IWYU pragma: keep
class Shared;
}  // namespace notary

class Notary;
}  // namespace session
}  // namespace api
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

class opentxs::api::session::notary::Actor final
    : public opentxs::Actor<notary::Actor, Job>
{
public:
    auto Init(std::shared_ptr<Actor> self) noexcept -> void
    {
        signal_startup(self);
    }

    auto get_deleter() noexcept -> delete_function final
    {
        return pmr::make_deleter(this);
    }

    Actor(
        std::shared_ptr<api::session::internal::Notary> api,
        std::shared_ptr<Shared> shared,
        allocator_type alloc) noexcept;
    Actor() = delete;
    Actor(const Actor&) = delete;
    Actor(Actor&&) = delete;
    auto operator=(const Actor&) -> Actor& = delete;
    auto operator=(Actor&&) -> Actor& = delete;

    ~Actor() final;

private:
    friend opentxs::Actor<notary::Actor, Job>;

    std::shared_ptr<api::session::internal::Notary> api_p_;
    std::shared_ptr<Shared> shared_p_;
    api::session::Notary& api_;
    Shared& shared_;
    Deque<identifier::UnitDefinition> queue_;

    auto do_shutdown() noexcept -> void;
    auto do_startup(allocator_type monotonic) noexcept -> bool;
    auto pipeline(const Work work, Message&& msg, allocator_type) noexcept
        -> void;
    auto process_queue_unitid(Message&& msg, allocator_type monotonic) noexcept
        -> void;
    auto work(allocator_type monotonic) noexcept -> bool;
};
