// Copyright (c) 2010-2023 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <opentxs/opentxs.hpp>
#include <memory>

#include "ottest/fixtures/common/OneClientSession.hpp"

namespace ottest
{
namespace ot = opentxs;

class OPENTXS_EXPORT ContactSection : public OneClientSession
{
public:
    static constexpr auto local_attr_ = {
        ot::identity::wot::claim::Attribute::Local};
    static constexpr auto active_attr_ = {
        opentxs::identity::wot::claim::Attribute::Active};

    const opentxs::identifier::Nym nym_id_;
    const ot::identity::wot::claim::Section contact_section_;
    const std::shared_ptr<ot::identity::wot::claim::Group> contact_group_;
    const std::shared_ptr<ot::identity::wot::claim::Item> active_contact_item_;

    ContactSection();
};
}  // namespace ottest
