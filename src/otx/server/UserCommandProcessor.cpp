// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_forward_declare opentxs::OTAgent

#include "otx/server/UserCommandProcessor.hpp"  // IWYU pragma: associated

#include <opentxs/protobuf/AsymmetricKey.pb.h>
#include <opentxs/protobuf/BasketItem.pb.h>
#include <opentxs/protobuf/BasketParams.pb.h>
#include <opentxs/protobuf/Context.pb.h>
#include <opentxs/protobuf/Nym.pb.h>
#include <opentxs/protobuf/ServerContract.pb.h>
#include <opentxs/protobuf/UnitDefinition.pb.h>
#include <filesystem>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>

#include "internal/api/session/Storage.hpp"
#include "internal/core/Armored.hpp"
#include "internal/core/String.hpp"
#include "internal/core/contract/BasketContract.hpp"
#include "internal/core/contract/Contract.hpp"
#include "internal/core/contract/ServerContract.hpp"
#include "internal/core/contract/Unit.hpp"
#include "internal/identity/Nym.hpp"
#include "internal/otx/blind/Mint.hpp"
#include "internal/otx/common/Account.hpp"
#include "internal/otx/common/Item.hpp"
#include "internal/otx/common/Ledger.hpp"
#include "internal/otx/common/Message.hpp"
#include "internal/otx/common/NumList.hpp"
#include "internal/otx/common/NymFile.hpp"
#include "internal/otx/common/OTTransaction.hpp"
#include "internal/otx/common/cron/OTCron.hpp"
#include "internal/otx/common/cron/OTCronItem.hpp"
#include "internal/otx/common/script/OTScriptable.hpp"
#include "internal/otx/common/trade/OTMarket.hpp"
#include "internal/otx/consensus/Client.hpp"
#include "internal/otx/consensus/Consensus.hpp"
#include "internal/otx/smartcontract/OTParty.hpp"
#include "internal/otx/smartcontract/OTSmartContract.hpp"
#include "internal/util/Editor.hpp"
#include "internal/util/Pimpl.hpp"
#include "opentxs/api/Factory.internal.hpp"
#include "opentxs/api/Session.hpp"
#include "opentxs/api/Settings.hpp"
#include "opentxs/api/Settings.internal.hpp"
#include "opentxs/api/session/Crypto.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/api/session/Factory.internal.hpp"
#include "opentxs/api/session/Notary.hpp"
#include "opentxs/api/session/Storage.hpp"
#include "opentxs/api/session/Wallet.hpp"
#include "opentxs/api/session/Wallet.internal.hpp"
#include "opentxs/contract/ContractType.hpp"  // IWYU pragma: keep
#include "opentxs/contract/Types.hpp"
#include "opentxs/contract/UnitDefinitionType.hpp"  // IWYU pragma: keep
#include "opentxs/core/Amount.hpp"
#include "opentxs/core/ByteArray.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/crypto/asymmetric/Key.hpp"
#include "opentxs/identifier/Account.hpp"
#include "opentxs/identifier/Generic.hpp"
#include "opentxs/identifier/Notary.hpp"
#include "opentxs/identifier/Nym.hpp"
#include "opentxs/identifier/UnitDefinition.hpp"
#include "opentxs/identity/Nym.hpp"
#include "opentxs/identity/wot/Claim.hpp"
#include "opentxs/identity/wot/claim/Attribute.hpp"  // IWYU pragma: keep
#include "opentxs/identity/wot/claim/Types.hpp"
#include "opentxs/otx/Types.internal.hpp"
#include "opentxs/otx/blind/Mint.hpp"  // IWYU pragma: keep
#include "opentxs/protobuf/Types.internal.hpp"
#include "opentxs/protobuf/Types.internal.tpp"
#include "opentxs/protobuf/syntax/Types.internal.tpp"
#include "opentxs/protobuf/syntax/UnitDefinition.hpp"
#include "opentxs/util/Container.hpp"
#include "opentxs/util/Log.hpp"
#include "opentxs/util/NymEditor.hpp"
#include "opentxs/util/PasswordPrompt.hpp"
#include "otx/common/OTStorage.hpp"
#include "otx/server/Macros.hpp"
#include "otx/server/MainFile.hpp"
#include "otx/server/Notary.hpp"
#include "otx/server/ReplyMessage.hpp"
#include "otx/server/Server.hpp"
#include "otx/server/ServerSettings.hpp"
#include "otx/server/Transactor.hpp"

namespace opentxs
{
constexpr auto MAX_UNUSED_NUMBERS = 100;
constexpr auto ISSUE_NUMBER_BATCH = 100;
constexpr auto NYMBOX_DEPTH = 0;
constexpr auto INBOX_DEPTH = 1;
constexpr auto OUTBOX_DEPTH = 2;
}  // namespace opentxs

namespace opentxs::server
{
UserCommandProcessor::FinalizeResponse::FinalizeResponse(
    const api::Session& api,
    const identity::Nym& nym,
    ReplyMessage& reply,
    Ledger& ledger)
    : api_{api}
    , nym_(nym)
    , reply_(reply)
    , ledger_(ledger)
    , response_()
{
}

auto UserCommandProcessor::FinalizeResponse::AddResponse(
    std::shared_ptr<OTTransaction> response) -> std::shared_ptr<OTTransaction>&
{
    if (false == ledger_.AddTransaction(response)) {
        LogError()()("Unable to add response transaction to response ledger.")
            .Flush();

        LogAbort()().Abort();
    }

    return response_.emplace_back(response);
}

UserCommandProcessor::FinalizeResponse::~FinalizeResponse()
{
    auto reason = api_.Factory().PasswordPrompt(__func__);

    if (response_.empty()) {
        auto transaction = api_.Factory().Internal().Session().Transaction(
            ledger_,
            otx::transactionType::error_state,
            otx::originType::not_applicable,
            0);
        auto response =
            AddResponse(std::shared_ptr<OTTransaction>(transaction.release()));

        if (false == response->SignContract(nym_, reason)) {
            LogError()()("Failed to sign response transaction.").Flush();

            LogAbort()().Abort();
        }

        if (false == response->SaveContract()) {
            LogError()()("Failed to serialize response transaction.").Flush();

            LogAbort()().Abort();
        }
    }

    ledger_.ReleaseSignatures();

    if (false == ledger_.SignContract(nym_, reason)) {
        LogError()()("Failed to sign response ledger.").Flush();

        LogAbort()().Abort();
    }

    if (false == ledger_.SaveContract()) {
        LogError()()("Failed to serialize response ledger.").Flush();

        LogAbort()().Abort();
    }

    reply_.SetPayload(String::Factory(ledger_));
    LogDetail()()(reply_.Context().AvailableNumbers())(" numbers available.")
        .Flush();
    LogDetail()()(reply_.Context().IssuedNumbers({}))(" numbers issued.")
        .Flush();
}

UserCommandProcessor::UserCommandProcessor(
    Server& server,
    const PasswordPrompt& reason,
    const opentxs::api::session::Notary& manager)
    : server_(server)
    , reason_(reason)
    , api_(manager)
{
}

auto UserCommandProcessor::add_numbers_to_nymbox(
    const TransactionNumber transactionNumber,
    const NumList& newNumbers,
    bool& savedNymbox,
    Ledger& nymbox,
    identifier::Generic& nymboxHash) const -> bool
{
    if (false == nymbox.LoadNymbox()) {
        LogError()()("Error loading nymbox.").Flush();

        return false;
    }

    bool success = true;
    success &= nymbox.VerifyContractID();

    if (success) { success &= nymbox.VerifySignature(server_.GetServerNym()); }

    if (false == success) {
        LogError()()("Error veryfying nymbox.").Flush();

        return false;
    }

    // Note: I decided against adding newly-requested transaction
    // numbers to existing otx::transactionType::blanks in the Nymbox.
    // Why not? Because once the user downloads the Box Receipt, he will
    // think he has it already, when the Server meanwhile
    // has a new version of that same Box Receipt. But the user will
    // never re-download it if he believes that he already has
    // it.
    // Since the transaction can contain 10, 20, or 50 transaction
    // numbers now, we don't NEED to be able to combine them
    // anyway, since the problem is still effectively solved.

    auto transaction{api_.Factory().Internal().Session().Transaction(
        nymbox,
        otx::transactionType::blank,
        otx::originType::not_applicable,
        transactionNumber)};

    assert_false(nullptr == transaction);

    if (transaction) {
        transaction->AddNumbersToTransaction(newNumbers);
        transaction->SignContract(server_.GetServerNym(), reason_);
        transaction->SaveContract();
        // Any inbox/nymbox/outbox ledger will only itself contain
        // abbreviated versions of the receipts, including their hashes.
        //
        // The rest is stored separately, in the box receipt, which is
        // created whenever a receipt is added to a box, and deleted after a
        // receipt is removed from a box.
        transaction->SaveBoxReceipt(nymbox);

        const std::shared_ptr<OTTransaction> ptransaction{
            transaction.release()};
        nymbox.AddTransaction(ptransaction);
        nymbox.ReleaseSignatures();
        nymbox.SignContract(server_.GetServerNym(), reason_);
        nymbox.SaveContract();
        savedNymbox = nymbox.SaveNymbox(nymboxHash);
    } else {
        nymbox.CalculateNymboxHash(nymboxHash);
    }

    return success;
}

// ACKNOWLEDGMENTS OF REPLIES ALREADY RECEIVED (FOR OPTIMIZATION.)

// On the client side, whenever the client is DEFINITELY made aware of the
// existence of a server reply, he adds its request number to this list,
// which is sent along with all client-side requests to the server. The
// server reads the list on the incoming client message (and it uses these
// same functions to store its own internal list.) If the # already appears
// on its internal list, then it does nothing. Otherwise, it loads up the
// Nymbox and removes the replyNotice, and then adds the # to its internal
// list. For any numbers on the internal list but NOT on the client's list,
// the server removes from the internal list. (The client removed them when
// it saw the server's internal list, which the server sends with its
// replies.)
//
// This entire protocol, densely described, is unnecessary for OT to
// function, but is great for optimization, as it enables OT to avoid
// downloading all Box Receipts containing replyNotices, as long as the
// original reply was properly received when the request was originally sent
// (which is MOST of the time...) Thus we can eliminate most replyNotice
// downloads, and likely a large % of box receipt downloads as well.
void UserCommandProcessor::check_acknowledgements(ReplyMessage& reply) const
{
    auto& context = reply.Context();
    // The server reads the list of acknowledged replies from the incoming
    // client message... If we add any acknowledged replies to the server-side
    // list, we will want to save (at the end.)
    auto numlist_ack_reply = reply.Acknowledged();
    const auto& nymID = context.RemoteNym().ID();
    auto nymbox{api_.Factory().Internal().Session().Ledger(
        nymID, nymID, context.Notary())};

    assert_false(nullptr == nymbox);

    if (nymbox->LoadNymbox() &&
        nymbox->VerifySignature(server_.GetServerNym())) {
        bool bIsDirtyNymbox = false;

        for (const auto& it : numlist_ack_reply) {
            const std::int64_t lRequestNum = it;
            // If the # already appears on its internal list, then it does
            // nothing. (It must have already done
            // whatever it needed to do, since it already has the number
            // recorded as acknowledged.)
            //
            // Otherwise, if the # does NOT already appear on server's
            // internal list, then it loads up the
            // Nymbox and removes the replyNotice, and then adds the # to
            // its internal list for safe-keeping.
            if (!context.VerifyAcknowledgedNumber(lRequestNum)) {
                // Verify whether a replyNotice exists in the Nymbox, with
                // that lRequestNum
                auto pReplyNotice = nymbox->GetReplyNotice(lRequestNum);

                if (pReplyNotice) {
                    // If so, remove it...
                    const bool bDeleted =
                        pReplyNotice->DeleteBoxReceipt(*nymbox);
                    const bool bRemoved = nymbox->RemoveTransaction(
                        pReplyNotice->GetTransactionNum());  // deletes
                    pReplyNotice = nullptr;
                    // (pReplyNotice is deleted, below this point,
                    // automatically by the above Remove call.)

                    if (!bDeleted || !bRemoved) {
                        LogError()()("Failed trying "
                                     "to delete a box receipt, or "
                                     "while removing its stub from the Nymbox.")
                            .Flush();
                    }

                    if (bRemoved) { bIsDirtyNymbox = true; }
                }

                context.AddAcknowledgedNumber(lRequestNum);
            }
        }

        if (bIsDirtyNymbox) {
            auto nymboxHash = identifier::Generic{};
            nymbox->ReleaseSignatures();
            nymbox->SignContract(server_.GetServerNym(), reason_);
            nymbox->SaveContract();
            nymbox->SaveNymbox(nymboxHash);
        }
    }

    context.FinishAcknowledgements(numlist_ack_reply);
    reply.SetAcknowledgments(context);
}

auto UserCommandProcessor::check_client_isnt_server(
    const identifier::Nym& nymID,
    const identity::Nym& serverNym) -> bool
{
    const auto& serverNymID = serverNym.ID();
    const bool bNymIsServerNym = serverNymID == nymID;

    if (bNymIsServerNym) {
        LogError()()("Server nym is not allowed to act as a client.").Flush();

        return false;
    }

    return true;
}

auto UserCommandProcessor::check_client_nym(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();
    const auto& nym = reply.Context().RemoteNym();

    if (false == msgIn.VerifySignature(nym)) {
        LogError()()("Unable to verify message signature.").Flush();

        return false;
    }

    LogVerbose()()("Message signature verification successful.").Flush();

    return true;
}

auto UserCommandProcessor::check_message_notary(
    const api::Crypto& crypto,
    const identifier::Notary& notaryID,
    const identifier::Generic& realNotaryID) -> bool
{
    // Validate the server ID, to keep users from intercepting a valid requst
    // and sending it to the wrong server.
    if (false == (realNotaryID == notaryID)) {
        LogError()()("Invalid server ID (")(notaryID, crypto)(
            ") sent in command request.")
            .Flush();

        return false;
    }

    LogTrace()()("Received valid Notary ID with command request.").Flush();

    return true;
}

auto UserCommandProcessor::check_ping_notary(const Message& msgIn) const -> bool
{
    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_check_notary_id);

