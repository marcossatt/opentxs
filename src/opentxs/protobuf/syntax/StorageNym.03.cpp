// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/protobuf/syntax/StorageNym.hpp"  // IWYU pragma: associated

#include <opentxs/protobuf/StorageNym.pb.h>
#include <string>

#include "opentxs/protobuf/syntax/Macros.hpp"
#include "opentxs/protobuf/syntax/StorageItemHash.hpp"  // IWYU pragma: keep
#include "opentxs/protobuf/syntax/VerifyStorage.hpp"

namespace opentxs::protobuf::inline syntax
{
auto version_3(const StorageNym& input, const Log& log) -> bool
{
    OPTIONAL_SUBOBJECT(credlist, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(sentpeerrequests, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(
        incomingpeerrequests, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(sentpeerreply, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(incomingpeerreply, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(finishedpeerrequest, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(finishedpeerreply, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(
        processedpeerrequest, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(processedpeerreply, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(mailinbox, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(mailoutbox, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(threads, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(contexts, StorageNymAllowedStorageItemHash());
    OPTIONAL_SUBOBJECT(accounts, StorageNymAllowedStorageItemHash());
    CHECK_NONE(bitcoin_hd_index);
    CHECK_NONE(bitcoin_hd);
    CHECK_EXCLUDED(issuers);
    CHECK_EXCLUDED(paymentworkflow);
    CHECK_EXCLUDED(bip47);
    CHECK_NONE(purse);
    CHECK_NONE(ethereum_hd_index);
    CHECK_NONE(ethereum_hd);

    return true;
}
}  // namespace opentxs::protobuf::inline syntax

#include "opentxs/protobuf/syntax/Macros.undefine.inc"  // IWYU pragma: keep
