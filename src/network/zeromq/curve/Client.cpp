// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "network/zeromq/curve/Client.hpp"  // IWYU pragma: associated

#include <zmq.h>
#include <array>
#include <cstdint>
#include <utility>

#include "internal/core/contract/ServerContract.hpp"
#include "internal/util/Mutex.hpp"
#include "network/zeromq/socket/Socket.hpp"
#include "opentxs/core/Data.hpp"
#include "opentxs/network/zeromq/Types.hpp"
#include "opentxs/util/Bytes.hpp"
#include "opentxs/util/Log.hpp"
#include "opentxs/util/Writer.hpp"

namespace opentxs::network::zeromq
{
auto curve::Client::RandomKeypair() noexcept
    -> std::pair<UnallocatedCString, UnallocatedCString>
{
    auto output = std::pair<UnallocatedCString, UnallocatedCString>{};
    auto& [privKey, pubKey] = output;

    if (false == CurveKeypairZ85(writer(privKey), writer(pubKey))) {
        LogError()()("Failed to generate keypair").Flush();
    }

    return output;
}
}  // namespace opentxs::network::zeromq

namespace opentxs::network::zeromq::curve::implementation
{
Client::Client(socket::implementation::Socket& socket) noexcept
    : parent_(socket)
{
}

auto Client::SetKeysZ85(
    const UnallocatedCString& serverPublic,
    const UnallocatedCString& clientPrivate,
    const UnallocatedCString& clientPublic) const noexcept -> bool
{
    if (CURVE_KEY_Z85_BYTES > serverPublic.size()) {
        LogError()()("Invalid server key size (")(serverPublic.size())(").")
            .Flush();

        return false;
    }

    std::array<std::uint8_t, CURVE_KEY_BYTES> key{};
    ::zmq_z85_decode(key.data(), serverPublic.data());

    if (false == set_remote_key(key.data(), key.size())) {
        LogError()()("Failed to set server key.").Flush();

        return false;
    }

    return set_local_keys(clientPrivate, clientPublic);
}

auto Client::SetServerPubkey(const contract::Server& contract) const noexcept
    -> bool
{
    return set_public_key(contract);
}

auto Client::SetServerPubkey(const Data& key) const noexcept -> bool
{
    return set_public_key(key);
}

auto Client::set_public_key(const contract::Server& contract) const noexcept
    -> bool
{
    const auto& key = contract.TransportKey();

    if (CURVE_KEY_BYTES != key.size()) {
        LogError()()("Invalid server key.").Flush();

        return false;
    }

    return set_public_key(key);
}

auto Client::set_public_key(const Data& key) const noexcept -> bool
{
    if (false == set_remote_key(key.data(), key.size())) { return false; }

    return set_local_keys();
}

auto Client::set_local_keys() const noexcept -> bool
{
    assert_false(nullptr == parent_);

    const auto [secretKey, publicKey] = RandomKeypair();

    if (secretKey.empty() || publicKey.empty()) {
        LogError()()("Failed to generate keypair.").Flush();

        return false;
    }

    return set_local_keys(secretKey, publicKey);
}

auto Client::set_local_keys(
    const UnallocatedCString& privateKey,
    const UnallocatedCString& publicKey) const noexcept -> bool
{
    assert_false(nullptr == parent_);

    if (CURVE_KEY_Z85_BYTES > privateKey.size()) {
        LogError()()("Invalid private key size (")(privateKey.size())(").")
            .Flush();

        return false;
    }

    std::array<std::uint8_t, CURVE_KEY_BYTES> privateDecoded{};
    ::zmq_z85_decode(privateDecoded.data(), privateKey.data());

    if (CURVE_KEY_Z85_BYTES > publicKey.size()) {
        LogError()()("Invalid public key size (")(publicKey.size())(").")
            .Flush();

        return false;
    }

    std::array<std::uint8_t, CURVE_KEY_BYTES> publicDecoded{};
    ::zmq_z85_decode(publicDecoded.data(), publicKey.data());

    return set_local_keys(
        privateDecoded.data(),
        privateDecoded.size(),
        publicDecoded.data(),
        publicDecoded.size());
}

auto Client::set_local_keys(
    const void* privateKey,
    const std::size_t privateKeySize,
    const void* publicKey,
    const std::size_t publicKeySize) const noexcept -> bool
{
    assert_false(nullptr == parent_);

    socket::implementation::Socket::SocketCallback cb{[&](const Lock&) -> bool {
        auto set = zmq_setsockopt(
            parent_, ZMQ_CURVE_SECRETKEY, privateKey, privateKeySize);

        if (0 != set) {
            LogError()()("Failed to set private key.").Flush();

            return false;
        }

        set = zmq_setsockopt(
            parent_, ZMQ_CURVE_PUBLICKEY, publicKey, publicKeySize);

        if (0 != set) {
            LogError()()("Failed to set public key.").Flush();

            return false;
        }

        return true;
    }};

    return parent_.apply_socket(std::move(cb));
}

auto Client::set_remote_key(const void* key, const std::size_t size)
    const noexcept -> bool
{
    assert_false(nullptr == parent_);

    socket::implementation::Socket::SocketCallback cb{[&](const Lock&) -> bool {
        const auto set =
            zmq_setsockopt(parent_, ZMQ_CURVE_SERVERKEY, key, size);

        if (0 != set) {
            LogError()()("Failed to set server key.").Flush();

            return false;
        }

        return true;
    }};

    return parent_.apply_socket(std::move(cb));
}
}  // namespace opentxs::network::zeromq::curve::implementation