    const auto serialized = protobuf::StringToProto<protobuf::AsymmetricKey>(
        api_.Crypto(), msgIn.nym_public_key_);
    auto nymAuthentKey =
        api_.Factory().Internal().Session().AsymmetricKey(serialized);

    if (false == bool(nymAuthentKey.IsValid())) { return false; }

    // Not all contracts are signed with the authentication key, but messages
    // are.
    if (!msgIn.VerifyWithKey(nymAuthentKey)) {
        LogError()()("Signature verification failed!").Flush();

        return false;
    }

    LogDebug()()("Signature verified! The message WAS signed by the Private "
                 "Authentication Key inside the message.")
        .Flush();

    return true;
}

auto UserCommandProcessor::check_request_number(
    const Message& msgIn,
    const RequestNumber& correctNumber) const -> bool
{
    const RequestNumber messageNumber = msgIn.request_num_->ToLong();

    if (correctNumber != messageNumber) {
        LogError()()("Request number sent in this message "
                     "(")(messageNumber)(") does not match the one in the "
                                         "context (")(correctNumber)(").")
            .Flush();

        return false;
    }

    LogDebug()()("Request number in this message ")(
        messageNumber)(" does match the "
                       "one in the "
                       "context.")
        .Flush();

    return true;
}

auto UserCommandProcessor::check_server_lock(
    const api::Session& api,
    const identifier::Nym& nymID) -> bool
{
    if (false == ServerSettings::_admin_server_locked) { return true; }

    if (isAdmin(api, nymID)) { return true; }

    LogError()()("Nym ")(nymID, api.Crypto())(
        " failed attempt to message the server, while server is in "
        "**LOCK DOWN MODE**.")
        .Flush();

    return false;
}

auto UserCommandProcessor::check_usage_credits(ReplyMessage& reply) const
    -> bool
{
    const auto nymfile = reply.Context().Internal().Nymfile(reason_);

    assert_false(nullptr == nymfile);

    const bool creditsRequired = ServerSettings::_admin_usage_credits;
    const bool needsCredits = nymfile->GetUsageCredits() >= 0;
    const bool checkCredits = creditsRequired && needsCredits &&
                              (false == isAdmin(server_.API(), nymfile->ID()));

    if (checkCredits) {
        auto nymFile = reply.Context().Internal().mutable_Nymfile(reason_);
        const auto& credits = nymFile.get().GetUsageCredits();

        if (0 == credits) {
            LogError()()("Nym ")(nymFile.get().ID(), api_.Crypto())(
                " is out of usage credits.")
                .Flush();

            return false;
        }

        nymFile.get().SetUsageCredits(credits - 1);
    }

    return true;
}

auto UserCommandProcessor::cmd_add_claim(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_request_admin);

    reply.SetSuccess(true);
    const auto& context = reply.Context();
    const auto& nymID = context.RemoteNym().ID();
    const auto requestingNym = String::Factory(nymID, api_.Crypto());
    const std::uint32_t section = msgIn.nym_id2_->ToUint();
    const std::uint32_t type = msgIn.instrument_definition_id_->ToUint();
    const UnallocatedCString value = msgIn.acct_id_->Get();
    const bool primary = msgIn.bool_;
    auto claim = server_.API().Factory().Claim(
        *context.Signer(),
        static_cast<identity::wot::claim::SectionType>(section),
        static_cast<identity::wot::claim::ClaimType>(type),
        value,
        {});
    using enum identity::wot::claim::Attribute;

    if (primary) { claim.Add(Primary); }

    claim.Add(Active);
    auto overrideNym = String::Factory();
    bool keyExists = false;
    server_.API().Config().Internal().Check_str(
        String::Factory("permissions"),
        String::Factory("override_nym_id"),
        overrideNym,
        keyExists);
    const bool haveAdmin = keyExists && overrideNym->Exists();
    const bool isAdmin = haveAdmin && (overrideNym->Compare(requestingNym));

    if (isAdmin) {
        auto nym = server_.API().Wallet().mutable_Nym(
            server_.GetServerNym().ID(), reason_);
        reply.SetBool(nym.AddClaim(claim, reason_));
    }

    return true;
}

auto UserCommandProcessor::cmd_check_nym(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();
    const auto& targetNym = msgIn.nym_id2_;
    reply.SetTargetNym(targetNym);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_check_nym);

    reply.SetSuccess(true);
    auto nym = server_.API().Wallet().Nym(
        server_.API().Factory().NymIDFromBase58(targetNym->Bytes()));

    if (nym) {
        auto publicNym = protobuf::Nym{};
        if (false == nym->Internal().Serialize(publicNym)) {
            LogError()()("Failed to serialize nym ")(targetNym.get()).Flush();
            reply.SetBool(false);
        } else {
            reply.SetPayload(api_.Factory().Internal().Data(publicNym));
            reply.SetBool(true);
        }
    } else {
        LogError()()("Nym ")(targetNym.get())(" does not exist.").Flush();
        reply.SetBool(false);
    }

    return true;
}

// If the client wants to delete an asset account, the server will allow it...
// ...IF: the Inbox and Outbox are both EMPTY. AND the Balance must be empty as
// well!
auto UserCommandProcessor::cmd_delete_asset_account(ReplyMessage& reply) const
    -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetAccount(msgIn.acct_id_);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_del_asset_acct);

    const auto accountID =
        server_.API().Factory().AccountIDFromBase58(msgIn.acct_id_->Bytes());
    const auto& context = reply.Context();
    const auto& serverNym = *context.Signer();
    auto account =
        server_.API().Wallet().Internal().mutable_Account(accountID, reason_);

    if (false == bool(account)) {
        LogError()()("Error loading account ")(accountID, api_.Crypto())(".")
            .Flush();

        return false;
    }

    const auto balance = account.get().GetBalance();

    if (balance != 0) {
        const auto unittype =
            server_.API().Storage().Internal().AccountUnit(accountID);
        LogError()()("Unable to delete account ")(accountID, api_.Crypto())(
            " with non-zero balance ")(balance, unittype)(".")
            .Flush();

        return false;
    }

    std::unique_ptr<Ledger> inbox(account.get().LoadInbox(serverNym));
    std::unique_ptr<Ledger> outbox(account.get().LoadOutbox(serverNym));

    if (false == bool(inbox)) {
        LogError()()("Error loading inbox for account ")(
            accountID, api_.Crypto())(".")
            .Flush();

        return false;
    }

    if (false == bool(outbox)) {
        LogError()()("Error loading outbox for account ")(
            accountID, api_.Crypto())(".")
            .Flush();

        return false;
    }

    const auto inboxTransactions = inbox->GetTransactionCount();
    const auto outboxTransactions = outbox->GetTransactionCount();

    if (inboxTransactions > 0) {
        LogError()()("Unable to delete account ")(accountID, api_.Crypto())(
            " with ")(inboxTransactions)(" open inbox transactions.")
            .Flush();

        return false;
    }

    if (outboxTransactions > 0) {
        LogError()()("Unable to delete account ")(accountID, api_.Crypto())(
            " with ")(inboxTransactions)(" open outbox transactions.")
            .Flush();

        return false;
    }

    const auto& contractID = account.get().GetInstrumentDefinitionID();

    try {
        const auto contract =
            server_.API().Wallet().Internal().UnitDefinition(contractID);

        if (contract->Type() == contract::UnitDefinitionType::Security) {
            if (false == contract->EraseAccountRecord(
                             server_.API().DataFolder().string(), accountID)) {
                LogError()()("Unable to delete account record ")(
                    contractID, api_.Crypto())(".")
                    .Flush();

                return false;
            }
        }
    } catch (...) {
        LogError()()("Unable to load unit definition ")(
            contractID, api_.Crypto())
            .Flush();

        return false;
    }

    reply.SetSuccess(true);
    auto nymfile = server_.API().Wallet().Internal().mutable_Nymfile(
        reply.Context().RemoteNym().ID(), reason_);
    auto& theAccountSet = nymfile.get().GetSetAssetAccounts();
    theAccountSet.erase(String::Factory(accountID, api_.Crypto())->Get());
    account.Release();
    server_.API().Wallet().DeleteAccount(accountID);
    reply.DropToNymbox(false);

    return true;
}

// If a user requests to delete his own Nym, the server will allow it.
// IF: If the transaction numbers are all closable (available on both lists).
// AND if the Nymbox is empty. AND if there are no cron items open, AND if
// there are no asset accounts! (Delete them / Close them all FIRST! Or this
// fails.)
auto UserCommandProcessor::cmd_delete_user(ReplyMessage& reply) const -> bool
{
    auto nymfile = server_.API().Wallet().Internal().mutable_Nymfile(
        reply.Context().RemoteNym().ID(), reason_);
    const auto& msgIn = reply.Original();
    auto& context = reply.Context();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_del_user_acct);

    const auto& nymID = context.RemoteNym().ID();
    const auto& server = context.Notary();
    const auto& serverNym = *context.Signer();
    auto nymbox = load_nymbox(nymID, server, serverNym, true);

    if (false == bool(nymbox)) {
        LogError()()("Failed to load or verify nymbox.").Flush();

        return false;
    }

    if (nymbox->GetTransactionCount() > 0) {
        LogError()()("Failed due to nymbox not empty.").Flush();

        return false;
    }

    if (0 < context.OpenCronItems()) {
        LogError()()("Failed due to open cron items.").Flush();

        return false;
    }

    if (nymfile.get().GetSetAssetAccounts().size() > 0) {
        LogError()()("Failed due to open asset accounts.").Flush();

        return false;
    }

    if (context.hasOpenTransactions()) {
        LogError()()("Failed due to open transactions.").Flush();

        return false;
    }

    reply.SetSuccess(true);

    // The Nym may have some numbers signed out, but none of them have come
    // through and been "used but not closed" yet. (That is, removed from
    // transaction num list but still on issued num list.) If they had (i.e.
    // if the previous elseif just above had discovered mismatched counts)
    // then we wouldn't be able to delete the Nym until those transactions
    // were closed. Since we know the counts match perfectly, here we remove
    // all the numbers. The client side must know to remove all the numbers
    // as well, when it receives a successful reply that the nym was
    // "deleted."
    context.Reset();
    // TODO: We may also need to mark the Nymbox, as well as the credential
    // files, as "Marked For Deletion."

    reply.DropToNymbox(false);

    return true;
}

