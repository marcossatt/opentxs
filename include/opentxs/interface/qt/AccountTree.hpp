// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <QMetaObject>
#include <QObject>

#include "opentxs/Export.hpp"
#include "opentxs/interface/qt/Model.hpp"
#include "opentxs/interface/qt/QML.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace ui
{
namespace internal
{
struct AccountTree;
}  // namespace internal
}  // namespace ui
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::ui
{
class OPENTXS_EXPORT AccountTreeQt final : public qt::Model
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        NameRole = Qt::UserRole + 0,              // QString
        NotaryIDRole = Qt::UserRole + 1,          // QString
        NotaryNameRole = Qt::UserRole + 2,        // QString
        UnitRole = Qt::UserRole + 3,              // int (opentxs::UnitType)
        UnitNameRole = Qt::UserRole + 4,          // QString
        AccountIDRole = Qt::UserRole + 5,         // QString
        BalanceRole = Qt::UserRole + 6,           // QString
        PolarityRole = Qt::UserRole + 7,          // int (-1, 0, or 1)
        AccountTypeRole = Qt::UserRole + 8,       // int (opentxs::AccountType)
        ContractIdRole = Qt::UserRole + 9,        // QString
        UnitDescriptionRole = Qt::UserRole + 10,  // QString
    };
    Q_ENUM(Roles)
    enum Columns {
        NameColumn = 0,
    };
    Q_ENUM(Columns)

    OPENTXS_NO_EXPORT AccountTreeQt(internal::AccountTree& parent) noexcept;
    AccountTreeQt(const AccountTreeQt&) = delete;
    AccountTreeQt(AccountTreeQt&&) = delete;
    auto operator=(const AccountTreeQt&) -> AccountTreeQt& = delete;
    auto operator=(AccountTreeQt&&) -> AccountTreeQt& = delete;

    OPENTXS_NO_EXPORT ~AccountTreeQt() final;

private:
    struct Imp;

    Imp* imp_;
};
}  // namespace opentxs::ui
