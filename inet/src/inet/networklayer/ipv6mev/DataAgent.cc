//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "inet/networklayer/ipv6mev/DataAgent.h"

#include <algorithm>
#include <stdlib.h>
#include "inet/common/ModuleAccess.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/common/IPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv6/IPv6ControlInfo.h"
#include "inet/networklayer/ipv6/IPv6Datagram.h"
#include "inet/networklayer/ipv6/IPv6ExtensionHeaders.h"
#include "inet/networklayer/ipv6/IPv6InterfaceData.h"
#include "inet/transportlayer/udp/UDPPacket.h"

#include "inet/common/NotifierConsts.h"
#include "inet/common/lifecycle/NodeStatus.h"

namespace inet {

Define_Module(DataAgent);

void DataAgent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
        IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
        ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
    }
    if (stage == INITSTAGE_APPLICATION_LAYER) {
    }
}

void DataAgent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {
        if(msg->getKind() == MSG_MA_INIT) { // from CA
//            sendAgentInit(msg);
        }
        else if(msg->getKind() == MSG_SEQ_UPDATE_NOTIFY) {
            UpdateNotifierTimer *unt = (UpdateNotifierTimer *) msg->getContextPointer();
            uint64 mobileId = unt->id;
            uint seq = unt->seq;
            uint ack = unt->ack;
//            sendSequenceUpdateNotification(mobileId, ack, seq);
            delete msg;
        }
        else
            throw cRuntimeError("handleMessage: Unknown timer expired. Which timer msg is unknown?");
    }
    else if(msg->arrivedOn("fromLowerLayer")) {
        if (dynamic_cast<IdentificationHeader *> (msg)) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            IdentificationHeader *idHdr = (IdentificationHeader *) msg;
            if (dynamic_cast<ControlAgentHeader *>(idHdr)) {
                ControlAgentHeader *ca = (ControlAgentHeader *) idHdr;
                processControlAgentMessage(ca, controlInfo);
            } else if (dynamic_cast<MobileAgentHeader *>(idHdr)) {
                MobileAgentHeader *ma = (MobileAgentHeader *) idHdr;
                processMobileAgentMessage(ma,controlInfo);
            } else {
                throw cRuntimeError("A:handleMsg: Extension Hdr Type not known. What did you send?");
            }
        }
    }
    else
        throw cRuntimeError("A:handleMsg: cMessage Type not known. What did you send?");
}

void DataAgent::createSequenceUpdateNotificaiton(uint64 mobileId)
{
    cMessage *msg = new cMessage("sendingCAseqUpd", MSG_SEQ_UPDATE_NOTIFY);
    if(!caAddress.isGlobal())
        throw cRuntimeError("DA:SeqUpdNotify: CA_Address is not set. Cannot send update message without address");
    TimerKey key(caAddress,-1,TIMERKEY_SEQ_UPDATE_NOT, mobileId, am.getCurrentSequenceNumber(mobileId));
    UpdateNotifierTimer *unt = (UpdateNotifierTimer *) getExpiryTimer(key,TIMERTYPE_SEQ_UPDATE_NOT);
//    unt->dest = daAddr.toIPv6();
//    unt->ie = -1;
    unt->timer = msg;
    unt->ackTimeout = TIMEOUT_SEQ_UPDATE;
    unt->nextScheduledTime = simTime();
    unt->id = mobileId;
    unt->ack = am.getLastAcknowledgemnt(mobileId);
//    unt->seq = am.getCurrentSequenceNumber(mobileId);
    msg->setContextPointer(unt);
    scheduleAt(unt->nextScheduledTime,msg);
}

void DataAgent::sendAgentInitResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId)
{
    DataAgentHeader *dah = new DataAgentHeader("da_seq_init_ack");
    dah->setIdInit(true);
    dah->setIdAck(true);
    dah->setSeqValid(true);
    dah->setIdValid(true);
    dah->setId(mobileId);
    dah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    dah->setByteLength(SIZE_AGENT_HEADER);
    sendToLowerLayer(dah,destAddr, sourceAddr);
}

void DataAgent::sendAgentUpdateResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId)
{
    sendAgentInitResponse(destAddr, sourceAddr, mobileId);
}