auto UserCommandProcessor::cmd_get_account_data(ReplyMessage& reply) const
    -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetAccount(msgIn.acct_id_);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_inbox);
    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_outbox);
    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_acct);

    const auto& context = reply.Context();
    const auto& nymID = context.RemoteNym().ID();
    const auto& serverID = context.Notary();
    const auto& serverNym = *context.Signer();
    const auto accountID =
        server_.API().Factory().AccountIDFromBase58(msgIn.acct_id_->Bytes());
    auto account =
        server_.API().Wallet().Internal().mutable_Account(accountID, reason_);

    if (false == bool(account)) {
        LogError()()("Unable to load account ")(accountID, api_.Crypto())(".")
            .Flush();

        return false;
    }

    if (account.get().GetNymID() != nymID) {
        LogError()()("Nym ")(nymID, api_.Crypto())(" does not own account ")(
            accountID, api_.Crypto())
            .Flush();

        return false;
    }

    const auto inbox = load_inbox(nymID, accountID, serverID, serverNym, false);

    if (false == bool(inbox)) {
        LogError()()("Unable to load or verify inbox for account ")(
            accountID, api_.Crypto())(".")
            .Flush();

        return false;
    }

    const auto outbox =
        load_outbox(nymID, accountID, serverID, serverNym, false);

    if (false == bool(outbox)) {
        LogError()()("Unable to load or verify outbox for account ")(
            accountID, api_.Crypto())(".")
            .Flush();

        return false;
    }

    auto inboxHash = identifier::Generic{};
    auto outboxHash = identifier::Generic{};
    auto serializedAccount = String::Factory();
    auto serializedInbox = String::Factory();
    auto serializedOutbox = String::Factory();
    account.get().SaveContractRaw(serializedAccount);
    inbox->SaveContractRaw(serializedInbox);
    inbox->CalculateInboxHash(inboxHash);
    outbox->SaveContractRaw(serializedOutbox);
    outbox->CalculateOutboxHash(outboxHash);
    reply.SetPayload(serializedAccount);
    reply.SetPayload2(serializedInbox);
    reply.SetPayload3(serializedOutbox);
    reply.SetInboxHash(inboxHash);
    reply.SetOutboxHash(outboxHash);
    reply.SetSuccess(true);

    return true;
}

// the "accountID" on this message will contain the NymID if retrieving a
// boxreceipt for the Nymbox. Otherwise it will contain an AcctID if retrieving
// a boxreceipt for an Asset Acct.
auto UserCommandProcessor::cmd_get_box_receipt(ReplyMessage& reply) const
    -> bool
{
    const auto& msgIn = reply.Original();
    const auto boxType = msgIn.depth_;
    const TransactionNumber number = msgIn.transaction_num_;
    reply.SetAccount(msgIn.acct_id_);
    reply.SetDepth(boxType);
    reply.SetTransactionNumber(number);

    switch (boxType) {
        case NYMBOX_DEPTH: {
            OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_nymbox);
        } break;
        case INBOX_DEPTH: {
            OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_inbox);
        } break;
        case OUTBOX_DEPTH: {
            OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_outbox);
        } break;
        default: {
            LogError()()("Invalid box type.").Flush();

            return false;
        }
    }

    const auto& context = reply.Context();
    const auto& nymID = context.RemoteNym().ID();
    const auto& serverID = context.Notary();
    const auto& serverNym = *context.Signer();
    const auto accountID =
        server_.API().Factory().AccountIDFromBase58(msgIn.acct_id_->Bytes());
    std::unique_ptr<Ledger> box{};

    switch (boxType) {
        case NYMBOX_DEPTH: {
            box = load_nymbox(nymID, serverID, serverNym, false);
        } break;
        case INBOX_DEPTH: {
            box = load_inbox(nymID, accountID, serverID, serverNym, false);
        } break;
        case OUTBOX_DEPTH: {
            box = load_outbox(nymID, accountID, serverID, serverNym, false);
        } break;
        default: {
            LogError()()("Invalid box type.").Flush();

            return false;
        }
    }

    if (false == bool(box)) {
        LogError()()("Unable to load or verify box.").Flush();

        return false;
    }

    auto transaction = box->GetTransaction(number);

    if (nullptr == transaction) {
        LogError()()("Transaction not found: ")(number)(".").Flush();

        return false;
    }

    box->LoadBoxReceipt(number);
    // The above call will replace transaction, inside box, with the full
    // version (instead of the abbreviated version) of that transaction, meaning
    // that the transaction pointer is now a bad pointer, if that call was
    // successful. Therefore we just call GetTransaction() AGAIN. This way,
    // whether LoadBoxReceipt() failed or not (perhaps it's legacy data and is
    // already not abbreviated, and thus the LoadBoxReceipt call failed, but
    // that's doesn't mean we're going to fail HERE, now does it?)
    transaction = box->GetTransaction(number);

    if (false == verify_transaction(transaction.get(), serverNym)) {
        LogError()()("Invalid box item.").Flush();

        return false;
    }

    reply.SetSuccess(true);
    reply.SetPayload(String::Factory(*transaction));

    return true;
}

auto UserCommandProcessor::cmd_get_instrument_definition(
    ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetInstrumentDefinitionID(msgIn.instrument_definition_id_);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_contract);

    reply.SetSuccess(true);
    reply.SetBool(false);
    reply.SetEnum(msgIn.enum_);
    const auto& api = server_.API();
    auto serialized = ByteArray{};

    switch (static_cast<contract::Type>(msgIn.enum_)) {
        case contract::Type::nym: {
            const auto id = api.Factory().NymIDFromBase58(
                msgIn.instrument_definition_id_->Bytes());
            auto contract = api.Wallet().Nym(id);

            if (contract) {
                auto publicNym = protobuf::Nym{};
                if (false == contract->Internal().Serialize(publicNym)) {
                    LogError()()("Failed to serialize nym.").Flush();
                    return false;
                }
                serialized = api_.Factory().Internal().Data(publicNym);
                reply.SetPayload(serialized);
                reply.SetBool(true);
            }
        } break;
        case contract::Type::notary: {
            const auto id = api.Factory().NotaryIDFromBase58(
                msgIn.instrument_definition_id_->Bytes());

            try {
                const auto contract = api.Wallet().Internal().Server(id);
                auto proto = protobuf::ServerContract{};

                if (false == contract->Serialize(proto, true)) {
                    LogError()()("Failed to serialize server contract.")
                        .Flush();
                    return false;
                }

                serialized = api_.Factory().Internal().Data(proto);
                reply.SetPayload(serialized);
                reply.SetBool(true);

                return true;
            } catch (...) {
            }
        } break;
        case contract::Type::unit: {
            const auto id = api.Factory().UnitIDFromBase58(
                msgIn.instrument_definition_id_->Bytes());

            try {
                const auto contract =
                    api.Wallet().Internal().UnitDefinition(id);
                auto proto = protobuf::UnitDefinition{};

                if (false == contract->Serialize(proto, true)) {
                    LogError()()("Failed to serialize unit definition.")
                        .Flush();
                }

                serialized = api_.Factory().Internal().Data(proto);
                reply.SetPayload(serialized);
                reply.SetBool(true);
            } catch (...) {
            }
        } break;
        case contract::Type::invalid:
        default: {
            LogError()()("Invalid type: ")(msgIn.enum_).Flush();
            reply.SetSuccess(true);
            reply.SetBool(false);
        }
    }

    return true;
}

// Get the list of markets on this server.
auto UserCommandProcessor::cmd_get_market_list(ReplyMessage& reply) const
    -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_market_list);

    auto output = Armored::Factory(api_.Crypto());
    std::int32_t count{0};
    reply.SetSuccess(server_.Cron().GetMarketList(output, count));

    if (reply.Success()) {
        reply.SetDepth(count);

        if (0 < count) {
            reply.ClearRequest();
            reply.SetPayload(output);
        }
    }

    return true;
}

// Get the publicly-available list of offers on a specific market.
auto UserCommandProcessor::cmd_get_market_offers(ReplyMessage& reply) const
    -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetTargetNym(msgIn.nym_id2_);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_market_offers);

    auto depth = msgIn.depth_;

    if (depth < 0) { depth = 0; }

    const auto market = server_.Cron().GetMarket(
        server_.API().Factory().NymIDFromBase58(msgIn.nym_id2_->Bytes()));

    if (false == bool(market)) { return false; }

    auto output = Armored::Factory(api_.Crypto());
    std::int32_t nOfferCount{0};
    reply.SetSuccess(market->GetOfferList(output, depth, nOfferCount));

    if (reply.Success()) {
        reply.SetDepth(nOfferCount);

        if (0 < nOfferCount) {
            reply.ClearRequest();
            reply.SetPayload(output);
        }
    }

    return true;
}

// Get a report of recent trades that have occurred on a specific market.
auto UserCommandProcessor::cmd_get_market_recent_trades(
    ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetTargetNym(msgIn.nym_id2_);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_market_recent_trades);

    const auto market = server_.Cron().GetMarket(
        server_.API().Factory().NymIDFromBase58(msgIn.nym_id2_->Bytes()));

    if (false == bool(market)) { return false; }

    auto output = Armored::Factory(api_.Crypto());
    std::int32_t count = 0;
    reply.SetSuccess(market->GetRecentTradeList(output, count));

    if (reply.Success()) {
        reply.SetDepth(count);

        if (0 < count) {
            reply.ClearRequest();
            reply.SetPayload(output);
        }
    }

    return true;
}

auto UserCommandProcessor::cmd_get_mint(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetInstrumentDefinitionID(msgIn.instrument_definition_id_);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_mint);

    reply.SetSuccess(true);
    reply.SetBool(false);
    const auto& unitID = msgIn.instrument_definition_id_;
    auto& mint = api_.GetPublicMint(
        server_.API().Factory().UnitIDFromBase58(unitID->Bytes()));

    if (mint) {
        reply.SetBool(true);
        reply.SetPayload(String::Factory(mint.Internal()));
    }

    return true;
}

// Get the offers that a specific Nym has placed on a specific market.
auto UserCommandProcessor::cmd_get_nym_market_offers(ReplyMessage& reply) const
    -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_nym_market_offers);

    const auto& nymID = reply.Context().RemoteNym().ID();

    auto output = Armored::Factory(api_.Crypto());
    std::int32_t count{0};
    reply.SetSuccess(server_.Cron().GetNym_OfferList(output, nymID, count));

    if (reply.Success()) {
        reply.SetDepth(count);

        if (0 < count) {
            reply.ClearRequest();
            reply.SetPayload(output);
        }
    }

    return true;
}

