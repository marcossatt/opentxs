// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "api/session/Crypto.hpp"  // IWYU pragma: associated

#include "internal/api/crypto/Factory.hpp"
#include "internal/util/Mutex.hpp"
#include "opentxs/api/Factory.internal.hpp"
#include "opentxs/api/crypto/Seed.hpp"
#include "opentxs/api/session/Factory.hpp"
#include "opentxs/api/session/Factory.internal.hpp"
#include "opentxs/api/session/internal.factory.hpp"
#include "opentxs/util/Log.hpp"

namespace opentxs::factory
{
auto SessionCryptoAPI(
    api::Crypto& parent,
    const api::internal::Session& session,
    const api::session::Endpoints& endpoints,
    const api::session::Factory& factory,
    const api::session::Storage& storage,
    const network::zeromq::Context& zmq) noexcept
    -> std::unique_ptr<api::session::Crypto>
{
    using ReturnType = api::session::imp::Crypto;

    return std::make_unique<ReturnType>(
        parent, session, endpoints, factory, storage, zmq);
}
}  // namespace opentxs::factory

namespace opentxs::api::session::imp
{
Crypto::Crypto(
    api::Crypto& parent,
    const api::internal::Session& session,
    const api::session::Endpoints& endpoints,
    const api::session::Factory& factory,
    const api::session::Storage& storage,
    const opentxs::network::zeromq::Context& zmq) noexcept
    : lock_()
    , blank_blockchain_(factory)
    , blockchain_()
    , parent_(parent.Internal())
    , asymmetric_(factory.Internal().Session().Asymmetric())
    , symmetric_(factory.Internal().Session().Symmetric())
    , seed_p_(factory::SeedAPI(
          session,
          endpoints,
          factory,
          asymmetric_,
          symmetric_,
          storage,
          parent_.BIP32(),
          parent_.BIP39(),
          zmq))
    , seed_(*seed_p_)
{
    assert_false(nullptr == seed_p_);
}

auto Crypto::Blockchain() const noexcept -> const crypto::Blockchain&
{
    auto lock = Lock{lock_};

    if (blockchain_) {

        return *blockchain_;
    } else {

        return blank_blockchain_;
    }
}

auto Crypto::Init(
    const std::shared_ptr<const crypto::Blockchain>& blockchain) noexcept
    -> void
{
    auto lock = Lock{lock_};
    blockchain_ = blockchain;
}

auto Crypto::Cleanup() noexcept -> void { seed_p_.reset(); }

auto Crypto::PrepareShutdown() noexcept -> void
{
    auto lock = Lock{lock_};
    blockchain_.reset();
}

Crypto::~Crypto() = default;
}  // namespace opentxs::api::session::imp
