# Copyright (c) 2010-2022 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

if(OT_BLOCKCHAIN_EXPORT)
  target_sources(
    opentxs-common
    PRIVATE
      "${opentxs_SOURCE_DIR}/src/internal/network/blockchain/Peer.hpp"
      "HasJob.cpp"
      "HasJob.hpp"
      "Imp.cpp"
      "Imp.hpp"
      "JobType.cpp"
      "JobType.hpp"
      "Peer.cpp"
      "RunJob.cpp"
      "RunJob.hpp"
      "UpdateBlockJob.cpp"
      "UpdateBlockJob.hpp"
      "UpdateGetHeadersJob.cpp"
      "UpdateGetHeadersJob.hpp"
  )
  libopentxs_link_external(Boost::system)
endif()
