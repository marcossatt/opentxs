// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "internal/blockchain/crypto/PaymentCode.hpp"  // IWYU pragma: associated

#include "opentxs/api/Session.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/blockchain/crypto/Types.internal.hpp"
#include "opentxs/core/ByteArray.hpp"
#include "opentxs/core/PaymentCode.hpp"
#include "opentxs/identifier/Account.hpp"
#include "opentxs/identifier/AccountSubtype.hpp"  // IWYU pragma: keep
#include "opentxs/identifier/Nym.hpp"
#include "opentxs/identifier/Types.hpp"

namespace opentxs::blockchain::crypto::internal
{
auto PaymentCode::AddIncomingNotification(
    const block::TransactionHash&) const noexcept -> bool
{
    return {};
}

auto PaymentCode::AddNotification(const block::TransactionHash&) const noexcept
    -> bool
{
    return {};
}

auto PaymentCode::Blank() noexcept -> PaymentCode&
{
    static auto blank = PaymentCode{};

    return blank;
}

auto PaymentCode::GetID(
    const api::Session& api,
    const crypto::Target target,
    const opentxs::PaymentCode& local,
    const opentxs::PaymentCode& remote) noexcept -> identifier::Account
{
    auto out = identifier::Account{};
    auto preimage = api.Factory().Data();
    serialize(target, preimage);
    preimage.Concatenate(local.ID().Bytes());
    preimage.Concatenate(remote.ID().Bytes());
    using enum identifier::AccountSubtype;

    return api.Factory().AccountIDFromPreimage(
        preimage.Bytes(), blockchain_subaccount);
}

auto PaymentCode::IncomingNotificationCount() const noexcept -> std::size_t
{
    return {};
}

auto PaymentCode::Local() const noexcept -> const opentxs::PaymentCode&
{
    static const auto blank = opentxs::PaymentCode{};

    return blank;
}

auto PaymentCode::NotificationCount() const noexcept
    -> std::pair<std::size_t, std::size_t>
{
    return {};
}

auto PaymentCode::OutgoingNotificationCount() const noexcept -> std::size_t
{
    return {};
}

auto PaymentCode::Remote() const noexcept -> const opentxs::PaymentCode&
{
    return Local();
}

auto PaymentCode::ReorgNotification(
    const block::TransactionHash&) const noexcept -> bool
{
    return {};
}
}  // namespace opentxs::blockchain::crypto::internal
