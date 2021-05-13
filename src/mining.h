// Copyright (c) 2016-2020 The CryptoCoderz Team / Espers
// Copyright (c) 2018-2020 The CryptoCoderz Team / INSaNe project
// Copyright (c) 2018-2020 The Rubix project
// Copyright (c) 2020 The PupaCoin project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_MINING_H
#define BITCOIN_MINING_H

#include "bignum.h"
/** Minimum nCoinAge required to stake PoS */
static const unsigned int nStakeMinAge = 2 / 60; // 30 minutes
/** Time to elapse before new modifier is computed */
static const unsigned int nModifierInterval = 2 * 60;
/** Genesis block subsidy */
static const int64_t nGenesisBlockReward = 1 * COIN;
/** Reserve block subsidy */
static const int64_t nBlockRewardReserve = 140000 * COIN; // premine 14,000,000 PPCN
/** Standard block subsidy */
static const int64_t nBlockStandardReward = 5 * COIN;
/** Block spacing preferred */
static const int64_t BLOCK_SPACING = (5 * 60); // Five minutes (5 Min)
/** Block spacing minimum */
static const int64_t BLOCK_SPACING_MIN = (4.5 * 60); // Four-and-a-Half minutes (4 Min, 30 Sec)
/** Block spacing maximum */
static const int64_t BLOCK_SPACING_MAX = (5.5 * 60); // Five-and-a-Half minutes (5 Min, 30 Sec)
/** Desired block times/spacing */
static const int64_t GetTargetSpacing = BLOCK_SPACING;
/** MNengine collateral */
static const int64_t MNengine_COLLATERAL = (1 * COIN);
/** MNengine pool values */
static const int64_t MNengine_POOL_MAX = (999 * COIN);
/** MasterNode required collateral */
inline int64_t MasternodeCollateral(int nHeight) { return 50000; } // 50 thousand PPCN required as collateral
/** Coinbase transaction outputs can only be staked after this number of new blocks (network rule) */
static const int nStakeMinConfirmations = 80;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int nTransactionMaturity = 10; // 10-TXs
/** Coinbase generated outputs can only be spent after this number of new blocks (network rule) */
static const int nCoinbaseMaturity = 30; // 30-Mined


#endif // BITCOIN_MINING_H
