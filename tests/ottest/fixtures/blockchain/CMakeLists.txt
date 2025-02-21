# Copyright (c) 2010-2022 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

if(OT_BLOCKCHAIN_EXPORT)
  add_subdirectory(regtest)

  target_sources(
    opentxs-testlib
    PRIVATE
      "Activity.cpp"
      "Activity.hpp"
      "BIP158.cpp"
      "BIP158.hpp"
      "BIP44.cpp"
      "BIP44.hpp"
      "Basic.cpp"
      "Basic.hpp"
      "BitcoinTransaction.cpp"
      "BitcoinTransaction.hpp"
      "BlockHeader.cpp"
      "BlockHeader.hpp"
      "BlockHeaderListener.cpp"
      "BlockHeaderListener.hpp"
      "BlockListener.cpp"
      "BlockListener.hpp"
      "BlockchainStartup.cpp"
      "BlockchainStartup.hpp"
      "Blocks.cpp"
      "Blocks.hpp"
      "Cashtoken.cpp"
      "Cashtoken.hpp"
      "CfilterListener.cpp"
      "CfilterListener.hpp"
      "Common.hpp"
      "FaucetListener.cpp"
      "FaucetListener.hpp"
      "Filters.cpp"
      "Filters.hpp"
      "HeaderOracle.cpp"
      "HeaderOracle.hpp"
      "MinedBlocks.cpp"
      "MinedBlocks.hpp"
      "NumericHash.cpp"
      "NumericHash.hpp"
      "P2PAddress.cpp"
      "P2PAddress.hpp"
      "PeerListener.cpp"
      "PeerListener.hpp"
      "ScanListener.cpp"
      "ScanListener.hpp"
      "StartStop.hpp"
      "SyncListener.cpp"
      "SyncListener.hpp"
      "SyncRequestor.cpp"
      "SyncRequestor.hpp"
      "SyncServerDB.cpp"
      "SyncServerDB.hpp"
      "SyncSubscriber.cpp"
      "SyncSubscriber.hpp"
      "TXOState.cpp"
      "TXOState.hpp"
      "TXOs.cpp"
      "TXOs.hpp"
  )
endif()
