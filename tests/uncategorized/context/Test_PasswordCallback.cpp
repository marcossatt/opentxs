// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <gtest/gtest.h>
#include <opentxs/opentxs.hpp>
#include <memory>
#include <string_view>

#include "internal/util/DeferredConstruction.hpp"
#include "ottest/fixtures/common/LowLevel.hpp"
#include "ottest/fixtures/common/PasswordCallback.hpp"

namespace ottest
{
using namespace std::literals;

ot::DeferredConstruction<ot::CString> profile_id_{};
ot::DeferredConstruction<ot::identifier::Nym> nym_id_{};
static constexpr auto preimage_ =
    "The quick brown fox jumped over the lazy dog"sv;

TEST_F(LowLevel, create_nym)
{
    profile_id_.set_value(ot::CString{ot_.ProfileId()});
    password_.SetPassword(PasswordCallback::password_1_);
    const auto& api = ot_.StartClientSession(0);
    const auto reason = api.Factory().PasswordPrompt(__func__);
    const auto nym = api.Wallet().Nym(reason);

    ASSERT_TRUE(nym);

    nym_id_.set_value(nym->ID());

    EXPECT_FALSE(nym_id_.get().empty());
}

TEST_F(LowLevel, sign_contract_correct_password)
{
    EXPECT_EQ(profile_id_.get(), ot_.ProfileId());

    password_.SetPassword(PasswordCallback::password_1_);
    const auto& api = ot_.StartClientSession(0);
    const auto nym = api.Wallet().Nym(nym_id_.get());

    ASSERT_TRUE(nym);
    EXPECT_EQ(nym->ID(), nym_id_.get());

    // Have the Nym sign something here, which should succeed.
    auto reason = api.Factory().PasswordPrompt(__func__);
    const auto pc = nym->PaymentCodeSecret(reason);
    const auto preimage = opentxs::ByteArray{preimage_};
    auto sig = opentxs::ByteArray{};

    EXPECT_TRUE(pc.Sign(preimage, sig, reason));
}

TEST_F(LowLevel, sign_contract_wrong_password)
{
    EXPECT_EQ(profile_id_.get(), ot_.ProfileId());

    password_.SetPassword(PasswordCallback::password_2_);
    const auto& api = ot_.StartClientSession(0);
    const auto nym = api.Wallet().Nym(nym_id_.get());

    ASSERT_TRUE(nym);
    EXPECT_EQ(nym->ID(), nym_id_.get());

    // Have the Nym sign something here, which should fail since we deliberately
    // used the wrong password.
    auto reason = api.Factory().PasswordPrompt(__func__);
    const auto pc = nym->PaymentCodeSecret(reason);
    const auto preimage = opentxs::ByteArray{preimage_};
    auto sig = opentxs::ByteArray{};

    EXPECT_FALSE(pc.Sign(preimage, sig, reason));
}
}  // namespace ottest
