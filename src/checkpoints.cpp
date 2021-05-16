// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

#include "checkpoints.h"

#include "txdb.h"
#include "main.h"
#include "uint256.h"


static const int nCheckpointSpan = 5000;

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    //
    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    //
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (0,        Params().HashGenesisBlock() )
        (1,        uint256("0x000084679a17f8cab7cdbd1ed41bdbc236af4d75ca544a1ec139eaa22342340a"))
        (10,       uint256("0x00003917aa4d2fd9a7f5522de54d09c2cdb7a9f791d4a5bd864e8d6df07daf1c"))
        (100,      uint256("0x135677ad9de09ab96bd928efb5b76ff360021a26070b236f18acb10d3f964df9"))
        (350,      uint256("0x000000001b1889c08489f2a4c9ad8ca71d2453612579df59ceb082e24fb73009"))
        (650,      uint256("0x000000000088804920bd4ed4abd9e6e87be19469ef401639e301b7a16c8ff574"))
        (724,      uint256("0x0000000004bdd7439bdfad949b180d6b62b6e0a8def91c29fff55da943b055ef"))
        (725,      uint256("0x000000000228cf6a0e5418acb956c60acff69a9eedb6fa4598084ca9dbde9aad"))
        (726,      uint256("0xe81edcd3efddf5a47fdaf00c8ef199c56e748794a8b95a082fc704c64d4582aa"))
        (728,      uint256("0x6ba1d0e84db6c0bfaf65d5eed6d58240028b147d66c1e9ea3abcad7e595c2c52"))
        (924,      uint256("0x4e2af755dbbf806ba1b05235efc2fcb8a05d2ea925501a31a07fdae5be114c9d"))
        (943,      uint256("0xcdfee0e47f2c6f8538cf43a0eeeb360c98106d00ce240f95a1b136ca20f187d9"))
    ;

    // TestNet has no checkpoints
    static MapCheckpoints mapCheckpointsTestnet;

    bool CheckHardened(int nHeight, const uint256& hash)
    {
        MapCheckpoints& checkpoints = (TestNet() ? mapCheckpointsTestnet : mapCheckpoints);

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    int GetTotalBlocksEstimate()
    {
        MapCheckpoints& checkpoints = (TestNet() ? mapCheckpointsTestnet : mapCheckpoints);

        if (checkpoints.empty())
            return 0;
        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        MapCheckpoints& checkpoints = (TestNet() ? mapCheckpointsTestnet : mapCheckpoints);

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }

    // Automatically select a suitable sync-checkpoint
    const CBlockIndex* AutoSelectSyncCheckpoint()
    {
        const CBlockIndex *pindex = pindexBest;
        // Search backward for a block within max span and maturity window
        while (pindex->pprev && pindex->nHeight + nCheckpointSpan > pindexBest->nHeight)
            pindex = pindex->pprev;
        return pindex;
    }

    // Check against synchronized checkpoint
    bool CheckSync(int nHeight)
    {
        const CBlockIndex* pindexSync = AutoSelectSyncCheckpoint();
        if (nHeight <= pindexSync->nHeight){
            return false;
        }
        return true;
    }
}