auto UserCommandProcessor::cmd_get_nymbox(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_nymbox);

    auto& context = reply.Context();
    const auto& nymID = context.RemoteNym().ID();
    const auto& serverID = context.Notary();
    const auto& serverNym = *context.Signer();
    auto newNymboxHash = identifier::Generic{};
    auto nymbox = load_nymbox(nymID, serverID, serverNym, false);

    if (false == bool(nymbox)) {
        LogError()()("Failed to load nymbox.").Flush();
        reply.SetNymboxHash(context.LocalNymboxHash());

        return false;
    }

    nymbox->CalculateNymboxHash(newNymboxHash);
    context.SetLocalNymboxHash(newNymboxHash);
    reply.SetSuccess(true);
    reply.SetPayload(String::Factory(*nymbox));
    reply.SetNymboxHash(newNymboxHash);

    return true;
}

// This command is special because it's the only one that doesn't require a
// request number. All of the other commands, below, will fail above if the
// proper request number isn't included in the message.  They will already have
// failed by this point if they // didn't verify.
auto UserCommandProcessor::cmd_get_request_number(ReplyMessage& reply) const
    -> bool
{
    auto& context = reply.Context();
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_requestnumber);

    const auto& serverNym = *context.Signer();
    auto number = context.Request();

    if (0 == number) {
        number = 1;
        context.SetRequest(number);
    }

    reply.SetRequestNumber(number);
    const auto& NOTARY_ID = server_.GetServerID();
    auto EXISTING_NYMBOX_HASH = context.LocalNymboxHash();

    if (String::Factory(EXISTING_NYMBOX_HASH, api_.Crypto())->Exists()) {
        reply.SetNymboxHash(EXISTING_NYMBOX_HASH);
    } else {
        const auto& nymID = context.RemoteNym().ID();
        auto nymbox = load_nymbox(nymID, NOTARY_ID, serverNym, false);

        if (nymbox) {
            nymbox->CalculateNymboxHash(EXISTING_NYMBOX_HASH);
            context.SetLocalNymboxHash(EXISTING_NYMBOX_HASH);
            reply.SetNymboxHash(EXISTING_NYMBOX_HASH);
        }
    }

    reply.SetSuccess(true);

    return true;
}

auto UserCommandProcessor::cmd_get_transaction_numbers(
    ReplyMessage& reply) const -> bool
{
    auto& context = reply.Context();
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_trans_nums);

    // A few client requests, and a few server replies (not exactly matched up)
    // will include a copy of the NymboxHash.  The server may reject certain
    // client requests that have a bad value here (since it would be out of sync
    // anyway); the client is able to see the server's hash and realize to
    // re-download the nymbox and other intermediary files.
    const auto nCount = context.AvailableNumbers();
    const bool hashMatch = context.NymboxHashMatch();
    const auto& nymID = context.RemoteNym().ID();

    if (context.HaveLocalNymboxHash()) {
        reply.SetNymboxHash(context.LocalNymboxHash());
    }

    if (!hashMatch) {
        LogError()()("Rejecting message since nymbox hash doesn't match.")
            .Flush();

        return false;
    }

    if (nCount > MAX_UNUSED_NUMBERS) {
        LogError()()("Nym ")(nymID, api_.Crypto())(
            " already has more than 50 unused transaction numbers.")
            .Flush();

        return false;
    }

    auto NYMBOX_HASH = identifier::Generic{};
    bool bSuccess = true;
    bool bSavedNymbox = false;
    const auto& serverID = context.Notary();
    auto theLedger{
        api_.Factory().Internal().Session().Ledger(nymID, nymID, serverID)};

    assert_false(nullptr == theLedger);

    NumList theNumlist;

    for (std::int32_t i = 0; i < ISSUE_NUMBER_BATCH; i++) {
        auto number = TransactionNumber{0};

        if (!server_.GetTransactor().issueNextTransactionNumber(number)) {
            LogError()()("Error issuing next transaction number.").Flush();
            bSuccess = false;
            break;
        }

        theNumlist.Add(number);
    }

    TransactionNumber transactionNumber{0};

    if (bSuccess) {
        const bool issued = server_.GetTransactor().issueNextTransactionNumber(
            transactionNumber);

        if (false == issued) {
            LogError()()("Error issuing transaction number.").Flush();
            bSuccess = false;
        }
    }

    if (bSuccess) {
        reply.SetSuccess(add_numbers_to_nymbox(
            transactionNumber,
            theNumlist,
            bSavedNymbox,
            *theLedger,
            NYMBOX_HASH));
    }

    if (bSavedNymbox) {
        context.SetLocalNymboxHash(NYMBOX_HASH);
    } else if (reply.Success()) {
        theLedger->CalculateNymboxHash(NYMBOX_HASH);
        context.SetLocalNymboxHash(NYMBOX_HASH);
    }

    reply.SetNymboxHash(NYMBOX_HASH);

    return true;
}

/// An existing user is creating an issuer account (that he will not control)
/// based on a basket of currencies.
auto UserCommandProcessor::cmd_issue_basket(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_issue_basket);

    auto serialized =
        protobuf::Factory<protobuf::UnitDefinition>(ByteArray{msgIn.payload_});

    if (false == protobuf::syntax::check(LogError(), serialized)) {
        LogError()()("Invalid contract.").Flush();

        return false;
    }

    if (contract::UnitDefinitionType::Basket != translate(serialized.type())) {
        LogError()()("Invalid contract type.").Flush();

        return false;
    }

    // The basket ID should be the same on all servers.
    // The basket contract ID will be unique on each server.
    //
    // The contract ID of the basket is calculated based on the UNSIGNED portion
    // of the contract (so it is unique on every server) and for the same
    // reason_ with the AccountID removed before calculating.
    auto basketAccountID = identifier::Generic{};
    const auto BASKET_ID =
        contract::unit::Basket::CalculateBasketID(server_.API(), serialized);

    const bool exists = server_.GetTransactor().lookupBasketAccountID(
        BASKET_ID, basketAccountID);

    if (exists) {
        LogError()()("Basket already exists.").Flush();

        return false;
    }

    // Let's make sure that all the sub-currencies for this basket are available
    // on this server. NOTE: this also prevents someone from using another
    // basket as a sub-currency UNLESS it already exists on this server. (For
    // example, they couldn't use a basket contract from some other server,
    // since it wouldn't be issued here...) Also note that
    // registerInstrumentDefinition explicitly prevents baskets from being
    // issued -- you HAVE to use issueBasket for creating any basket currency.
    // Taken in tandem, this insures that the only possible way to have a basket
    // currency as a sub-currency is if it's already issued on this server.
    for (const auto& it : serialized.basket().item()) {
        const auto& subcontractID = it.unit();

        try {
            server_.API().Wallet().Internal().UnitDefinition(
                server_.API().Factory().UnitIDFromBase58(subcontractID));
        } catch (...) {
            LogError()()("Missing subcurrency ")(subcontractID)(".").Flush();

            return false;
        }
    }

    const auto& context = reply.Context();
    const auto& serverID = context.Notary();
    const auto& serverNym = context.Signer();
    const auto& serverNymID = context.Signer()->ID();

    // We need to actually create all the sub-accounts. This loop also sets the
    // Account ID onto the basket items (which formerly was blank, from the
    // client.) This loop also adds the BASKET_ID and the NEW ACCOUNT ID to a
    // map on the server for later reference.
    for (auto& it : *serialized.mutable_basket()->mutable_item()) {
        auto newAccount = server_.API().Wallet().Internal().CreateAccount(
            serverNymID,
            serverID,
            server_.API().Factory().UnitIDFromBase58(it.unit()),
            *serverNym,
            Account::basketsub,
            0,
            reason_);

        if (newAccount) {
            auto newAccountID = String::Factory();
            newAccount.get().GetIdentifier(newAccountID);
            it.set_account(newAccountID->Get());
        } else {
            LogError()()("Unable to create subaccount.").Flush();

            return false;
        }
    }

    if (false == contract::unit::Basket::FinalizeTemplate(
                     server_.API(), serverNym, serialized, reason_)) {
        LogError()()("Unable to finalize basket contract.").Flush();

        return false;
    }

    if (contract::UnitDefinitionType::Basket != translate(serialized.type())) {
        LogError()()("Invalid basket contract.").Flush();

        return false;
    }

    try {
        const auto contract =
            server_.API().Wallet().Internal().UnitDefinition(serialized);
        const auto& contractID = contract->ID();
        reply.SetInstrumentDefinitionID(
            String::Factory(contractID, api_.Crypto()));

        // I don't save this here. Instead, I wait for AddBasketAccountID and
        // then I call SaveMainFile after that. See below.

        // TODO need better code for reverting when something screws up halfway
        // through a change. If I add this contract, it's not enough to just
        // "not save" the file. I actually need to re-load the file in order to
        // TRULY "make sure" this won't screw something up on down the line.

        // Once the new Asset Type is generated, from which the BasketID can be
        // requested at will, next we need to create the issuer account for that
        // new Asset Type.  (We have the instrument definition ID and the
        // contract file. Now let's create the issuer account the same as we
        // would for any normal issuer account.)
        //
        // The issuer account is special compared to a normal issuer account,
        // because within its walls, it must store an OTAccount for EACH
        // sub-contract, in order to store the reserves. That's what makes the
        // basket work.

        auto basketAccount = server_.API().Wallet().Internal().CreateAccount(
            serverNymID,
            serverID,
            contractID,
            *serverNym,
            Account::basket,
            0,
            reason_);

        if (false == bool(basketAccount)) {
            LogError()()("Failed to instantiate basket account.").Flush();

            return false;
        }

        basketAccount.get().GetIdentifier(basketAccountID);
        reply.SetSuccess(true);
        reply.SetAccount(String::Factory(basketAccountID, api_.Crypto()));

        // So the server can later use the BASKET_ID (which is universal) to
        // lookup the account ID on this server corresponding to that basket.
        // (The account ID will be different from server to server, thus the
        // need to be able to look it up via the basket ID.)
        server_.GetTransactor().addBasketAccountID(
            BASKET_ID, basketAccountID, contractID);
        server_.GetMainFile().SaveMainFile();
        api_.UpdateMint(contractID);
        basketAccount.Release();

        return true;
    } catch (...) {
        LogError()()("Failed to construct basket contract object.").Flush();

        return false;
    }
}

auto UserCommandProcessor::cmd_notarize_transaction(ReplyMessage& reply) const
    -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetAccount(msgIn.acct_id_);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_notarize_transaction);

    auto& context = reply.Context();
    const auto& nymID = context.RemoteNym().ID();
    const auto& serverID = context.Notary();
    const auto& serverNym = *context.Signer();
    const auto& serverNymID = serverNym.ID();
    const auto accountID =
        server_.API().Factory().AccountIDFromBase58(msgIn.acct_id_->Bytes());
    auto nymboxHash = identifier::Generic{};
    auto input{
        api_.Factory().Internal().Session().Ledger(nymID, accountID, serverID)};
    auto responseLedger{api_.Factory().Internal().Session().Ledger(
        serverNymID, accountID, serverID, otx::ledgerType::message, false)};

    assert_false(nullptr == input);
    assert_false(nullptr == responseLedger);

    if (false == hash_check(context, nymboxHash)) {
        LogError()()("Nymbox hash mismatch.").Flush();

        return false;
    }

    if (false == input->LoadLedgerFromString(String::Factory(msgIn.payload_))) {
        LogError()()("Unable to load input ledger.").Flush();

        return false;
    }

    // Returning before this point will result in the reply message
    // success_ = false, and no reply ledger
    FinalizeResponse response(api_, serverNym, reply, *responseLedger);
    reply.SetSuccess(true);
    reply.DropToNymbox(true);
    // Returning after this point will result in the reply message
    // success_ = true, and a signed reply ledger containing at least one
    // transaction

    for (const auto& it : input->GetTransactionMap()) {
        const auto transaction = it.second;

        if (nullptr == transaction) {
            LogError()()("Invalid input ledger.").Flush();
            continue;
        }

        const auto inputNumber = transaction->GetTransactionNum();
        auto outTrans = response.AddResponse(std::shared_ptr<OTTransaction>(
            api_.Factory()
                .Internal()
                .Session()
                .Transaction(
                    *responseLedger,
                    otx::transactionType::error_state,
                    otx::originType::not_applicable,
                    inputNumber)
                .release()));

        bool success{false};
        server_.GetNotary().NotarizeTransaction(
            context, *transaction, *outTrans, success);

        if (outTrans->IsCancelled()) {
            LogError()()("Success canceling transaction ")(
                inputNumber)(" for nym ")(nymID, api_.Crypto())(".")
                .Flush();
        } else {
            if (success) {
                LogDetail()()("Success processing transaction ")(
                    inputNumber)(" for nym ")(nymID, api_.Crypto())(".")
                    .Flush();
            } else {
                LogError()()("Failure processing transaction ")(
                    inputNumber)(" for nym ")(nymID, api_.Crypto())(".")
                    .Flush();
            }
        }

        assert_true(
            inputNumber == outTrans->GetTransactionNum(),
            "Transaction number and response number should always be the same. "
            "(But this time, they weren't.)");
    }

    return true;
}

