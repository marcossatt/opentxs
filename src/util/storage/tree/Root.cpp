// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "util/storage/tree/Root.hpp"  // IWYU pragma: associated

#include <opentxs/protobuf/StorageRoot.pb.h>
#include <functional>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <utility>

#include "internal/util/DeferredConstruction.hpp"
#include "internal/util/storage/drivers/Plugin.hpp"
#include "opentxs/Time.hpp"
#include "opentxs/protobuf/Types.internal.tpp"
#include "opentxs/protobuf/syntax/StorageRoot.hpp"
#include "opentxs/protobuf/syntax/Types.internal.tpp"
#include "opentxs/storage/Types.internal.hpp"
#include "opentxs/util/Bytes.hpp"
#include "opentxs/util/Log.hpp"
#include "opentxs/util/Writer.hpp"
#include "opentxs/util/storage/Driver.hpp"
#include "util/storage/tree/Node.hpp"
#include "util/storage/tree/Trunk.hpp"

namespace opentxs::storage::tree
{
using namespace std::literals;

Root::Root(
    const api::Crypto& crypto,
    const api::session::Factory& factory,
    const driver::Plugin& storage,
    const Hash& hash,
    std::atomic<Bucket>& bucket)
    : Node(
          crypto,
          factory,
          storage,
          hash,
          std::source_location::current().function_name(),
          current_version_)
    , current_bucket_(bucket)
    , sequence_(0)
    , gc_params_()
    , tree_root_(NullHash{})
    , tree_lock_()
    , tree_()
{
    if (is_valid(hash)) {
        init(hash);
    } else {
        blank();
    }
}

auto Root::blank() noexcept -> void
{
    Node::blank();
    current_bucket_.store(Bucket::left);
    gc_params_.last_ = Clock::now();
    gc_params_.from_ = next(current_bucket_.load());
}

auto Root::CheckSequence(
    const Log& log,
    const Hash& hash,
    const storage::Driver& driver) noexcept -> std::uint64_t
{
    try {
        auto bytes = ""s;
        auto write = writer(bytes);
        const auto loaded = driver.Load(log, hash, {}, write);

        if (false == loaded) {

            throw std::runtime_error{
                "failed to load hash "s.append(to_string(hash))
                    .append(" using ")
                    .append(driver.Description())
                    .append(" driver")};
        }

        const auto proto = protobuf::Factory<protobuf::StorageRoot>(
            bytes.data(), bytes.size());

        return proto.sequence();
    } catch (const std::exception& e) {
        LogError()()(e.what()).Flush();

        return {};
    }
}

auto Root::dump(const Lock& lock, const Log& log, Vector<Hash>& out)
    const noexcept -> bool
{
    if (false == is_valid(root_)) { return true; }

    if (false == Node::dump(lock, log, out)) { return false; }

    if (is_valid(tree_root_)) {
        if (false == trunk()->dump(lock, log, out)) { return false; }
    }

    return true;
}

auto Root::FinishGC(bool success) noexcept -> void
{
    auto lock = Lock{write_lock_};
    gc_params_.running_ = false;

    if (success) { gc_params_.last_ = Clock::now(); }

    save(lock);
}

auto Root::GCStatus() const noexcept -> GCParams
{
    auto lock = Lock{write_lock_};

    return gc_params_;
}

auto Root::init(const Hash& hash) noexcept(false) -> void
{
    auto p = std::shared_ptr<protobuf::StorageRoot>{};

    if (LoadProto(hash, p, verbose) && p) {
        const auto& proto = *p;

        switch (set_original_version(proto.version())) {
            case 3u:
            case 2u:
            case 1u:
            default: {
                current_bucket_.store(static_cast<Bucket>(proto.altlocation()));
                sequence_.store(proto.sequence());
                tree_root_ = read(proto.items());
                gc_params_.running_ = proto.gc();
                gc_params_.last_ =
                    seconds_since_epoch_unsigned(proto.lastgc()).value();
                gc_params_.root_ = read(proto.gcroot());
                gc_params_.from_ = next(current_bucket_.load());
            }
        }
    } else {
        throw std::runtime_error{"failed to load root object file in "s.append(
            std::source_location::current().function_name())};
    }
}

auto Root::mutable_Trunk() -> Editor<tree::Trunk>
{
    const std::function<void(tree::Trunk*, Lock&)> callback =
        [&](tree::Trunk* in, Lock& lock) -> void { this->save(in, lock); };

    return {write_lock_, trunk(), callback};
}

auto Root::save(const Lock& lock) const -> bool
{
    assert_true(verify_write_lock(lock));

    sequence_++;
    auto serialized = serialize(lock);

    if (false == protobuf::syntax::check(LogError(), serialized)) {
        return false;
    }

    return StoreProto(serialized, root_);
}

auto Root::save(tree::Trunk* tree, const Lock& lock) -> void
{
    assert_true(verify_write_lock(lock));
    assert_false(nullptr == tree);

    auto treeLock = Lock{tree_lock_};
    tree_root_ = tree->root_;
    treeLock.unlock();

    const bool saved = save(lock);

    assert_true(saved);
}

auto Root::Sequence() const -> std::uint64_t { return sequence_.load(); }

auto Root::serialize(const Lock&) const -> protobuf::StorageRoot
{
    auto output = protobuf::StorageRoot{};
    output.set_version(version_);
    write(tree_root_, *output.mutable_items());
    output.set_altlocation(static_cast<bool>(current_bucket_.load()));
    output.set_sequence(sequence_);
    output.set_lastgc(seconds_since_epoch(gc_params_.last_).value());
    output.set_gc(gc_params_.running_);
    write(gc_params_.root_, *output.mutable_gcroot());

    return output;
}

auto Root::StartGC() noexcept -> std::optional<GCParams>
{
    auto lock = Lock{write_lock_};
    const auto original{gc_params_};

    if (auto running = std::exchange(gc_params_.running_, true); running) {

        return std::nullopt;
    }

    gc_params_.root_ = plugin_.LoadRoot();
    gc_params_.from_ = current_bucket_.load();
    current_bucket_.store(next(gc_params_.from_));

    if (save(lock)) {

        return gc_params_;
    } else {
        gc_params_ = original;

        return std::nullopt;
    }
}

auto Root::trunk() const -> tree::Trunk*
{
    auto lock = Lock{tree_lock_};

    if (!tree_) {
        tree_.reset(new tree::Trunk(crypto_, factory_, plugin_, tree_root_));
    }

    assert_false(nullptr == tree_);

    lock.unlock();

    return tree_.get();
}

auto Root::Trunk() const -> const tree::Trunk& { return *trunk(); }

auto Root::upgrade(const Lock& lock) noexcept -> bool
{
    auto changed = Node::upgrade(lock);

    switch (original_version_.get()) {
        case 1u:
        case 2u:
        case 3u:
        default: {
        }
    }

    if (is_valid(tree_root_)) {
        if (auto* node = trunk(); node->Upgrade()) {
            tree_root_ = node->root_;
            changed = true;
        }
    }

    return changed;
}

Root::~Root() = default;
}  // namespace opentxs::storage::tree
