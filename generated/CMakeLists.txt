# Copyright (c) 2010-2022 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

add_subdirectory(blockchain)
if(CASH_LUCRE_EXPORT)
  add_subdirectory(lucre)
endif()
add_subdirectory(license)
add_subdirectory(mapped)
add_subdirectory(size)
add_subdirectory(tbb)
add_subdirectory(tests)

if(BIP32_EXPORT)
  set(OPENTXS_HAVE_HD_KEYS "true")
  set(OPENTXS_DEFAULT_CRED_TYPE "HD")

  if(SECP256K1_EXPORT)
    set(OPENTXS_DEFAULT_SOURCE_TYPE "Bip47")
  else()
    set(OPENTXS_DEFAULT_SOURCE_TYPE "PubKey")
  endif()
else()
  set(OPENTXS_HAVE_HD_KEYS "false")
  set(OPENTXS_DEFAULT_CRED_TYPE "Legacy")
  set(OPENTXS_DEFAULT_SOURCE_TYPE "PubKey")
endif()

if(SECP256K1_EXPORT)
  set(OPENTXS_DEFAULT_KEY_TYPE "secp256k1")
elseif(ED25519_EXPORT)
  set(OPENTXS_DEFAULT_KEY_TYPE "ed25519")
elseif(RSA_EXPORT)
  set(OPENTXS_DEFAULT_KEY_TYPE "rsa")
else()
  set(OPENTXS_DEFAULT_KEY_TYPE "error")
endif()

if(SECP256K1_EXPORT)
  set(OPENTXS_CRYPTO_HAVE_SECP256K1 "true")
else()
  set(OPENTXS_CRYPTO_HAVE_SECP256K1 "false")
endif()

if(ED25519_EXPORT)
  set(OPENTXS_CRYPTO_HAVE_ED25519 "true")
else()
  set(OPENTXS_CRYPTO_HAVE_ED25519 "false")
endif()

if(RSA_EXPORT)
  set(OPENTXS_CRYPTO_HAVE_RSA "true")
else()
  set(OPENTXS_CRYPTO_HAVE_RSA "false")
endif()

if(MSVC)
  set(OPENTXS_EXPORT_TEMPLATE_PARAM "")
else()
  set(OPENTXS_EXPORT_TEMPLATE_PARAM "OPENTXS_EXPORT")
endif()

configure_file(
  "StorageLevel.cpp.in"
  "${opentxs_BINARY_DIR}/src/blockchain/database/common/StorageLevel.cpp"
  @ONLY
)

configure_file(
  "NymParameters.cpp.in"
  "${opentxs_BINARY_DIR}/src/crypto/Parameters_generated.cpp"
  @ONLY
)

configure_file(
  "CryptoConfig.cpp.in"
  "${opentxs_BINARY_DIR}/src/api/crypto/Config_generated.cpp"
  @ONLY
)

if(APPLE)
  configure_file(
    "BoostStacktrace.hpp.apple"
    "${opentxs_BINARY_DIR}/external/BoostStacktrace.hpp"
    @ONLY
  )
else()
  configure_file(
    "BoostStacktrace.hpp.other"
    "${opentxs_BINARY_DIR}/external/BoostStacktrace.hpp"
    @ONLY
  )
endif()

target_include_directories(
  opentxs-common PRIVATE "${opentxs_BINARY_DIR}/external"
)