auto UserCommandProcessor::cmd_ping_notary(ReplyMessage& reply) const -> bool
{
    reply.SetSuccess(check_ping_notary(reply.Original()));

    return true;
}

auto UserCommandProcessor::cmd_process_inbox(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetAccount(msgIn.acct_id_);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_process_inbox);

    auto& context = reply.Context();
    const auto& clientNym = context.RemoteNym();
    const auto& nymID = clientNym.ID();
    const auto& serverID = context.Notary();
    const auto& serverNym = *context.Signer();
    const auto& serverNymID = serverNym.ID();
    const auto& nym = reply.Context().RemoteNym();
    const auto accountID =
        server_.API().Factory().AccountIDFromBase58(msgIn.acct_id_->Bytes());
    auto nymboxHash = identifier::Generic{};
    auto input{
        api_.Factory().Internal().Session().Ledger(nymID, accountID, serverID)};
    auto responseLedger{api_.Factory().Internal().Session().Ledger(
        serverNymID, accountID, serverID, otx::ledgerType::message, false)};

    assert_false(nullptr == input);
    assert_false(nullptr == responseLedger);

    if (false == hash_check(context, nymboxHash)) {
        LogError()()("Nymbox hash mismatch.").Flush();

        return false;
    }

    if (false == input->LoadLedgerFromString(String::Factory(msgIn.payload_))) {
        LogError()()("Unable to load input ledger.").Flush();

        return false;
    }

    auto account =
        server_.API().Wallet().Internal().mutable_Account(accountID, reason_);

    if (false == bool(account)) { return false; }

    auto pProcessInbox =
        input->GetTransaction(otx::transactionType::processInbox);

    if (false == bool(pProcessInbox)) {
        LogError()()("processInbox transaction not found in input ledger.")
            .Flush();

        return false;
    }

    auto& processInbox = *pProcessInbox;
    const auto inputNumber = processInbox.GetTransactionNum();

    if (false == context.VerifyIssuedNumber(inputNumber)) {
        LogError()()("Transaction number ")(inputNumber)(" is not issued to ")(
            nymID, api_.Crypto())
            .Flush();

        return false;
    }

    // The items' acct and server ID were already checked in VerifyContractID()
    // when they were loaded. Now this checks a little deeper, to verify
    // ownership, signatures, and transaction number on each item. That way
    // those things don't have to be checked for security over and over again in
    // the subsequent calls.
    if (false == processInbox.VerifyItems(nym, reason_)) {
        LogError()()("Failed to verify transaction items.").Flush();

        return false;
    }

    // We don't want any transaction number being used twice. (The number, at
    // this point, is STILL issued to the user, who is still responsible for
    // that number and must continue signing for it. All this means here is that
    // the user no longer has the number on his AVAILABLE list. Removal from
    // issued list happens separately.)
    if (context.ConsumeAvailable(inputNumber)) {
        LogDetail()()("Consumed available number ")(inputNumber).Flush();
    } else {
        LogError()()("Error removing available number ")(inputNumber).Flush();

        return false;
    }

    // Returning after this point will result in the reply message
    // success_ = true, and a signed reply ledger containing at least one
    // transaction
    FinalizeResponse response(api_, serverNym, reply, *responseLedger);
    reply.SetSuccess(true);
    reply.DropToNymbox(true);
    auto pResponseTrans = response.AddResponse(std::shared_ptr<OTTransaction>(
        api_.Factory()
            .Internal()
            .Session()
            .Transaction(
                *responseLedger,
                otx::transactionType::error_state,
                otx::originType::not_applicable,
                inputNumber)
            .release()));

    if (false == bool(pResponseTrans)) {
        LogError()()("Failed to instantiate response transaction").Flush();
        reply.SetSuccess(false);

        return false;
    }

    auto& responseTransaction = *pResponseTrans;
    bool transactionSuccess{false};
    std::unique_ptr<Ledger> pInbox(account.get().LoadInbox(serverNym));
    std::unique_ptr<Ledger> pOutbox(account.get().LoadOutbox(serverNym));

    if (false == bool(pInbox) || false == bool(pOutbox)) {
        LogError()()("Error loading or verifying inbox or outbox.").Flush();
        responseTransaction.SignContract(serverNym, reason_);
        responseTransaction.SaveContract();

        return true;
    }

    auto& inbox = *pInbox;
    auto& outbox = *pOutbox;
    server_.GetNotary().NotarizeProcessInbox(
        context,
        account,
        processInbox,
        responseTransaction,
        inbox,
        outbox,
        transactionSuccess);
    const auto consumed = context.ConsumeIssued(inputNumber);

    if (consumed) {
        LogDetail()()("Consumed issued number ")(inputNumber).Flush();
    } else {
        LogError()()("Error removing issued number ")(inputNumber).Flush();

        LogAbort()().Abort();
    }

    if (transactionSuccess) {
        LogDetail()()("Success processing process inbox ")(
            inputNumber)(" for nym ")(nymID, api_.Crypto())
            .Flush();
    } else {
        LogError()()("Failure processing process inbox ")(
            inputNumber)(" for nym ")(nymID, api_.Crypto())
            .Flush();
    }

    assert_true(
        inputNumber == responseTransaction.GetTransactionNum(),
        "Transaction number and response number should always be the same. "
        "(But this time they weren't).");

    return true;
}

auto UserCommandProcessor::cmd_process_nymbox(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_process_nymbox);

    auto& context = reply.Context();
    const auto& nymID = context.RemoteNym().ID();
    const auto& serverID = context.Notary();
    const auto& serverNym = *context.Signer();
    const auto& serverNymID = serverNym.ID();
    auto nymboxHash = identifier::Generic{};
    auto input{
        api_.Factory().Internal().Session().Ledger(nymID, nymID, serverID)};
    auto responseLedger{api_.Factory().Internal().Session().Ledger(
        serverNymID, nymID, serverID, otx::ledgerType::message, false)};

    assert_false(nullptr == input);
    assert_false(nullptr == responseLedger);

    if (false == hash_check(context, nymboxHash)) {
        LogError()()("Nymbox hash mismatch.").Flush();

        return false;
    }

    if (false == input->LoadLedgerFromString(String::Factory(msgIn.payload_))) {
        LogError()()("Unable to load input ledger.").Flush();

        return false;
    }

    // Returning before this point will result in the reply message
    // success_ = false, and no reply ledger
    FinalizeResponse response(api_, serverNym, reply, *responseLedger);
    reply.SetSuccess(true);
    bool nymboxUpdated{false};
    // Returning after this point will result in the reply message
    // success_ = true, and a signed reply ledger containing at least one
    // transaction

    for (const auto& it : input->GetTransactionMap()) {
        const auto transaction = it.second;

        if (nullptr == transaction) {
            LogError()()("Invalid input ledger.").Flush();

            LogAbort()().Abort();
        }

        const auto inputNumber = transaction->GetTransactionNum();
        auto responseTrans =
            response.AddResponse(std::shared_ptr<OTTransaction>(
                api_.Factory()
                    .Internal()
                    .Session()
                    .Transaction(
                        *responseLedger,
                        otx::transactionType::error_state,
                        otx::originType::not_applicable,
                        transaction->GetTransactionNum())
                    .release()));
        bool success{false};
        nymboxUpdated = server_.GetNotary().NotarizeProcessNymbox(
            context, *transaction, *responseTrans, success);

        if (success) {
            LogDetail()()(
                "Success processing process "
                "nymbox ")(inputNumber)(" for nym ")(nymID, api_.Crypto())(".")
                .Flush();
        } else {
            LogError()()(
                "Failure processing process "
                "nymbox ")(inputNumber)(" for nym ")(nymID, api_.Crypto())(".")
                .Flush();
        }

        assert_true(
            inputNumber == responseTrans->GetTransactionNum(),
            "Transaction number and response number should always be the same. "
            "(But this time, they weren't.)");
    }

    reply.DropToNymbox(nymboxUpdated);

    return true;
}

auto UserCommandProcessor::cmd_query_instrument_definitions(
    ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_get_contract);

    const std::unique_ptr<OTDB::Storable> pStorable(OTDB::DecodeObject(
        api_.Crypto(), OTDB::STORED_OBJ_STRING_MAP, msgIn.payload_->Get()));
    auto* inputMap = dynamic_cast<OTDB::StringMap*>(pStorable.get());

    if (nullptr == inputMap) { return false; }

    auto& map = inputMap->the_map_;
    UnallocatedMap<UnallocatedCString, UnallocatedCString> newMap{};

    for (auto& it : map) {
        const auto& unitID = it.first;
        const auto& status = it.second;

        if (unitID.empty()) { continue; }

        // TODO security: limit on length of this map? (sent through
        // user message...)
        // "exists" means, "Here's an instrument definition id. Please tell me
        // whether or not it's actually issued on this server." Future options
        // might include "issue", "audit", "contract", etc.
        if (0 == status.compare("exists")) {
            try {
                server_.API().Wallet().Internal().UnitDefinition(
                    server_.API().Factory().UnitIDFromBase58(unitID));

                newMap[unitID] = "true";
            } catch (...) {
                newMap[unitID] = "false";
            }
        }
    }

    map.swap(newMap);
    const auto output = OTDB::EncodeObject(server_.API(), *inputMap);

    if (false == output.empty()) {
        reply.SetSuccess(true);
        reply.SetPayload(String::Factory(output));
    }

    return true;
}

