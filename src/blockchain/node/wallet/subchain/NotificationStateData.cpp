// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "blockchain/node/wallet/subchain/NotificationStateData.hpp"  // IWYU pragma: associated

#include <opentxs/protobuf/HDPath.pb.h>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string_view>
#include <utility>

#include "internal/blockchain/crypto/Deterministic.hpp"
#include "internal/blockchain/crypto/Notification.hpp"
#include "internal/blockchain/crypto/PaymentCode.hpp"
#include "internal/blockchain/crypto/Subaccount.hpp"
#include "internal/blockchain/node/wallet/subchain/statemachine/Index.hpp"
#include "internal/core/PaymentCode.hpp"
#include "internal/util/P0330.hpp"
#include "opentxs/Types.hpp"
#include "opentxs/api/Factory.internal.hpp"
#include "opentxs/api/Session.hpp"
#include "opentxs/api/Session.internal.hpp"
#include "opentxs/api/crypto/Blockchain.hpp"
#include "opentxs/api/session/Contacts.hpp"
#include "opentxs/api/session/Crypto.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/api/session/Wallet.hpp"
#include "opentxs/blockchain/Types.hpp"
#include "opentxs/blockchain/block/Block.hpp"
#include "opentxs/blockchain/block/TransactionHash.hpp"
#include "opentxs/blockchain/crypto/Notification.hpp"
#include "opentxs/blockchain/crypto/PaymentCode.hpp"
#include "opentxs/blockchain/crypto/Subchain.hpp"  // IWYU pragma: keep
#include "opentxs/blockchain/protocol/bitcoin/base/block/Output.hpp"
#include "opentxs/blockchain/protocol/bitcoin/base/block/Script.hpp"
#include "opentxs/blockchain/protocol/bitcoin/base/block/Transaction.hpp"
#include "opentxs/core/Contact.hpp"
#include "opentxs/crypto/asymmetric/key/HD.hpp"
#include "opentxs/identifier/Generic.hpp"
#include "opentxs/identifier/HDSeed.hpp"
#include "opentxs/util/Container.hpp"
#include "opentxs/util/Log.hpp"
#include "opentxs/util/NymEditor.hpp"
#include "opentxs/util/PasswordPrompt.hpp"
#include "util/ScopeGuard.hpp"

