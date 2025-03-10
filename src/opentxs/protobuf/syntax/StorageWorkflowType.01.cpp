// Copyright (c) 2010-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "opentxs/protobuf/syntax/StorageWorkflowType.hpp"  // IWYU pragma: associated

#include <opentxs/protobuf/PaymentWorkflowEnums.pb.h>
#include <opentxs/protobuf/StorageWorkflowType.pb.h>
#include <cstdint>

#include "opentxs/protobuf/syntax/Macros.hpp"

namespace opentxs::protobuf::inline syntax
{
auto version_1(const StorageWorkflowType& input, const Log& log) -> bool
{
    CHECK_IDENTIFIER(workflow);

    switch (input.type()) {
        case PAYMENTWORKFLOWTYPE_OUTGOINGCHEQUE: {
            switch (input.state()) {
                case PAYMENTWORKFLOWSTATE_UNSENT:
                case PAYMENTWORKFLOWSTATE_CONVEYED:
                case PAYMENTWORKFLOWSTATE_CANCELLED:
                case PAYMENTWORKFLOWSTATE_ACCEPTED:
                case PAYMENTWORKFLOWSTATE_COMPLETED:
                case PAYMENTWORKFLOWSTATE_EXPIRED: {
                } break;
                case PAYMENTWORKFLOWSTATE_ABORTED:
                case PAYMENTWORKFLOWSTATE_ACKNOWLEDGED:
                case PAYMENTWORKFLOWSTATE_REJECTED:
                case PAYMENTWORKFLOWSTATE_INITIATED:
                case PAYMENTWORKFLOWSTATE_ERROR:
                default: {
                    FAIL_2(
                        "invalid state",
                        static_cast<std::uint32_t>(input.state()));
                }
            }
        } break;
        case PAYMENTWORKFLOWTYPE_INCOMINGCHEQUE: {
            switch (input.state()) {
                case PAYMENTWORKFLOWSTATE_CONVEYED:
                case PAYMENTWORKFLOWSTATE_COMPLETED:
                case PAYMENTWORKFLOWSTATE_EXPIRED: {
                } break;
                case PAYMENTWORKFLOWSTATE_INITIATED:
                case PAYMENTWORKFLOWSTATE_UNSENT:
                case PAYMENTWORKFLOWSTATE_CANCELLED:
                case PAYMENTWORKFLOWSTATE_ACCEPTED:
                case PAYMENTWORKFLOWSTATE_ABORTED:
                case PAYMENTWORKFLOWSTATE_ACKNOWLEDGED:
                case PAYMENTWORKFLOWSTATE_REJECTED:
                case PAYMENTWORKFLOWSTATE_ERROR:
                default: {
                    FAIL_2(
                        "invalid state",
                        static_cast<std::uint32_t>(input.state()));
                }
            }
        } break;
        case PAYMENTWORKFLOWTYPE_OUTGOINGINVOICE: {
            switch (input.state()) {
                case PAYMENTWORKFLOWSTATE_UNSENT:
                case PAYMENTWORKFLOWSTATE_CONVEYED:
                case PAYMENTWORKFLOWSTATE_CANCELLED:
                case PAYMENTWORKFLOWSTATE_ACCEPTED:
                case PAYMENTWORKFLOWSTATE_COMPLETED:
                case PAYMENTWORKFLOWSTATE_EXPIRED: {
                } break;
                case PAYMENTWORKFLOWSTATE_INITIATED:
                case PAYMENTWORKFLOWSTATE_ABORTED:
                case PAYMENTWORKFLOWSTATE_ACKNOWLEDGED:
                case PAYMENTWORKFLOWSTATE_REJECTED:
                case PAYMENTWORKFLOWSTATE_ERROR:
                default: {
                    FAIL_2(
                        "invalid state",
                        static_cast<std::uint32_t>(input.state()));
                }
            }
        } break;
        case PAYMENTWORKFLOWTYPE_INCOMINGINVOICE: {
            switch (input.state()) {
                case PAYMENTWORKFLOWSTATE_CONVEYED:
                case PAYMENTWORKFLOWSTATE_COMPLETED:
                case PAYMENTWORKFLOWSTATE_EXPIRED: {
                } break;
                case PAYMENTWORKFLOWSTATE_INITIATED:
                case PAYMENTWORKFLOWSTATE_UNSENT:
                case PAYMENTWORKFLOWSTATE_CANCELLED:
                case PAYMENTWORKFLOWSTATE_ACCEPTED:
                case PAYMENTWORKFLOWSTATE_ABORTED:
                case PAYMENTWORKFLOWSTATE_ACKNOWLEDGED:
                case PAYMENTWORKFLOWSTATE_REJECTED:
                case PAYMENTWORKFLOWSTATE_ERROR:
                default: {
                    FAIL_2(
                        "invalid state",
                        static_cast<std::uint32_t>(input.state()));
                }
            }
        } break;
        case PAYMENTWORKFLOWTYPE_OUTGOINGTRANSFER:
        case PAYMENTWORKFLOWTYPE_INCOMINGTRANSFER:
        case PAYMENTWORKFLOWTYPE_INTERNALTRANSFER:
        case PAYMENTWORKFLOWTYPE_OUTGOINGCASH:
        case PAYMENTWORKFLOWTYPE_INCOMINGCASH:
        case PAYMENTWORKFLOWTYPE_ERROR:
        default: {
            FAIL_2("invalid type", static_cast<std::uint32_t>(input.type()));
        }
    }

    return true;
}
}  // namespace opentxs::protobuf::inline syntax

#include "opentxs/protobuf/syntax/Macros.undefine.inc"  // IWYU pragma: keep