/// An existing user is creating an asset account.
auto UserCommandProcessor::cmd_register_account(ReplyMessage& reply) const
    -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_create_asset_acct);

    const auto& context = reply.Context();
    const auto& nymID = context.RemoteNym().ID();
    const auto& serverID = context.Notary();
    const auto& serverNym = *context.Signer();
    const auto contractID = server_.API().Factory().UnitIDFromBase58(
        msgIn.instrument_definition_id_->Bytes());
    auto account = server_.API().Wallet().Internal().CreateAccount(
        nymID, serverID, contractID, serverNym, Account::user, 0, reason_);

    // If we successfully create the account, then bundle it in the message XML
    // payload
    if (false == bool(account)) {
        LogError()()("Unable to create new account.").Flush();

        return false;
    }

    try {
        const auto contract = server_.API().Wallet().Internal().UnitDefinition(
            account.get().GetInstrumentDefinitionID());

        if (contract->Type() == contract::UnitDefinitionType::Security) {
            // The instrument definition keeps a list of all accounts for that
            // type. (For shares, not for currencies.)
            if (false ==
                contract->AddAccountRecord(
                    server_.API().DataFolder().string(), account.get())) {
                LogError()()("Unable to add account record ")(
                    contractID, api_.Crypto())
                    .Flush();

                return false;
            }
        }
    } catch (...) {
        LogError()()("Unable to load unit definition ")(
            contractID, api_.Crypto())
            .Flush();

        return false;
    }

    auto accountID = identifier::Account{};
    account.get().GetIdentifier(accountID);
    auto outbox{
        api_.Factory().Internal().Session().Ledger(nymID, accountID, serverID)};
    auto inbox{
        api_.Factory().Internal().Session().Ledger(nymID, accountID, serverID)};

    assert_false(nullptr == outbox);
    assert_false(nullptr == inbox);

    bool inboxLoaded = inbox->LoadInbox();
    bool outboxLoaded = outbox->LoadOutbox();

    // ...or generate them otherwise...

    if (inboxLoaded) {
        inboxLoaded = inbox->VerifyAccount(serverNym);
    } else {
        inboxLoaded = inbox->CreateLedger(
            nymID, accountID, serverID, otx::ledgerType::inbox, true);

        if (inboxLoaded) {
            inboxLoaded = inbox->SignContract(serverNym, reason_);
        }

        if (inboxLoaded) { inboxLoaded = inbox->SaveContract(); }

        if (inboxLoaded) { inboxLoaded = account.get().SaveInbox(*inbox); }
    }

    if (true == outboxLoaded) {
        outboxLoaded = outbox->VerifyAccount(serverNym);
    } else {
        outboxLoaded = outbox->CreateLedger(
            nymID, accountID, serverID, otx::ledgerType::outbox, true);

        if (outboxLoaded) {
            outboxLoaded = outbox->SignContract(serverNym, reason_);
        }

        if (outboxLoaded) { outboxLoaded = outbox->SaveContract(); }

        if (outboxLoaded) { outboxLoaded = account.get().SaveOutbox(*outbox); }
    }

    if (false == inboxLoaded) {
        LogError()()("Error generating inbox for account ")(
            accountID, api_.Crypto())(".")
            .Flush();

        return false;
    }

    if (false == outboxLoaded) {
        LogError()()("Error generating outbox for account ")(
            accountID, api_.Crypto())(".")
            .Flush();

        return false;
    }

    reply.SetSuccess(true);
    reply.SetAccount(String::Factory(accountID, api_.Crypto()));
    auto nymfile = server_.API().Wallet().Internal().mutable_Nymfile(
        reply.Context().RemoteNym().ID(), reason_);
    auto& theAccountSet = nymfile.get().GetSetAssetAccounts();
    theAccountSet.insert(String::Factory(accountID, api_.Crypto())->Get());
    reply.SetPayload(String::Factory(account.get()));
    reply.DropToNymbox(false);
    account.Release();

    return true;
}

auto UserCommandProcessor::cmd_register_contract(ReplyMessage& reply) const
    -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_register_contract);

    const auto type = static_cast<contract::Type>(msgIn.enum_);

    switch (type) {
        case (contract::Type::nym): {
            const auto nym =
                protobuf::Factory<protobuf::Nym>(ByteArray{msgIn.payload_});
            reply.SetSuccess(bool(server_.API().Wallet().Internal().Nym(nym)));
        } break;
        case (contract::Type::notary): {
            const auto server = protobuf::Factory<protobuf::ServerContract>(
                ByteArray{msgIn.payload_});
            try {
                server_.API().Wallet().Internal().Server(server);
                reply.SetSuccess(true);
            } catch (const std::exception& e) {
                LogError()()(e.what()).Flush();
                reply.SetSuccess(false);
            }
        } break;
        case (contract::Type::unit): {
            try {
                server_.API().Wallet().Internal().UnitDefinition(
                    protobuf::Factory<protobuf::UnitDefinition>(
                        ByteArray{msgIn.payload_}));
                reply.SetSuccess(true);
            } catch (const std::exception& e) {
                LogError()()(e.what()).Flush();
                reply.SetSuccess(false);
            }
        } break;
        case contract::Type::invalid:
        default: {
            LogError()()("Invalid contract type: ")(msgIn.enum_).Flush();
        }
    }

    return true;
}

/// An existing user is issuing a new currency.
auto UserCommandProcessor::cmd_register_instrument_definition(
    ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetInstrumentDefinitionID(msgIn.instrument_definition_id_);
    const auto contractID = server_.API().Factory().UnitIDFromBase58(
        msgIn.instrument_definition_id_->Bytes());

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_issue_asset);

    // Make sure the contract isn't already available on this server.
    try {
        server_.API().Wallet().Internal().UnitDefinition(contractID);
        LogError()()("Instrument definition ")(contractID, api_.Crypto())(
            " already exists.")
            .Flush();

        return false;
    } catch (...) {
    }

    const auto serialized =
        protobuf::Factory<protobuf::UnitDefinition>(ByteArray{msgIn.payload_});

    if (contract::UnitDefinitionType::Basket == translate(serialized.type())) {
        LogError()()("Incorrect unit type.").Flush();

        return false;
    }

    try {
        const auto contract =
            server_.API().Wallet().Internal().UnitDefinition(serialized);

        if (contract->ID() != contractID) {
            LogError()()("ID mismatch.").Flush();

            return false;
        }
    } catch (...) {
        LogError()()("Invalid contract.").Flush();

        return false;
    }

    // Create an ISSUER account (like a normal account, except it can go
    // negative)
    const auto& context = reply.Context();
    const auto& nymID = context.RemoteNym().ID();
    const auto& serverID = context.Notary();
    const auto& serverNym = *context.Signer();
    auto account = server_.API().Wallet().Internal().CreateAccount(
        nymID, serverID, contractID, serverNym, Account::issuer, 0, reason_);

    if (false == bool(account)) {
        LogError()()("Unable to generate issuer account.").Flush();

        return false;
    }

    reply.SetPayload(String::Factory(account.get()));
    auto accountID = identifier::Account{};
    account.get().GetIdentifier(accountID);
    reply.SetAccount(String::Factory(accountID, api_.Crypto()));
    server_.GetMainFile().SaveMainFile();

    if (false == account.get().InitBoxes(serverNym, reason_)) {

        LogError()()("Error initializing boxes for account ")(
            accountID, api_.Crypto())(".")
            .Flush();

        return false;
    }

    reply.SetSuccess(true);
    account.Release();
    auto nymfile = server_.API().Wallet().Internal().mutable_Nymfile(
        reply.Context().RemoteNym().ID(), reason_);
    auto& theAccountSet = nymfile.get().GetSetAssetAccounts();
    theAccountSet.insert(accountID.asBase58(server_.API().Crypto()));
    reply.DropToNymbox(false);
    api_.UpdateMint(contractID);

    return true;
}

auto UserCommandProcessor::cmd_register_nym(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_create_user_acct);

    auto serialized =
        protobuf::Factory<protobuf::Nym>(ByteArray{reply.Original().payload_});
    auto sender_nym = server_.API().Wallet().Internal().Nym(serialized);

    if (false == bool(sender_nym)) {
        LogError()()("Invalid nym: ")(msgIn.nym_id_.get())(".").Flush();

        return false;
    }

    LogDebug()()("Nym verified!").Flush();

    if (false == msgIn.VerifySignature(*sender_nym)) {
        LogError()()("Invalid signature ")(sender_nym->ID(), api_.Crypto())(".")
            .Flush();

        return false;
    }

    LogDebug()()("Signature verified!").Flush();

    if (false == reply.LoadContext(reason_)) { return false; }

    auto& context = reply.Context();

    // The below block is for the case where the Nym is re-registering, even
    // though he's already registered on this Notary.
    //
    // He ALREADY exists. We'll set success to true, and send him a copy of his
    // own nymfile.
    if (0 != context.Request()) { return reregister_nym(reply); }

    context.IncrementRequest();
    reply.SetSuccess(true);
    context.InitializeNymbox(reason_);
    context.SetRemoteNymboxHash(context.LocalNymboxHash());

    LogDetail()()("Success registering Nym credentials.").Flush();
    auto strNymContents = String::Factory();
    // This will save the nymfile.
    auto nymfile = server_.API().Wallet().Internal().mutable_Nymfile(
        sender_nym->ID(), reason_);
    nymfile.get().SerializeNymFile(strNymContents);
    reply.SetPayload(strNymContents);
    reply.SetSuccess(true);

    return true;
}

auto UserCommandProcessor::cmd_request_admin(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_request_admin);

    const String& requestingNym = msgIn.nym_id_;
    const UnallocatedCString candidate = requestingNym.Get();
    const UnallocatedCString providedPassword = msgIn.acct_id_->Get();
    UnallocatedCString overrideNym, password;
    bool notUsed = false;
    const auto& config = server_.API().Config().Internal();
    config.CheckSet_str(
        String::Factory("permissions"),
        String::Factory("override_nym_id"),
        String::Factory(),
        overrideNym,
        notUsed);
    config.CheckSet_str(
        String::Factory("permissions"),
        String::Factory("admin_password"),
        String::Factory(),
        password,
        notUsed);
    const bool noAdminYet = overrideNym.empty();
    const bool passwordSet = !password.empty();
    const bool readyForAdmin = (noAdminYet && passwordSet);
    const bool correctPassword = (providedPassword == password);
    const bool returningAdmin = (candidate == overrideNym);
    const bool duplicateRequest = (!noAdminYet && returningAdmin);
    reply.SetSuccess(true);
    reply.SetBool(false);

    if (false == correctPassword) {
        LogError()()("Incorrect password.").Flush();

        return true;
    }

    if (readyForAdmin) {
        const auto set = config.Set_str(
            String::Factory("permissions"),
            String::Factory("override_nym_id"),
            requestingNym,
            notUsed);

        if (set) {
            if (config.Save()) {
                LogConsole()("    Override nym set to ")(requestingNym).Flush();
                reply.SetBool(true);
            } else {
                LogError()()("failed to save config file").Flush();
            }
        }
    } else {
        if (duplicateRequest) {
            reply.SetBool(true);
        } else {
            LogError()()("Admin password empty or admin nym "
                         "already set.")
                .Flush();
        }
    }

    return true;
}

auto UserCommandProcessor::cmd_send_nym_message(ReplyMessage& reply) const
    -> bool
{
    auto& context = reply.Context();
    const auto& sender = context.RemoteNym().ID();
    const auto& server = context.Notary();
    const auto& msgIn = reply.Original();
    const auto& targetNym = msgIn.nym_id2_;
    const auto recipient = api_.Factory().NymIDFromBase58(targetNym->Bytes());
    reply.SetTargetNym(targetNym);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_send_message);

    reply.SetSuccess(send_message_to_nym(server, sender, recipient, msgIn));

    if (false == reply.Success()) {
        LogError()()("Failed to send message to nym.").Flush();
    }

    return true;
}

// msg, the request msg from payer, which is attached WHOLE to the Nymbox
// receipt. contains payment already.
// or pass pPayment instead: we will create our own msg here (with payment
// inside) to be attached to the receipt.
auto UserCommandProcessor::send_message_to_nym(
    const identifier::Notary& NOTARY_ID,
    const identifier::Nym& SENDER_NYM_ID,
    const identifier::Nym& RECIPIENT_NYM_ID,
    const Message& pMsg) const -> bool
{
    return server_.DropMessageToNymbox(
        NOTARY_ID,
        SENDER_NYM_ID,
        RECIPIENT_NYM_ID,
        otx::transactionType::message,
        pMsg);
}

