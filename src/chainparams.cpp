// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"
#include "util.h"
// TODO: Verify the requirement of below link
// #include "base58.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

// Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

//
// Main network
//
class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xb5;
        pchMessageStart[1] = 0x5f;
        pchMessageStart[2] = 0x1a;
        pchMessageStart[3] = 0xa3;
        vAlertPubKey = ParseHex("03bacf5aa489f123756b659c91a56897eba2efaacb6a192acdbef7894465f81f85d131aadfef3be6145678454852a2d08c6314bba5ca3cbe5616262da3b1a6aaac");
        nDefaultPort = 20205; // May of 2020, when Solar Opposites was released
        nRPCPort = 20083; // March of 2008, when Hulu was made public (couldn't do anything actually meaningful as it had to be 5 digits)
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 16);
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 18);

        const char* pszTimestamp = "Bitcoin Vault inks major deal with ESE to co-produce Gaming & Esports Talent Show in five countries | Null Transaction PR | May 11, 2021";
        std::vector<CTxIn> vin;
        vin.resize(1);
        vin[0].scriptSig = CScript() << 0 << CBigNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        std::vector<CTxOut> vout;
        vout.resize(1);
        vout[0].nValue = 1 * COIN;
        vout[0].SetEmpty();
        CTransaction txNew(1, 1620864000, vin, vout, 0);
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1620864000;
        genesis.nBits    = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce   = 114752;

        /** Genesis Block MainNet */
        /*
        Hashed MainNet Genesis Block Output
        block.hashMerkleRoot == 4787cdaf8fb6f6dacc68f52fbd60e9f95044f7a239efe174e052a7f9fca2ded2
        block.nTime = 1620864000
        block.nNonce = 114752
        block.GetHash = 0000c7d7bc6a7b6bc769d03439e9a9bcb7c9818e802cb96253fc58df8aa33176

        */

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x0000c7d7bc6a7b6bc769d03439e9a9bcb7c9818e802cb96253fc58df8aa33176"));
        assert(genesis.hashMerkleRoot == uint256("0x4787cdaf8fb6f6dacc68f52fbd60e9f95044f7a239efe174e052a7f9fca2ded2"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,55);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,63);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,56);
        base58Prefixes[STEALTH_ADDRESS] = std::vector<unsigned char>(1,64);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        // vSeeds.push_back(CDNSSeedData("node0",  "000.000.000.000"));


        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        nPoolMaxTransactions = 9;
        strMNenginePoolDummyAddress = "PBzzcxLFGMTprSs4j6MNJNA9u5P9birvwJ";
        strDevOpsAddress = "PTUzfmVxBk6eZFa3J3JJeCyru8oCbZJuPx";
        nEndPoWBlock = 0x7fffffff;
        nStartPoSBlock = 1;
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet
//
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x1a;
        pchMessageStart[1] = 0x4e;
        pchMessageStart[2] = 0xa3;
        pchMessageStart[3] = 0x12;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 12);
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 14);
        vAlertPubKey = ParseHex("04acef5aa489f996be6b659c91a518567ba2efaacb6acdefcdbef7894452f81f85d131aadfef3be6145678454852a2d08c6314bba5ca3cbe5616262da3b1a6aaab");
        nDefaultPort = 20063; // December 3rd, 2006 (Got lazy, here you go) - The Real Animated Adventures of Doc and Mharti release date
        nRPCPort = 20132; // December 2nd, 2013 - Release date of Rick and Morty, a show about depressed people living even more depressing lives.
        strDataDir = "testnet";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime  = 1620864000;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 13250;

        /** Genesis Block TestNet */
        /*
        Hashed TestNet Genesis Block Output
        block.hashMerkleRoot == 4787cdaf8fb6f6dacc68f52fbd60e9f95044f7a239efe174e052a7f9fca2ded2
        block.nTime = 1620864000
        block.nNonce = 13250
        block.GetHash = 000088f08e579c29f9e3349bd4767c84c4a8098f9668af2d1c1d8e004887543b

        */

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x000088f08e579c29f9e3349bd4767c84c4a8098f9668af2d1c1d8e004887543b"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,54); // Maybe P
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,62); // Maybe S
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,68); // U
        base58Prefixes[STEALTH_ADDRESS] = std::vector<unsigned char>(1,23); // A
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        nEndPoWBlock = 0x7fffffff;
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;

//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        pchMessageStart[0] = 0x22;
        pchMessageStart[1] = 0x9a;
        pchMessageStart[2] = 0x99;
        pchMessageStart[3] = 0x19;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        genesis.nTime = 1620864000;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 8;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 20208; // Would have made it Mike McMahan's B-Day, but I can't find it, unfortunately. So here's release date of Star Trek: Lower Decks. August 6^th, of 2020
        strDataDir = "regtest";

        /** Genesis Block RegNet */
        /*
        Hashed RegNet Genesis Block Output
        block.hashMerkleRoot == 4787cdaf8fb6f6dacc68f52fbd60e9f95044f7a239efe174e052a7f9fca2ded2
        block.nTime = 1620864000
        block.nNonce = 8
        block.GetHash = 578667a92fd32d1d7df14d155984ee61d91fd2681826d7737cd035e1e6535b00
        */

        assert(hashGenesisBlock == uint256("0x578667a92fd32d1d7df14d155984ee61d91fd2681826d7737cd035e1e6535b00"));

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = &regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest) {
        return false;
    }

    if (fRegTest) {
        SelectParams(CChainParams::REGTEST);
    } else if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}
