// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "opentxs/rpc/response/Message.hpp"  // IWYU pragma: associated

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace protobuf
{
class RPCResponse;
}  // namespace protobuf

namespace rpc
{
namespace request
{
class Message;
}  // namespace request

namespace response
{
class GetAccountActivity;
class GetAccountBalance;
class ListAccounts;
class ListNyms;
class SendPayment;
}  // namespace response
}  // namespace rpc

class Writer;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::rpc::response
{
struct Message::Imp {
    const Message* parent_;
    const VersionNumber version_;
    const UnallocatedCString cookie_;
    const CommandType type_;
    const Responses responses_;
    const SessionIndex session_;
    const Identifiers identifiers_;
    const Tasks tasks_;

    virtual auto asGetAccountActivity() const noexcept
        -> const response::GetAccountActivity&;
    virtual auto asGetAccountBalance() const noexcept
        -> const response::GetAccountBalance&;
    virtual auto asListAccounts() const noexcept
        -> const response::ListAccounts&;
    virtual auto asListNyms() const noexcept -> const response::ListNyms&;
    virtual auto asSendPayment() const noexcept -> const response::SendPayment&;

    virtual auto serialize(protobuf::RPCResponse& dest) const noexcept -> bool;
    auto serialize(Writer&& dest) const noexcept -> bool;
    auto serialize_identifiers(protobuf::RPCResponse& dest) const noexcept
        -> void;
    auto serialize_tasks(protobuf::RPCResponse& dest) const noexcept -> void;

    Imp(const Message* parent) noexcept;
    Imp(const Message* parent,
        const request::Message& request,
        Responses&& response) noexcept;
    Imp(const Message* parent,
        const request::Message& request,
        Responses&& response,
        Identifiers&& identifiers) noexcept;
    Imp(const Message* parent,
        const request::Message& request,
        Responses&& response,
        Tasks&& tasks) noexcept;
    Imp(const Message* parent,
        const protobuf::RPCResponse& serialized) noexcept;
    Imp() noexcept = delete;
    Imp(const Imp&) = delete;
    Imp(Imp&&) = delete;
    auto operator=(const Imp&) -> Imp& = delete;
    auto operator=(Imp&&) -> Imp& = delete;

    virtual ~Imp() = default;

private:
    static auto status_version(
        const CommandType& type,
        const VersionNumber parentVersion) noexcept -> VersionNumber;
    static auto task_version(
        const CommandType& type,
        const VersionNumber parentVersion) noexcept -> VersionNumber;

    Imp(const Message* parent,
        VersionNumber version,
        const UnallocatedCString& cookie,
        const CommandType& type,
        const Responses responses,
        SessionIndex session,
        Identifiers&& identifiers,
        Tasks&& tasks) noexcept;
};
}  // namespace opentxs::rpc::response