auto UserCommandProcessor::cmd_trigger_clause(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_trigger_clause);

    const auto& number = msgIn.transaction_num_;
    const auto& context = reply.Context();
    const auto& nym = context.RemoteNym();

    if (context.HaveLocalNymboxHash()) {
        const auto hash = context.LocalNymboxHash();
        reply.SetNymboxHash(hash);
    }

    if (false == context.NymboxHashMatch()) {
        LogError()()("Rejecting message due to nymbox hash mismatch.").Flush();

        return false;
    }

    OTSmartContract* smartContract{nullptr};
    auto cronItem = server_.Cron().GetItemByValidOpeningNum(number);

    if (false == bool(cronItem)) {
        LogError()()("Can not find smart contract based on transaction "
                     "number ")(number)(".")
            .Flush();

        return false;
    }

    smartContract = dynamic_cast<OTSmartContract*>(cronItem.get());

    if (nullptr == smartContract) {
        LogError()()("Cron item ")(number)(" is not a smart contract.").Flush();

        return false;
    }

    // FIND THE PARTY / PARTY NAME
    OTAgent* agent = nullptr;
    OTParty* party = smartContract->FindPartyBasedOnNymAsAgent(nym, &agent);

    if (nullptr == party) {
        LogError()()("Unable to find party to this contract based on nym.")
            .Flush();

        return false;
    }

    const UnallocatedCString clauseID = msgIn.nym_id2_->Get();

    if (smartContract->CanExecuteClause(party->GetPartyName(), clauseID)) {
        // Execute the clause.
        mapOfClauses theMatchingClauses{};
        auto* clause = smartContract->GetClause(clauseID);

        if (nullptr == clause) {
            LogError()()("Clause ")(clauseID)(" does not exist.").Flush();

            return false;
        }

        theMatchingClauses.insert({clauseID, clause});
        smartContract->ExecuteClauses(theMatchingClauses, reason_);

        if (smartContract->IsFlaggedForRemoval()) {
            LogError()()("Removing smart contract ")(
                smartContract->GetTransactionNum())(" from"
                                                    " cron"
                                                    ".")
                .Flush();
        }
    } else {
        LogError()()("Party ")(party->GetPartyName())(
            " lacks permission to execute "
            "clause ")(clauseID)(".")
            .Flush();

        return false;
    }

    reply.SetSuccess(true);

    // If we just removed the smart contract from cron, that means a
    // finalReceipt was just dropped into the inboxes for the relevant asset
    // accounts. Once I process that receipt out of my inbox, (which will
    // require my processing out all related marketReceipts) then the closing
    // number will be removed from my list of responsibility.

    LogError()()("Party ")(party->GetPartyName())(" successfully triggered "
                                                  "clause: ")(clauseID)(".")
        .Flush();

    return true;
}

auto UserCommandProcessor::cmd_usage_credits(ReplyMessage& reply) const -> bool
{
    const auto& msgIn = reply.Original();
    reply.SetTargetNym(msgIn.nym_id2_);
    reply.SetDepth(0);

    OT_ENFORCE_PERMISSION_MSG(ServerSettings::_cmd_usage_credits);

    const auto& adminContext = reply.Context();
    const auto& serverID = adminContext.Notary();
    const auto& adminNym = adminContext.RemoteNym();
    const auto& adminNymID = adminNym.ID();
    const auto& serverNym = *adminContext.Signer();
    const bool admin = isAdmin(server_.API(), adminNymID);
    auto adjustment = msgIn.depth_;

    if (false == admin) { adjustment = 0; }

    const auto targetNymID =
        server_.API().Factory().NymIDFromBase58(msgIn.nym_id2_->Bytes());
    auto nymID = identifier::Nym{};

    if (targetNymID == adminNymID) {
        nymID = adminNymID;
    } else {
        if (false == admin) {
            LogError()()("This command is only available to admin nym.")
                .Flush();

            return false;
        }

        nymID = targetNymID;
    }

    auto nymfile =
        server_.API().Wallet().Internal().mutable_Nymfile(targetNymID, reason_);
    auto nymbox = load_nymbox(targetNymID, serverID, serverNym, true);

    if (false == bool(nymbox)) {
        nymbox = create_nymbox(targetNymID, serverID, serverNym);
    }

    if (false == bool(nymbox)) {
        LogError()()("Unable to load nymbox for ")(targetNymID, api_.Crypto())(
            ".")
            .Flush();

        return false;
    }

    const auto oldCredits = nymfile.get().GetUsageCredits();
    auto newCredits = oldCredits + adjustment;

    if (0 > newCredits) { newCredits = -1; }

    if (0 != adjustment) { nymfile.get().SetUsageCredits(newCredits); }
    reply.SetSuccess(true);

    if (ServerSettings::_admin_usage_credits) {
        reply.SetDepth(newCredits);
    } else {
        reply.SetDepth(-1);
    }

    return true;
}

auto UserCommandProcessor::create_nymbox(
    const identifier::Nym& nymID,
    const identifier::Notary& server,
    const identity::Nym& serverNym) const -> std::unique_ptr<Ledger>
{
    auto nymbox{
        api_.Factory().Internal().Session().Ledger(nymID, nymID, server)};

    if (false == bool(nymbox)) {
        LogError()()("Unable to instantiate nymbox for ")(nymID, api_.Crypto())(
            ".")
            .Flush();

        return {};
    }

    if (false ==
        nymbox->GenerateLedger(nymID, server, otx::ledgerType::nymbox, true)) {
        LogError()()("Unable to generate nymbox for ")(nymID, api_.Crypto())(
            ".")
            .Flush();

        return {};
    }

    auto notUsed = identifier::Generic{};

    if (false == save_nymbox(serverNym, notUsed, *nymbox)) {
        LogError()()("Unable to save nymbox for ")(nymID, api_.Crypto())(".")
            .Flush();

        return {};
    }

    return nymbox;
}

// After EVERY / ANY transaction, plus certain messages, we drop a copy of the
// server's reply into the Nymbox.  This way we are GUARANTEED that the Nym will
// receive and process it. (And thus never get out of sync.)  This is the
// function used for doing that.
void UserCommandProcessor::drop_reply_notice_to_nymbox(
    const api::session::Wallet& wallet,
    const Message& message,
    const std::int64_t& lRequestNum,
    const bool bReplyTransSuccess,
    otx::context::Client& context,
    Server& server) const
{
    const auto& nymID = context.RemoteNym().ID();
    const auto& serverID = context.Notary();
    const auto& serverNym = *context.Signer();
    auto theNymbox{server.API().Factory().Internal().Session().Ledger(
        nymID, nymID, serverID)};

    assert_false(nullptr == theNymbox);

    bool bSuccessLoadingNymbox = theNymbox->LoadNymbox();

    if (true == bSuccessLoadingNymbox) {
        bSuccessLoadingNymbox =
            (theNymbox->VerifyContractID() &&
             theNymbox->VerifySignature(serverNym));
    }

    if (!bSuccessLoadingNymbox) {
        LogError()()("Failed loading or verifying Nymbox for user: ")(
            nymID, api_.Crypto())(".")
            .Flush();

        return;
    }

    auto lReplyNoticeTransNum = TransactionNumber{};
    const bool bGotNextTransNum =
        server.GetTransactor().issueNextTransactionNumber(lReplyNoticeTransNum);

    if (!bGotNextTransNum) {
        LogError()()("Error getting next transaction number for a "
                     "otx::transactionType::replyNotice.")
            .Flush();

        return;
    }

    auto pReplyNotice{server.API().Factory().Internal().Session().Transaction(
        *theNymbox,
        otx::transactionType::replyNotice,
        otx::originType::not_applicable,
        lReplyNoticeTransNum)};

    assert_false(nullptr == pReplyNotice);

    auto pReplyNoticeItem{server.API().Factory().Internal().Session().Item(
        *pReplyNotice, otx::itemType::replyNotice, {})};

    assert_false(nullptr == pReplyNoticeItem);

    // Nymbox notice is always a success. It's just a notice. (The message
    // inside it will have success/failure also, and any transaction inside that
    // will also.)
    pReplyNoticeItem->SetStatus(Item::acknowledgement);
    // Purpose of this notice is to carry a copy of server's reply message (to
    // certain requests, including all transactions.)
    pReplyNoticeItem->SetAttachment(String::Factory(message));
    pReplyNoticeItem->SignContract(serverNym, reason_);
    pReplyNoticeItem->SaveContract();
    // the Transaction's destructor will cleanup the item. It "owns" it now. So
    // the client-side can quickly/easily match up the replyNotices in the
    // Nymbox with the request numbers of the messages that were sent. I think
    // this is actually WHY the server message low-level functions now RETURN
    // the request number. FYI: replyNotices will ONLY be in my Nymbox if the
    // MESSAGE was successful. (Meaning, the balance agreement might have
    // failed, and the transaction might have failed, but the MESSAGE ITSELF
    // must be a success, in order for the replyNotice to appear in the Nymbox.)
    const std::shared_ptr<Item> replyNoticeItem{pReplyNoticeItem.release()};
    pReplyNotice->AddItem(replyNoticeItem);
    pReplyNotice->SetRequestNum(lRequestNum);
    pReplyNotice->SetReplyTransSuccess(bReplyTransSuccess);
    pReplyNotice->SignContract(serverNym, reason_);
    pReplyNotice->SaveContract();
    // Add the replyNotice to the nymbox.
    const std::shared_ptr<OTTransaction> replyNotice{pReplyNotice.release()};
    theNymbox->AddTransaction(replyNotice);
    theNymbox->ReleaseSignatures();
    theNymbox->SignContract(serverNym, reason_);
    theNymbox->SaveContract();
    auto NYMBOX_HASH = identifier::Generic{};
    theNymbox->SaveNymbox(NYMBOX_HASH);
    replyNotice->SaveBoxReceipt(*theNymbox);
    context.SetLocalNymboxHash(NYMBOX_HASH);
}

auto UserCommandProcessor::hash_check(
    const otx::context::Client& context,
    identifier::Generic& nymboxHash) const -> bool
{
    if (context.HaveLocalNymboxHash()) {
        nymboxHash = context.LocalNymboxHash();
    } else {
        LogError()()("Continuing without server side nymbox hash.").Flush();

        return true;
    }

    if (false == context.HaveRemoteNymboxHash()) {
        LogError()()("Continuing without client side nymbox hash.").Flush();

        return true;
    }

    return context.NymboxHashMatch();
}

auto UserCommandProcessor::initialize_request_number(
    otx::context::Client& context) const -> RequestNumber
{
    auto requestNumber = context.Request();

    if (0 == requestNumber) {
        LogError()()("Request number doesn't exist.").Flush();

        requestNumber = context.IncrementRequest();

        assert_true(1 == requestNumber);
    }

    return requestNumber;
}

auto UserCommandProcessor::isAdmin(
    const api::Session& api,
    const identifier::Nym& nymID) -> bool
{
    const auto adminNym = ServerSettings::GetOverrideNymID();

    if (adminNym.empty()) { return false; }

    return (0 == adminNym.compare(String::Factory(nymID, api.Crypto())->Get()));
}

auto UserCommandProcessor::load_inbox(
    const identifier::Nym& nymID,
    const identifier::Account& accountID,
    const identifier::Notary& serverID,
    const identity::Nym& serverNym,
    const bool verifyAccount) const -> std::unique_ptr<Ledger>
{
    if (accountID == nymID) {
        LogError()()("Invalid account ID ")(accountID, api_.Crypto())(".")
            .Flush();

        return {};
    }

    auto inbox{
        api_.Factory().Internal().Session().Ledger(nymID, accountID, serverID)};

    if (false == bool(inbox)) {
        LogError()()("Unable to instantiate inbox for ")(nymID, api_.Crypto())(
            ".")
            .Flush();

        return {};
    }

    if (false == inbox->LoadInbox()) {
        LogError()()("Unable to load inbox for ")(nymID, api_.Crypto())(".")
            .Flush();

        return {};
    }

    if (false == verify_box(nymID, *inbox, serverNym, verifyAccount)) {
        LogError()()("Unable to verify inbox for ")(nymID, api_.Crypto())(".")
            .Flush();

        return {};
    }

    auto notUsed = identifier::Generic{};

    if (inbox->LoadedLegacyData()) { save_inbox(serverNym, notUsed, *inbox); }

    return inbox;
}

