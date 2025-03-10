// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// IWYU pragma: no_forward_declare opentxs::protobuf::SourceProofType

#pragma once

#include <frozen/unordered_map.h>
#include <opentxs/protobuf/Enums.pb.h>
#include <opentxs/protobuf/SourceProof.pb.h>
#include <cstddef>
#include <memory>

#include "identity/credential/Base.hpp"
#include "identity/credential/Key.hpp"
#include "internal/identity/credential/Credential.hpp"
#include "opentxs/Types.internal.hpp"
#include "opentxs/identity/Types.hpp"
#include "opentxs/identity/Types.internal.hpp"
#include "opentxs/util/Container.hpp"
#include "opentxs/util/Numbers.hpp"

// NOLINTBEGIN(modernize-concat-nested-namespaces)
namespace opentxs
{
namespace api
{
class Session;
}  // namespace api

namespace crypto
{
class Parameters;
}  // namespace crypto

namespace identity
{
namespace internal
{
class Authority;
}  // namespace internal

class Source;
}  // namespace identity

namespace protobuf
{
class Credential;
class HDPath;
class Signature;
}  // namespace protobuf

class Factory;
class PasswordPrompt;
}  // namespace opentxs
// NOLINTEND(modernize-concat-nested-namespaces)

namespace opentxs::identity::credential::implementation
{
class Primary final : virtual public credential::internal::Primary,
                      credential::implementation::Key
{
public:
    auto hasCapability(const NymCapability& capability) const -> bool final;
    auto Path(protobuf::HDPath& output) const -> bool final;
    auto Path() const -> UnallocatedCString final;
    using Base::Verify;
    auto Verify(
        const protobuf::Credential& credential,
        const identity::CredentialRole& role,
        const identifier_type& masterID,
        const protobuf::Signature& masterSig) const -> bool final;

    Primary() = delete;
    Primary(const Primary&) = delete;
    Primary(Primary&&) = delete;
    auto operator=(const Primary&) -> Primary& = delete;
    auto operator=(Primary&&) -> Primary& = delete;

    ~Primary() final = default;

private:
    friend opentxs::Factory;

    static constexpr auto SourceProofTypeMapSize = std::size_t{3};
    using SourceProofTypeMap = frozen::unordered_map<
        identity::SourceProofType,
        protobuf::SourceProofType,
        SourceProofTypeMapSize>;
    using SourceProofTypeReverseMap = frozen::unordered_map<
        protobuf::SourceProofType,
        identity::SourceProofType,
        SourceProofTypeMapSize>;

    static const VersionConversionMap credential_to_master_params_;

    const protobuf::SourceProof source_proof_;

    static auto source_proof(const crypto::Parameters& params)
        -> protobuf::SourceProof;
    static auto sourceprooftype_map() noexcept -> const SourceProofTypeMap&;
    static auto translate(const identity::SourceProofType in) noexcept
        -> protobuf::SourceProofType;
    static auto translate(const protobuf::SourceProofType in) noexcept
        -> identity::SourceProofType;

    auto serialize(
        const SerializationModeFlag asPrivate,
        const SerializationSignatureFlag asSigned) const
        -> std::shared_ptr<internal::Base::SerializedType> final;
    auto verify_against_source() const -> bool;
    auto verify_internally() const -> bool final;

    auto sign(
        const identity::credential::internal::Primary& master,
        const PasswordPrompt& reason,
        Signatures& out) noexcept(false) -> void final;

    Primary(
        const api::Session& api,
        const identity::internal::Authority& parent,
        const identity::Source& source,
        const crypto::Parameters& nymParameters,
        const VersionNumber version,
        const PasswordPrompt& reason) noexcept(false);
    Primary(
        const api::Session& api,
        const identity::internal::Authority& parent,
        const identity::Source& source,
        const protobuf::Credential& serializedCred) noexcept(false);
};
}  // namespace opentxs::identity::credential::implementation
