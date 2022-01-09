// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

#include "clientversion.h"
#include <stdint.h>
#include <string>

//
// client versioning
//

static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_PPCNISION
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;
extern const std::string CLIENT_DATE;

//
// database format versioning
//
static const int DATABASE_VERSION = 70509;

//
// network protocol versioning
//
static const int PROTOCOL_VERSION = 62039;

// intial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 209;

// disconnect from peers older than this proto version
static const int MIN_PEER_PROTO_VERSION = 62038;

// minimum peer version accepted by MNenginePool
static const int MIN_POOL_PEER_PROTO_VERSION = 62032;
static const int MIN_INSTANTX_PROTO_VERSION = 62032;

//! minimum peer version that can receive masternode payments
// V1 - Last protocol version before update
// V2 - Newest protocol version
static const int MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1 = 62032;
static const int MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2 = 62032;

// nTime field added to CAddress, starting with this version;
// if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

// only request blocks from nodes outside this range of versions
static const int NOBLKS_VERSION_START = 0;
static const int NOBLKS_VERSION_END = 62037;

// hard cutoff time for legacy network connections
static const int64_t HRD_LEGACY_CUTOFF = 1641186911; // ON (Sunday, January 2, 2022 9:15:11 PM GMT-08:00)

// hard cutoff time for future network connections
static const int64_t HRD_FUTURE_CUTOFF = 9993058800; // OFF (NOT TOGGLED)

// BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

// "mempool" command, enhanced "getdata" behavior starts with this version:
static const int MEMPOOL_GD_VERSION = 60002;

// MasterNode peer IP advanced relay system start (Unfinished, not used)
static const int64_t MIN_MASTERNODE_ADV_RELAY = 9993058800; // OFF (NOT TOGGLED)

// MasterNode peer IP basic relay system start (on and functional)
static const int64_t MIN_MASTERNODE_BSC_RELAY = 62026; // ON

// MasterNode Tier 2 payment start date
static const int64_t MASTERNODE_TIER_2_START = 1602504000; // ON (Monday, October 12, 2020 5:00:00 AM GMT-07:00 PST)

// "demi-nodes" command, enhanced "getdata" behavior starts with this version:
static const int DEMINODE_VERSION = 60035;

#endif
