// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <tuple>

#include "core/Worker.hpp"
#include "interface/ui/base/List.hpp"
#include "internal/interface/ui/UI.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/WorkType.hpp"  // IWYU pragma: keep
#include "opentxs/WorkType.internal.hpp"
#include "opentxs/api/Session.hpp"
#include "opentxs/api/session/Client.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/identifier/Nym.hpp"
#include "opentxs/util/Container.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace identifier
{
class Account;
class UnitDefinition;
}  // namespace identifier

class Amount;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::ui::implementation
{
using AccountTreeList = List<
    AccountTreeExternalInterface,
    AccountTreeInternalInterface,
    AccountTreeRowID,
    AccountTreeRowInterface,
    AccountTreeRowInternal,
    AccountTreeRowBlank,
    AccountTreeSortKey,
    AccountTreePrimaryID>;

class AccountTree final : public AccountTreeList, Worker<AccountTree>
{
public:
    auto API() const noexcept -> const api::Session& final { return api_; }
    auto Debug() const noexcept -> UnallocatedCString final;
    auto Owner() const noexcept -> const identifier::Nym& final
    {
        return primary_id_;
    }

    AccountTree(
        const api::session::Client& api,
        const identifier::Nym& nymID,
        const SimpleCallback& cb) noexcept;
    AccountTree() = delete;
    AccountTree(const AccountTree&) = delete;
    AccountTree(AccountTree&&) = delete;
    auto operator=(const AccountTree&) -> AccountTree& = delete;
    auto operator=(AccountTree&&) -> AccountTree& = delete;

    ~AccountTree() final;

private:
    friend Worker<AccountTree>;

    enum class Work : OTZMQWorkType {
        shutdown = value(WorkType::Shutdown),
        custodial = value(WorkType::AccountUpdated),
        blockchain = value(WorkType::BlockchainAccountCreated),
        balance = value(WorkType::BlockchainBalance),
        init = OT_ZMQ_INIT_SIGNAL,
        statemachine = OT_ZMQ_STATE_MACHINE_SIGNAL,
    };

    using SortIndex = int;
    using AccountName = UnallocatedCString;
    using AccountID = AccountCurrencyRowID;
    using CurrencyName = UnallocatedCString;
    using AccountData =
        std::tuple<SortIndex, AccountName, AccountType, CustomData>;
    using AccountMap = UnallocatedMap<AccountID, AccountData>;
    using CurrencyData =
        std::tuple<SortIndex, CurrencyName, CustomData, AccountMap>;
    using ChildMap = UnallocatedMap<UnitType, CurrencyData>;
    using SubscribeSet = UnallocatedSet<blockchain::Type>;

    auto construct_row(
        const AccountTreeRowID& id,
        const AccountTreeSortKey& index,
        CustomData& custom) const noexcept -> RowPointer final;
    auto load_blockchain(ChildMap& out, SubscribeSet& subscribe) const noexcept
        -> void;
    auto load_blockchain_account(
        identifier::Account&& id,
        ChildMap& out,
        SubscribeSet& subscribe) const noexcept -> void;
    auto load_blockchain_account(
        blockchain::Type chain,
        ChildMap& out,
        SubscribeSet& subscribe) const noexcept -> void;
    auto load_blockchain_account(
        identifier::Account&& id,
        blockchain::Type chain,
        ChildMap& out,
        SubscribeSet& subscribe) const noexcept -> void;
    auto load_blockchain_account(
        identifier::Account&& id,
        blockchain::Type chain,
        Amount&& balance,
        ChildMap& out,
        SubscribeSet& subscribe) const noexcept -> void;
    auto load_custodial(ChildMap& out) const noexcept -> void;
    auto load_custodial_account(identifier::Account&& id, ChildMap& out)
        const noexcept -> void;
    auto load_custodial_account(
        identifier::Account&& id,
        Amount&& balance,
        ChildMap& out) const noexcept -> void;
    auto load_custodial_account(
        identifier::Account&& id,
        identifier::UnitDefinition&& contract,
        UnitType type,
        Amount&& balance,
        UnallocatedCString&& name,
        ChildMap& out) const noexcept -> void;
    auto subscribe(SubscribeSet&& chains) const noexcept -> void;

    auto add_children(ChildMap&& children) noexcept -> void;
    auto load() noexcept -> void;
    auto pipeline(Message&& in) noexcept -> void;
    auto process_blockchain(Message&& message) noexcept -> void;
    auto process_blockchain_balance(Message&& message) noexcept -> void;
    auto process_custodial(Message&& message) noexcept -> void;
    auto startup() noexcept -> void;
};
}  // namespace opentxs::ui::implementation