auto UserCommandProcessor::load_nymbox(
    const identifier::Nym& nymID,
    const identifier::Notary& serverID,
    const identity::Nym& serverNym,
    const bool verifyAccount) const -> std::unique_ptr<Ledger>
{
    auto nymbox{
        api_.Factory().Internal().Session().Ledger(nymID, nymID, serverID)};

    if (false == bool(nymbox)) {
        LogError()()("Unable to instantiate nymbox for ")(nymID, api_.Crypto())(
            ".")
            .Flush();

        return {};
    }

    if (false == nymbox->LoadNymbox()) {
        LogError()()("Unable to load nymbox for ")(nymID, api_.Crypto())(".")
            .Flush();

        return {};
    }

    if (false == verify_box(nymID, *nymbox, serverNym, verifyAccount)) {
        LogError()()("Unable to verify nymbox for ")(nymID, api_.Crypto())(".")
            .Flush();

        return {};
    }

    auto notUsed = identifier::Generic{};

    if (nymbox->LoadedLegacyData()) {
        save_nymbox(serverNym, notUsed, *nymbox);
    }

    return nymbox;
}

auto UserCommandProcessor::load_outbox(
    const identifier::Nym& nymID,
    const identifier::Account& accountID,
    const identifier::Notary& serverID,
    const identity::Nym& serverNym,
    const bool verifyAccount) const -> std::unique_ptr<Ledger>
{
    if (accountID == nymID) {
        LogError()()("Invalid account ID ")(accountID, api_.Crypto())(".")
            .Flush();

        return {};
    }

    auto outbox{
        api_.Factory().Internal().Session().Ledger(nymID, accountID, serverID)};

    if (false == bool(outbox)) {
        LogError()()("Unable to instantiate outbox for ")(nymID, api_.Crypto())(
            ".")
            .Flush();

        return {};
    }

    if (false == outbox->LoadOutbox()) {
        LogError()()("Unable to load outbox for ")(nymID, api_.Crypto())(".")
            .Flush();

        return {};
    }

    if (false == verify_box(nymID, *outbox, serverNym, verifyAccount)) {
        LogError()()("Unable to verify outbox for ")(nymID, api_.Crypto())(".")
            .Flush();

        return {};
    }

    auto notUsed = identifier::Generic{};

    if (outbox->LoadedLegacyData()) {
        save_outbox(serverNym, notUsed, *outbox);
    }

    return outbox;
}

auto UserCommandProcessor::ProcessUserCommand(
    const Message& msgIn,
    Message& msgOut) -> bool
{
    UnallocatedCString command(msgIn.command_->Get());
    const auto type = Message::Type(command);
    ReplyMessage reply(
        *this,
        server_.API().Wallet(),
        server_.GetServerID(),
        server_.GetServerNym(),
        msgIn,
        server_,
        type,
        msgOut,
        reason_);

    if (false == reply.Init()) {
        LogError()()("Failed to instantiate reply").Flush();

        return false;
    }

    LogConsole()("*** Received a ")(command)(" message. Nym: ")(
        msgIn.nym_id_.get())
        .Flush();

    switch (type) {
        case otx::MessageType::pingNotary: {
            return cmd_ping_notary(reply);
        }
        case otx::MessageType::registerNym: {
            return cmd_register_nym(reply);
        }
        default: {
        }
    }

    if (false == reply.LoadContext(reason_)) {
        LogError()()("Failed to load context").Flush();

        return false;
    }

    if (false == check_client_nym(reply)) { return false; }

    assert_true(reply.HaveContext());

    auto& context = reply.Context();
    context.SetRemoteNymboxHash(
        api_.Factory().IdentifierFromBase58(msgIn.nymbox_hash_->Bytes()));

    // ENTERING THE INNER SANCTUM OF SECURITY. If the user got all the way to
    // here, Then he has passed multiple levels of security, and all commands
    // below will assume the Nym is secure, validated, and loaded into memory
    // for use. But still need to verify the Request Number for all other
    // commands except Get Request Number itself... Request numbers start at 100
    // (currently). (Since certain special messages USE 1 already... Such as
    // messages that occur before requestnumbers are possible, like
    // RegisterNym.)
    auto requestNumber = initialize_request_number(context);

    // At this point, I now have the current request number for this nym in
    // requestNumber Let's compare it to the one that was sent in the message...
    // (This prevents attackers from repeat-sending intercepted messages to the
    // server.)
    if (otx::MessageType::getRequestNumber != type) {
        if (false == check_request_number(msgIn, requestNumber)) {

            return false;
        }

        if (false == check_usage_credits(reply)) { return false; }

        context.IncrementRequest();
    }

    // At this point, we KNOW that it is EITHER a GetRequestNumber command,
    // which doesn't require a request number, OR it was some other command, but
    // the request number they sent in the command MATCHES the one that we just
    // read out of the file.

    // Therefore, we can process ALL messages below this point KNOWING that the
    // Nym is properly verified in all ways. No messages need to worry about
    // verifying the Nym, or about dealing with the Request Number. It's all
    // handled in here.
    check_acknowledgements(reply);

    switch (type) {
        case otx::MessageType::getRequestNumber: {
            return cmd_get_request_number(reply);
        }
        case otx::MessageType::getTransactionNumbers: {
            return cmd_get_transaction_numbers(reply);
        }
        case otx::MessageType::checkNym: {
            return cmd_check_nym(reply);
        }
        case otx::MessageType::sendNymMessage: {
            return cmd_send_nym_message(reply);
        }
        case otx::MessageType::unregisterNym: {
            return cmd_delete_user(reply);
        }
        case otx::MessageType::unregisterAccount: {
            return cmd_delete_asset_account(reply);
        }
        case otx::MessageType::registerAccount: {
            return cmd_register_account(reply);
        }
        case otx::MessageType::registerInstrumentDefinition: {
            return cmd_register_instrument_definition(reply);
        }
        case otx::MessageType::issueBasket: {
            return cmd_issue_basket(reply);
        }
        case otx::MessageType::notarizeTransaction: {
            return cmd_notarize_transaction(reply);
        }
        case otx::MessageType::getNymbox: {
            return cmd_get_nymbox(reply);
        }
        case otx::MessageType::getBoxReceipt: {
            return cmd_get_box_receipt(reply);
        }
        case otx::MessageType::getAccountData: {
            return cmd_get_account_data(reply);
        }
        case otx::MessageType::processNymbox: {
            return cmd_process_nymbox(reply);
        }
        case otx::MessageType::processInbox: {
            return cmd_process_inbox(reply);
        }
        case otx::MessageType::queryInstrumentDefinitions: {
            return cmd_query_instrument_definitions(reply);
        }
        case otx::MessageType::getInstrumentDefinition: {
            return cmd_get_instrument_definition(reply);
        }
        case otx::MessageType::getMint: {
            return cmd_get_mint(reply);
        }
        case otx::MessageType::getMarketList: {
            return cmd_get_market_list(reply);
        }
        case otx::MessageType::getMarketOffers: {
            return cmd_get_market_offers(reply);
        }
        case otx::MessageType::getMarketRecentTrades: {
            return cmd_get_market_recent_trades(reply);
        }
        case otx::MessageType::getNymMarketOffers: {
            return cmd_get_nym_market_offers(reply);
        }
        case otx::MessageType::triggerClause: {
            return cmd_trigger_clause(reply);
        }
        case otx::MessageType::usageCredits: {
            return cmd_usage_credits(reply);
        }
        case otx::MessageType::registerContract: {
            return cmd_register_contract(reply);
        }
        case otx::MessageType::requestAdmin: {
            return cmd_request_admin(reply);
        }
        case otx::MessageType::addClaim: {
            return cmd_add_claim(reply);
        }
        default: {
            LogError()()("Unknown command type: ")(command)(".").Flush();

            reply.SetAccount(msgIn.acct_id_);
            command.append("Response");
            auto response = String::Factory(command);
            reply.OverrideType(response);

            return false;
        }
    }
}

auto UserCommandProcessor::reregister_nym(ReplyMessage& reply) const -> bool
{
    auto& context = reply.Context();
    context.IncrementRequest();
    const auto& msgIn = reply.Original();
    LogDebug()()("Re-registering nym: ")(msgIn.nym_id_.get()).Flush();
    const auto& nym = reply.Context().RemoteNym();
    const auto& serverNym = *context.Signer();
    const auto& serverID = context.Notary();
    const auto& targetNymID = nym.ID();
    auto nymbox = load_nymbox(targetNymID, serverID, serverNym, false);

    if (false == bool(nymbox)) {
        context.InitializeNymbox(reason_);
        nymbox = load_nymbox(targetNymID, serverID, serverNym, false);
        context.SetRemoteNymboxHash(context.LocalNymboxHash());
    }

    if (false == bool(nymbox)) {
        LogError()()(
            "Error during nym re-registration. Failed verifying or generating "
            "nymbox for Nym: ")(msgIn.nym_id_.get())(".")
            .Flush();

        return false;
    }

    reply.SetPayload(api_.Factory().Internal().Data([&] {
        auto proto = protobuf::Context{};
        context.Refresh(proto, reason_);

        return proto;
    }()));
    reply.SetSuccess(true);

    return true;
}

auto UserCommandProcessor::save_box(const identity::Nym& nym, Ledger& box) const
    -> bool
{
    box.ReleaseSignatures();

    if (false == box.SignContract(nym, reason_)) { return false; }

    return box.SaveContract();
}

auto UserCommandProcessor::save_inbox(
    const identity::Nym& nym,
    identifier::Generic& hash,
    Ledger& inbox) const -> bool
{
    if (false == save_box(nym, inbox)) { return false; }

    if (false == inbox.SaveInbox(hash)) { return false; }

    return true;
}

auto UserCommandProcessor::save_nymbox(
    const identity::Nym& nym,
    identifier::Generic& hash,
    Ledger& nymbox) const -> bool
{
    if (false == save_box(nym, nymbox)) { return false; }

    if (false == nymbox.SaveNymbox(hash)) { return false; }

    return true;
}

auto UserCommandProcessor::save_outbox(
    const identity::Nym& nym,
    identifier::Generic& hash,
    Ledger& outbox) const -> bool
{
    if (false == save_box(nym, outbox)) { return false; }

    if (false == outbox.SaveOutbox(hash)) { return false; }

    return true;
}

auto UserCommandProcessor::verify_box(
    const identifier::Generic& ownerID,
    Ledger& box,
    const identity::Nym& nym,
    const bool full) const -> bool
{
    if (false == box.VerifyContractID()) {
        LogError()()("Unable to verify box ID for ")(ownerID, api_.Crypto())(
            ".")
            .Flush();

        return false;
    }

    if (full) {
        if (false == box.VerifyAccount(nym)) {
            LogError()()("Unable to verify box for ")(ownerID, api_.Crypto())(
                ".")
                .Flush();

            return false;
        }
    } else {
        if (false == box.VerifySignature(nym)) {
            LogError()()("Unable to verify signature for ")(
                ownerID, api_.Crypto())(".")
                .Flush();

            return false;
        }
    }

    return true;
}

auto UserCommandProcessor::verify_transaction(
    const OTTransaction* transaction,
    const identity::Nym& signer) const -> bool
{
    if (nullptr == transaction) { return false; }

    if (transaction->IsAbbreviated()) { return false; }

    if (false == transaction->VerifyContractID()) { return false; }

    return transaction->VerifySignature(signer);
}
}  // namespace opentxs::server
