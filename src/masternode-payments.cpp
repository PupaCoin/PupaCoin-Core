// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "masternode-payments.h"
#include "masternodeman.h"
#include "mnengine.h"
#include "util.h"
#include "sync.h"
#include "spork.h"
#include "addrman.h"
#include <boost/lexical_cast.hpp>

CCriticalSection cs_masternodepayments;

/** Object for who's going to get paid on which blocks */
CMasternodePayments masternodePayments;
// keep track of Masternode votes I've seen
map<uint256, CMasternodePaymentWinner> mapSeenMasternodeVotes;

int CMasternodePayments::GetMinMasternodePaymentsProto() {
    return IsSporkActive(SPORK_10_MASTERNODE_PAY_UPDATED_NODES)
            ? MIN_MASTERNODE_PAYMENT_PROTO_VERSION_2
            : MIN_MASTERNODE_PAYMENT_PROTO_VERSION_1;
}

void ProcessMessageMasternodePayments(CNode* pfrom, std::string& strCommand, CDataStream& vRecv)
{
    if(!mnEnginePool.IsBlockchainSynced()) return;

    if (strCommand == "mnget") { //Masternode Payments Request Sync

        if(pfrom->HasFulfilledRequest("mnget")) {
            LogPrintf("mnget - peer already asked me for the list\n");
            Misbehaving(pfrom->GetId(), 20);
            return;
        }

        pfrom->FulfilledRequest("mnget");
        masternodePayments.Sync(pfrom);
        LogPrintf("mnget - Sent Masternode winners to %s\n", pfrom->addr.ToString().c_str());
    }
    else if (strCommand == "mnw") { //Masternode Payments Declare Winner

        LOCK(cs_masternodepayments);

        //this is required in litemode
        CMasternodePaymentWinner winner;
        vRecv >> winner;

        if(pindexBest == NULL) return;

        CTxDestination address1;
        ExtractDestination(winner.payee, address1);
        CPupaCoinAddress address2(address1);

        uint256 hash = winner.GetHash();
        if(mapSeenMasternodeVotes.count(hash)) {
            if(fDebug) LogPrintf("mnw - seen vote %s Addr %s Height %d bestHeight %d\n", hash.ToString().c_str(), address2.ToString().c_str(), winner.nBlockHeight, pindexBest->nHeight);
            return;
        }

        if(winner.nBlockHeight < pindexBest->nHeight - 10 || winner.nBlockHeight > pindexBest->nHeight+20){
            LogPrintf("mnw - winner out of range %s Addr %s Height %d bestHeight %d\n", winner.vin.ToString().c_str(), address2.ToString().c_str(), winner.nBlockHeight, pindexBest->nHeight);
            return;
        }

        if(winner.vin.nSequence != std::numeric_limits<unsigned int>::max()){
            LogPrintf("mnw - invalid nSequence\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if(pfrom->nVersion < MIN_MASTERNODE_BSC_RELAY){
            LogPrintf("mnw - WARNING - Skipping winner relay, peer: %d is unable to handle such requests!\n", pfrom->nVersion);
            return;
        }

        LogPrintf("mnw - winning vote - Vin %s Addr %s Height %d bestHeight %d\n", winner.vin.ToString().c_str(), address2.ToString().c_str(), winner.nBlockHeight, pindexBest->nHeight);

        if(!masternodePayments.CheckSignature(winner)){
            LogPrintf("mnw - invalid signature\n");
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        mapSeenMasternodeVotes.insert(make_pair(hash, winner));

        if(masternodePayments.AddWinningMasternode(winner)){
            masternodePayments.Relay(winner);
        }
    }
}


bool CMasternodePayments::CheckSignature(CMasternodePaymentWinner& winner)
{
    //note: need to investigate why this is failing
    std::string strMessage = winner.vin.ToString().c_str() + boost::lexical_cast<std::string>(winner.nBlockHeight) + winner.payee.ToString();
    std::string strPubKey = strMainPubKey ;
    CPubKey pubkey(ParseHex(strPubKey));

    std::string errorMessage = "";
    if(!mnEngineSigner.VerifyMessage(pubkey, winner.vchSig, strMessage, errorMessage)){
        return false;
    }

    return true;
}

bool CMasternodePayments::Sign(CMasternodePaymentWinner& winner)
{
    std::string strMessage = winner.vin.ToString().c_str() + boost::lexical_cast<std::string>(winner.nBlockHeight) + winner.payee.ToString();

    CKey key2;
    CPubKey pubkey2;
    std::string errorMessage = "";

    if(!mnEngineSigner.SetKey(strMasterPrivKey, errorMessage, key2, pubkey2))
    {
        LogPrintf("CMasternodePayments::Sign - ERROR: Invalid Masternodeprivkey: '%s'\n", errorMessage.c_str());
        LogPrintf("CMasternodePayments::Sign - FORCE BYPASS - SetKey checks!!!\n");
        //return false;
    }

    if(!mnEngineSigner.SignMessage(strMessage, errorMessage, winner.vchSig, key2)) {
        LogPrintf("CMasternodePayments::Sign - Sign message failed\n");
        LogPrintf("CMasternodePayments::Sign - FORCE BYPASS - Sign message checks!!!\n");
        //return false;
    }

    if(!mnEngineSigner.VerifyMessage(pubkey2, winner.vchSig, strMessage, errorMessage)) {
        LogPrintf("CMasternodePayments::Sign - Verify message failed\n");
        LogPrintf("CMasternodePayments::Sign - FORCE BYPASS - Verify message checks!!!\n");
        //return false;
    }

    return true;
}

uint64_t CMasternodePayments::CalculateScore(uint256 blockHash, CTxIn& vin)
{
    uint256 n1 = blockHash;
    uint256 n2 = Hash(BEGIN(n1), END(n1));
    uint256 n3 = Hash(BEGIN(vin.prevout.hash), END(vin.prevout.hash));
    uint256 n4 = n3 > n2 ? (n3 - n2) : (n2 - n3);

    //printf(" -- CMasternodePayments CalculateScore() n2 = %d \n", n2.Get64());
    //printf(" -- CMasternodePayments CalculateScore() n3 = %d \n", n3.Get64());
    //printf(" -- CMasternodePayments CalculateScore() n4 = %d \n", n4.Get64());

    return n4.Get64();
}

bool CMasternodePayments::GetWinningMasternode(int nBlockHeight, CScript& payee, CTxIn& vin)
{
    // Try to get frist masternode in our list
    CMasternode* winningNode = mnodeman.GetCurrentMasterNode(1);
    // If initial sync or we can't find a masternode in our list
    if(IsInitialBlockDownload() || !winningNode || !ProcessBlock(nBlockHeight)){
        // Return false (for sanity, we have no masternode to pay)
        return false;
    }
    // Set masternode winner to pay
    BOOST_FOREACH(CMasternodePaymentWinner& winner, vWinning){
        payee = winner.payee;
        vin = winner.vin;
    }
    // Check Tier level of MasterNode winner
    fMnT2 = mnEngineSigner.IsVinTier2(vin);
    // Return true if previous checks pass
    return true;
}

bool CMasternodePayments::AddWinningMasternode(CMasternodePaymentWinner& winnerIn)
{
    uint256 blockHash = 0;

    winnerIn.score = CalculateScore(blockHash, winnerIn.vin);

    bool foundBlock = false;
    BOOST_FOREACH(CMasternodePaymentWinner& winner, vWinning){
        if(winner.nBlockHeight == winnerIn.nBlockHeight) {
            foundBlock = true;
            if(winner.score < winnerIn.score){
                winner.score = winnerIn.score;
                winner.vin = winnerIn.vin;
                winner.payee = winnerIn.payee;
                winner.vchSig = winnerIn.vchSig;

                mapSeenMasternodeVotes.insert(make_pair(winnerIn.GetHash(), winnerIn));

                return true;
            }
        }
    }

    // if it's not in the vector
    if(!foundBlock){
        vWinning.push_back(winnerIn);
        mapSeenMasternodeVotes.insert(make_pair(winnerIn.GetHash(), winnerIn));

        return true;
    }

    return false;
}

void CMasternodePayments::CleanPaymentList()
{
    LOCK(cs_masternodepayments);

    if(pindexBest == NULL) return;

    int nLimit = std::max(((int)mnodeman.size())*((int)1.25), 1000);

    vector<CMasternodePaymentWinner>::iterator it;
    for(it=vWinning.begin();it<vWinning.end();it++){
        if(pindexBest->nHeight - (*it).nBlockHeight > nLimit){
            if(fDebug) LogPrintf("CMasternodePayments::CleanPaymentList - Removing old Masternode payment - block %d\n", (*it).nBlockHeight);
            vWinning.erase(it);
            break;
        }
    }
}

bool CMasternodePayments::NodeisCapable()
{
    bool fNodeisCapable;
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        if(pnode->nVersion >= MIN_MASTERNODE_BSC_RELAY) {
            LogPrintf(" NodeisCapable() - Capable peers found (Masternode relay) \n");
            fNodeisCapable = true;
        } else {
            LogPrintf(" NodeisCapable() - Cannot relay with peers (Masternode relay) \n");
            fNodeisCapable = false;
            break;
        }
    }
    return fNodeisCapable;
}

bool CMasternodePayments::ProcessBlock(int nBlockHeight)
{
    LOCK(cs_masternodepayments);

    if(nBlockHeight <= 10) return false;//Superficial, checked in "GetWinningMasternode"
    if(IsInitialBlockDownload()) return false;//Superficial, checked in "GetWinningMasternode"
    if(!NodeisCapable()) { LogPrintf("Masternode-Payments::ProcessBlock - WARNING - Capability issue, peer is unable to handle relay requests!\n");}
    CMasternodePaymentWinner newWinner;
    int nMinimumAge = mnodeman.CountEnabled();
    CScript payeeSource;

    uint256 hash;
    if(!GetBlockHash(hash, nBlockHeight)) return false;
    unsigned int nHash;
    memcpy(&nHash, &hash, 2);

    LogPrintf(" Masternode-Payments::ProcessBlock - Start nHeight %d - vin %s. \n", nBlockHeight, activeMasternode.vin.ToString().c_str());

    // TODO: Rewrite masternode winner sync/logging
    //std::vector<CTxIn> vecLastPayments;
    //BOOST_REVERSE_FOREACH(CMasternodePaymentWinner& winner, vWinning)
    //{
    //    //if we already have the same vin - we have one full payment cycle, break
    //    if(vecLastPayments.size() > (unsigned int)nMinimumAge) break;
    //    vecLastPayments.push_back(winner.vin);
    //}
    //
    // pay to the oldest MN that still had no payment but its input is old enough and it was active long enough
    //CMasternode *pmn = mnodeman.FindOldestNotInVec(vecLastPayments, nMinimumAge);
    //
    // For now just get the current highest ranking MasterNode
    // and relay that (if capable)
    CMasternode *pmn = mnodeman.GetCurrentMasterNode(1);

    if(pmn->protocolVersion < MIN_MASTERNODE_BSC_RELAY) {
        LogPrintf("Masternode-Payments::ProcessBlock - WARNING - Capability issue, masternode: %d is unable to handle relay requests!\n", pmn->protocolVersion);
        //return false;
    }

    if(pmn != NULL)
    {
        LogPrintf("  Found MasterNode winner to pay\n");//Found by FindOldestNotInVec

        newWinner.score = 0;
        newWinner.nBlockHeight = nBlockHeight;
        newWinner.vin = pmn->vin;

        newWinner.payee = GetScriptForDestination(pmn->pubkey.GetID());
        payeeSource = GetScriptForDestination(pmn->pubkey.GetID());
    }

    //if we can't find new MN to get paid, pick first active MN counting back from the end of vecLastPayments list
    if(newWinner.nBlockHeight == 0 && nMinimumAge > 0)
    {
        LogPrintf(" Finding logged winners is not currently supported (SKIPPING) \n");//Find by reverse

        /*BOOST_REVERSE_FOREACH(CTxIn& vinLP, vecLastPayments)
        {
            CMasternode* pmn = mnodeman.Find(vinLP);
            if(pmn != NULL)
            {
                pmn->Check();
                if(!pmn->IsEnabled()) continue;

                newWinner.score = 0;
                newWinner.nBlockHeight = nBlockHeight;
                newWinner.vin = pmn->vin;

                if(pmn->donationPercentage > 0 && (nHash % 100) <= (unsigned int)pmn->donationPercentage) {
                    newWinner.payee = pmn->donationAddress;
                } else {
                    newWinner.payee = GetScriptForDestination(pmn->pubkey.GetID());
                }

                payeeSource = GetScriptForDestination(pmn->pubkey.GetID());

                break; // we found active MN
            }
        }*/
    }

    if(newWinner.nBlockHeight == 0) return false;

    CTxDestination address1;
    ExtractDestination(newWinner.payee, address1);
    CPupaCoinAddress address2(address1);

    CTxDestination address3;
    ExtractDestination(payeeSource, address3);
    CPupaCoinAddress address4(address3);

    LogPrintf("Winner payee %s nHeight %d vin source %s. \n", address2.ToString().c_str(), newWinner.nBlockHeight, address4.ToString().c_str());

    if(Sign(newWinner))
    {
        LogPrintf("Signed winning masternode. \n");
        if(AddWinningMasternode(newWinner))
        {
            LogPrintf("Added winning masternode. \n");

            if(pmn->protocolVersion >= MIN_MASTERNODE_BSC_RELAY){
                // Relay node if capable peer
                Relay(newWinner);
            }

            nLastBlockHeight = nBlockHeight;
            return true;
        }
    }

    return false;
}


void CMasternodePayments::Relay(CMasternodePaymentWinner& winner)
{
    CInv inv(MSG_MASTERNODE_WINNER, winner.GetHash());

    LogPrintf("Relayed winning masternode. \n");

    vector<CInv> vInv;
    vInv.push_back(inv);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes){
        if(NodeisCapable()) { // Only send to capable nodes
            //pnode->PushMessage("inv", vInv);
        }
    }
}

void CMasternodePayments::Sync(CNode* node)
{
    LOCK(cs_masternodepayments);

    BOOST_FOREACH(CMasternodePaymentWinner& winner, vWinning)
        if(winner.nBlockHeight >= pindexBest->nHeight-10 && winner.nBlockHeight <= pindexBest->nHeight + 20)
            node->PushMessage("mnw", winner);
}


bool CMasternodePayments::SetPrivKey(std::string strPrivKey)
{
    CMasternodePaymentWinner winner;

    // Test signing successful, proceed
    strMasterPrivKey = strPrivKey;

    Sign(winner);

    if(CheckSignature(winner)){
        LogPrintf("CMasternodePayments::SetPrivKey - Successfully initialized as Masternode payments master\n");
        enabled = true;
        return true;
    } else {
        return false;
    }
}