namespace opentxs::blockchain::node::wallet
{
NotificationStateData::NotificationStateData(
    Reorg& reorg,
    crypto::Notification& subaccount,
    const opentxs::PaymentCode& code,
    std::shared_ptr<const api::internal::Session> api,
    std::shared_ptr<const node::Manager> node,
    crypto::Subchain subchain,
    std::string_view fromParent,
    network::zeromq::BatchID batch,
    allocator_type alloc) noexcept
    : SubchainStateData(
          reorg,
          subaccount,
          std::move(api),
          std::move(node),
          std::move(subchain),
          std::move(fromParent),
          std::move(batch),
          std::move(alloc))
    , path_(subaccount.Internal().asNotification().Path())
    , pc_(code)
    , pc_display_(pc_.asBase58(), get_allocator())
    , pc_secret_(pc_)
    , cache_(get_allocator())
{
}

auto NotificationStateData::CheckCache(const std::size_t, FinishedCallback cb)
    const noexcept -> void
{
    if (cb) {
        cache_.modify([&](auto& data) {
            cb(data);
            data.clear();
        });
    }
}

auto NotificationStateData::do_startup(allocator_type monotonic) noexcept
    -> bool
{
    if (SubchainStateData::do_startup(monotonic)) { return true; }

    auto reason =
        api_.Factory().PasswordPrompt("Verifying / updating contact data");
    auto mNym = api_.Wallet().mutable_Nym(owner_, reason);
    const auto type = blockchain_to_unit(chain_);
    const auto existing = mNym.PaymentCode(type);
    const auto expected = UnallocatedCString{pc_display_};

    if (existing != expected) {
        mNym.AddPaymentCode(expected, type, existing.empty(), true, reason);
    }

    return false;
}

auto NotificationStateData::get_index(
    const std::shared_ptr<const SubchainStateData>& me) const noexcept -> void
{
    Index::NotificationFactory(me, pc_).Init();
}

auto NotificationStateData::handle_block_matches(
    const block::Position& position,
    const block::Matches& confirmed,
    const Log& log,
    block::Block& block,
    allocator_type) const noexcept -> void
{
    const auto& [utxo, general] = confirmed;
    log()(general.size())(" confirmed matches for ")(pc_)(" on ")(print(chain_))
        .Flush();
    auto post = ScopeGuard{[&] {
        cache_.modify([&](auto& vector) { vector.emplace_back(position); });
    }};

    if (general.empty()) { return; }

    const auto reason = api_.Factory().PasswordPrompt(
        "Decoding confirmed payment code notification transaction");

    for (const auto& match : general) {
        const auto& [txid, elementID] = match;
        const auto& [version, subchainID] = elementID;
        LogConsole()()(print(chain_))(" transaction ")
            .asHex(txid)(" contains a version ")(version)(" notification for ")(
                pc_)
            .Flush();
        const auto tx = block.FindByID(txid);
        process(match, tx, true, reason);
    }
}

auto NotificationStateData::handle_mempool_match(
    const block::Matches& matches,
    block::Transaction tx,
    allocator_type) const noexcept -> void
{
    const auto& log = log_;
    const auto& [utxo, general] = matches;

    if (general.empty()) { return; }

    const auto reason = api_.Factory().PasswordPrompt(
        "Decoding unconfirmed payment code notification transaction");

    for (const auto& match : general) {
        const auto& [txid, elementID] = match;
        const auto& [version, subchainID] = elementID;
        log()(print(chain_))(" mempool transaction ")
            .asHex(txid)(" contains a version ")(version)(" notification for ")(
                pc_)
            .Flush();
        process(match, tx, false, reason);
    }
}

auto NotificationStateData::init_contacts(allocator_type monotonic) noexcept
    -> void
{
    const auto& api = api_.Internal().Contacts();
    const auto contacts = [&] {
        const auto data = api.ContactList();
        auto out = Vector<identifier::Generic>{monotonic};
        out.reserve(data.size());
        out.clear();
        std::ranges::transform(
            data, std::back_inserter(out), [this](const auto& item) {
                return api_.Factory().IdentifierFromBase58(item.first);
            });

        return out;
    }();

    for (const auto& id : contacts) {
        const auto contact = api.Contact(id);

        assert_false(nullptr == contact);

        for (const auto& remote : contact->PaymentCodes(monotonic)) {
            const auto prompt = [&] {
                // TODO use allocator when we upgrade to c++20
                auto out = std::stringstream{};
                out << "Generate keys for a ";
                out << print(chain_);
                out << " payment code account for ";
                out << api.ContactName(id);

                return out;
            }();
            const auto reason = api_.Factory().PasswordPrompt(prompt.str());
            static const auto blank = block::TransactionHash{};
            process(blank, remote, false, reason);
        }
    }
}

auto NotificationStateData::init_keys(
    opentxs::PaymentCode& pc,
    const PasswordPrompt& reason) const noexcept -> void
{
    const auto& key = pc.Key();

    assert_true(key.IsValid());

    if (key.HasPrivate()) { return; }

    const auto seed = api_.Factory().Internal().SeedID(path_.seed());
    const auto upgraded =
        pc.Internal().AddPrivateKeys(seed, *path_.child().rbegin(), reason);

    assert_true(upgraded);
}

auto NotificationStateData::process(
    const block::Match match,
    const block::Transaction& tx,
    bool confirmed,
    const PasswordPrompt& reason) const noexcept -> void
{
    const auto& log = log_;
    const auto& [txid, elementID] = match;
    const auto& [version, subchainID] = elementID;

    for (const auto& output : tx.asBitcoin().Outputs()) {
        const auto& script = output.Script();

        if (script.IsNotification(version, pc_)) {
            const auto elements = [&] {
                auto out = UnallocatedVector<Space>{};

                for (auto i = 0_uz; i < 3_uz; ++i) {
                    const auto view = script.MultisigPubkey(i);

                    assert_true(view.has_value());

                    const auto& value = view.value();
                    const auto* start =
                        reinterpret_cast<const std::byte*>(value.data());
                    const auto* stop = std::next(start, value.size());
                    out.emplace_back(start, stop);
                }

                return out;
            }();
            const auto sender = [&, v = version] {
                auto out = opentxs::PaymentCode{};  // TODO monotonic allocator
                pc_secret_.modify([&](auto& pc) {
                    init_keys(pc, reason);
                    out = pc.DecodeNotificationElements(v, elements, reason);
                });

                return out;
            }();

            if (0u == sender.Version()) { continue; }

            log()("decoded incoming notification from ")(sender)(" on ")(
                print(chain_))(" for ")(pc_)
                .Flush();
            process(tx.ID(), sender, confirmed, reason);
        }
    }
}

auto NotificationStateData::process(
    const block::TransactionHash& tx,
    const opentxs::PaymentCode& remote,
    bool confirmed,
    const PasswordPrompt& reason) const noexcept -> void
{
    const auto& log = log_;

    if (remote == pc_) { return; }

    const auto& account = api_.Crypto().Blockchain().LoadOrCreateSubaccount(
        owner_, remote, chain_, reason);

    if (confirmed) {
        account.Internal()
            .asDeterministic()
            .asPaymentCode()
            .AddIncomingNotification(tx);
    }

    log()("Created or verified account ")(account.ID(), api_.Crypto())(" for ")(
        remote)
        .Flush();
}

auto NotificationStateData::work(allocator_type monotonic) noexcept -> bool
{
    auto again = SubchainStateData::work(monotonic);
    init_contacts(monotonic);

    return again;
}

NotificationStateData::~NotificationStateData() = default;
}  // namespace opentxs::blockchain::node::wallet