void DataAgent::processMobileAgentMessage(MobileAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    // ================================================================================
    // dataAgent message processing
    // ================================================================================
    IPv6Address destAddr = controlInfo->getSrcAddr(); // address to be responsed
    IPv6Address sourceAddr = controlInfo->getDestAddr();
    if(caAddress.isUnspecified()) { caAddress = controlInfo->getSrcAddr(); }
    // header flag bits 101010 + NextHeader=NONE
    if(agentHeader->getIdInit() && !agentHeader->getIdAck() && agentHeader->getSeqValid() && !agentHeader->getAckValid()  && agentHeader->getAddValid() && !agentHeader->getRemValid() && (agentHeader->getNextHeader() == IP_PROT_NONE)) // check for other bits
    { // INIT PROCESSING
        if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) == mobileIdList.end())
        { // if mobile is not registered. insert a new entry in map
            EV << "DA: Received agent init message from CA. Adding id to list: " << agentHeader->getId() << endl;
            mobileIdList.push_back(agentHeader->getId());
                if(agentHeader->getIpAddingField() > 0) { // check size of field
                    bool addrMgmtEntry = am.insertNewId(agentHeader->getId(), agentHeader->getIpSequenceNumber(), agentHeader->getAddedAddresses(0)); //first number
                    if(!addrMgmtEntry) // if we receive for the first time an init message, then response to this one
                        throw cRuntimeError("DA:procMAmsg: Could not insert id.");
                } else
                    throw cRuntimeError("DA:procMAmsg: Init message must contain the start message of mobile agent.");
                am.setLastAcknowledgemnt(agentHeader->getId(), agentHeader->getIpSequenceNumber());
                sendAgentInitResponse(destAddr, sourceAddr, agentHeader->getId());
        } else
        { // IF NOT AGENT INIT then JUST SEQ UPDATE
            EV << "DA: Received message from CA. Id already exists. Skipping init and update just map." << endl;
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                AddressManagement::IPv6AddressList ipList;
                if(agentHeader->getIpAddingField() > 0) { // check size of field
                    if(agentHeader->getIpAddingField() != agentHeader->getAddedAddressesArraySize())
                        throw cRuntimeError("CA:Hdr: field of array and field not same. Check header.");
                    for(int i=0; i<agentHeader->getIpAddingField(); i++){ // copy array in vector list
                        ipList.push_back(agentHeader->getAddedAddresses(i)); // inserting elements of array into vector list due to function
                    }
                }
                am.insertSequenceTableToAddressMap(agentHeader->getId(), ipList, agentHeader->getIpSequenceNumber());
                am.setLastAcknowledgemnt(agentHeader->getId(),agentHeader->getIpSequenceNumber()); //
                sendAgentUpdateResponse(destAddr, sourceAddr, agentHeader->getId());
            } else
                throw cRuntimeError("DA:procMA: Id is not inserted in map. Register id before seq update.");
        }
    }
    else if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid() && agentHeader->getAckValid())
    { // regular message from mobile agent
        if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
           // insert new address
            // and then check if ip addr is in map
            if(agentHeader->getNextHeader() == IP_PROT_UDP) {
                // process here UDP packets from mobile agent
            } else if(agentHeader->getNextHeader() == IP_PROT_TCP) {
                // process here TCP packets from mobile agent
            } else { throw cRuntimeError("DA:procMAmsg: header declaration not known."); }
        } else { throw cRuntimeError("DA:procMAmsg: DA is not initialized with given ID."); }
    }
delete agentHeader;
delete controlInfo;
}

void DataAgent::processControlAgentMessage(ControlAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid() && (agentHeader->getNextHeader() == IP_PROT_NONE)) {

    }
}

InterfaceEntry *DataAgent::getInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol) { // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal()) { return ie; }
    }
    return ie;
}

void DataAgent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr, int interfaceId, simtime_t delayTime) {
//    EV << "A: Creating IPv6ControlInfo to lower layer" << endl;
    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
    ctrlInfo->setProtocol(IP_PROT_IPv6EXT_ID); // todo must be adjusted
    ctrlInfo->setDestAddr(destAddr);
    ctrlInfo->setSrcAddr(srcAddr);
    ctrlInfo->setHopLimit(255);
    InterfaceEntry *ie = getInterface(destAddr);
    if(ie) { ctrlInfo->setInterfaceId(ie->getInterfaceId()); }
    msg->setControlInfo(ctrlInfo);
    cGate *outgate = gate("toLowerLayer");
    EV << "ControlInfo: Dest=" << ctrlInfo->getDestAddr() << " Src=" << ctrlInfo->getSrcAddr() << " If=" << ctrlInfo->getInterfaceId() << endl;
    if (delayTime > 0) {
        EV << "delayed sending" << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

} //namespace
