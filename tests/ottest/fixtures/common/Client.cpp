// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ottest/fixtures/common/Client.hpp"  // IWYU pragma: associated

#include <opentxs/opentxs.hpp>
#include <memory>
#include <utility>

#include "internal/core/contract/ServerContract.hpp"
#include "opentxs/api/session/Wallet.internal.hpp"
#include "ottest/fixtures/common/User.hpp"

namespace ottest
{
Client_fixture::UserIndex Client_fixture::users_{};

auto Client_fixture::CleanupClient() noexcept -> void { users_.clear(); }

auto Client_fixture::CreateNym(
    const ot::api::session::Client& api,
    const ot::UnallocatedCString& name,
    const ot::crypto::SeedID& seed,
    int index) const noexcept -> const User&
{
    static auto counter = int{-1};
    const auto reason = api.Factory().PasswordPrompt(__func__);
    auto [it, added] = users_.try_emplace(
        ++counter,
        api.Crypto().Seed().Words(seed, reason),
        name,
        api.Crypto().Seed().Passphrase(seed, reason));

    opentxs::assert_true(added);

    auto& user = it->second;
    user.init(api, ot::identity::Type::individual, index);
    auto& nym = user.nym_;

    opentxs::assert_false(nullptr == nym);

    return user;
}

auto Client_fixture::ImportBip39(
    const ot::api::Session& api,
    const ot::UnallocatedCString& words) const noexcept -> ot::crypto::SeedID
{
    using SeedLang = ot::crypto::Language;
    using SeedStyle = ot::crypto::SeedStyle;
    const auto reason = api.Factory().PasswordPrompt(__func__);
    const auto id = api.Crypto().Seed().ImportSeed(
        ot_.Factory().SecretFromText(words),
        ot_.Factory().SecretFromText(""),
        SeedStyle::BIP39,
        SeedLang::en,
        reason);

    return id;
}

auto Client_fixture::ImportServerContract(
    const ot::api::session::Notary& from,
    const ot::api::session::Client& to) const noexcept -> bool
{
    const auto& id = from.ID();
    const auto server = from.Wallet().Internal().Server(id);

    if (0u == server->Version()) { return false; }

    auto bytes = ot::Space{};
    if (false == server->Serialize(ot::writer(bytes), true)) { return false; }
    const auto client = to.Wallet().Internal().Server(ot::reader(bytes));

    if (0u == client->Version()) { return false; }

    return id == client->ID();
}

auto Client_fixture::SetIntroductionServer(
    const ot::api::session::Client& on,
    const ot::api::session::Notary& to) const noexcept -> bool
{
    const auto& id = to.ID();

    if (false == ImportServerContract(to, on)) { return false; }

    const auto clientID =
        on.OTX().SetIntroductionServer(on.Wallet().Internal().Server(id));

    return id == clientID;
}

auto Client_fixture::StartClient(int index) const noexcept
    -> const ot::api::session::Client&
{
    const auto& out = ot_.StartClientSession(index);

    return out;
}
}  // namespace ottest
