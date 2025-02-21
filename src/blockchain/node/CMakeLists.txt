# Copyright (c) 2010-2022 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(blockoracle)
add_subdirectory(filteroracle)
add_subdirectory(headeroracle)
add_subdirectory(manager)
add_subdirectory(spend)
add_subdirectory(stats)
add_subdirectory(wallet)

if(OT_BLOCKCHAIN_EXPORT)
  add_subdirectory(peermanager)
endif()

if(OT_BLOCKCHAIN_EXPORT)
  target_sources(
    opentxs-common
    PRIVATE
      "${opentxs_SOURCE_DIR}/src/internal/blockchain/node/Config.hpp"
      "${opentxs_SOURCE_DIR}/src/internal/blockchain/node/Endpoints.hpp"
      "${opentxs_SOURCE_DIR}/src/internal/blockchain/node/Factory.hpp"
      "${opentxs_SOURCE_DIR}/src/internal/blockchain/node/Job.hpp"
      "${opentxs_SOURCE_DIR}/src/internal/blockchain/node/Mempool.hpp"
      "${opentxs_SOURCE_DIR}/src/internal/blockchain/node/SpendPolicy.hpp"
      "Config.cpp"
      "Downloader.cpp"
      "Downloader.hpp"
      "Endpoints.cpp"
      "Job.cpp"
      "Mempool.cpp"
      "Mempool.hpp"
      "UpdateTransaction.cpp"
      "UpdateTransaction.hpp"
  )
  libopentxs_link_external(Boost::system)

  if(PACKETCRYPT_EXPORT)
    target_sources(
      opentxs-common PRIVATE "blockoracle/GetValidator_packetcrypt.cpp"
    )
  else()
    target_sources(opentxs-common PRIVATE "blockoracle/GetValidator.cpp")
  endif()
endif()
