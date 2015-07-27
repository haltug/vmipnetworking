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

#include "inet/networklayer/ipv6mev/ControlAgent.h"

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

namespace inet {

Define_Module(ControlAgent);

void ControlAgent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
            IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
            ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
            srand(321); // TODO must be changed
            agentId = (uint64) 0xCA;
            agentId = agentId << 48;
            agentId = agentId | (0xFFFFFFFFFFFF & rand());
    }
}

void ControlAgent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {
        if(msg->getKind() == MSG_MA_INIT) { // from CA
            sendAgentInit(msg);
        }
        else if(msg->getKind() == MSG_MA_INIT_DELAY) { // from CA for retransmitting of da_init
            MobileInitTimer *mit = (MobileInitTimer *) msg->getContextPointer();
            uint64 mobileId = mit->id;
            createAgentInit(mobileId);
            delete msg;
        }
        else if(msg->getKind() == MSG_AGENT_UPDATE) { // from CA for retransmitting of da_init
            sendAgentUpdate(msg);
        }
        else
            throw cRuntimeError("handleMessage: Unknown timer expired. Which timer msg is unknown?");
    }
    else if(msg->arrivedOn("fromLowerLayer")) {
        if (dynamic_cast<IdentificationHeader *> (msg)) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processAgentMessage((IdentificationHeader *) msg, controlInfo);
        } else {
            throw cRuntimeError("A:handleMsg: Extension Hdr Type not known. What did you send?");
        }
    }
    else
        throw cRuntimeError("A:handleMsg: cMessage Type not known. What did you send?");
}

void ControlAgent::createAgentInit(uint64 mobileId)
{
//    EV << "CA: Create DA_MobileId_init" << endl;
    const char *dataAgentAddr = par("dataAgentAddress");
    cStringTokenizer tokenizer(dataAgentAddr);
    while (tokenizer.hasMoreTokens()) {
        L3Address daAddr;
        L3AddressResolver().tryResolve(tokenizer.nextToken(), daAddr);
        if(daAddr.toIPv6().isGlobal()) {
            cMessage *msg = new cMessage("sendingDAinit", MSG_MA_INIT);
            TimerKey key(daAddr.toIPv6(),-1,TIMERKEY_MA_INIT, mobileId);
            if(std::find(agentAddressList.begin(), agentAddressList.end(), daAddr.toIPv6()) == agentAddressList.end())
                agentAddressList.push_back(daAddr.toIPv6()); // should be placed at the point of response
            MobileInitTimer *mit = (MobileInitTimer *) getExpiryTimer(key,TIMERTYPE_MA_INIT);
            mit->dest = daAddr.toIPv6();
            mit->timer = msg;
            mit->ackTimeout = TIMEOUT_SESSION_INIT;
            mit->nextScheduledTime = simTime();
            mit->id = mobileId;
            msg->setContextPointer(mit);
            scheduleAt(mit->nextScheduledTime,msg);
        } else {
            cMessage *msg = new cMessage("sendingDAinit", MSG_MA_INIT_DELAY);
            MobileInitTimer *mit = new MobileInitTimer();
            mit->id = mobileId;
            msg->setContextPointer(mit);
            scheduleAt(simTime()+TIMEDELAY_MA_INIT, msg);
        }
    }
}

void ControlAgent::sendAgentInit(cMessage *msg)
{   // header format MA:10101000 CA:11100000
    EV << "CA: Send initialization message to any DataAgent." << endl;
    MobileInitTimer *mit = (MobileInitTimer *) msg->getContextPointer();
    InterfaceEntry *ie = getInterface();
    if(!ie) throw cRuntimeError("CA: no interface provided.");
    const IPv6Address &dest = mit->dest;
    mit->nextScheduledTime = simTime() + mit->ackTimeout;
    mit->ackTimeout = (mit->ackTimeout)*2;
    for(auto &item : am.getAddressMap()) {
        IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, am.getSeqNo(item.first), 0, item.first);
        ih->setIsIdInitialized(true);
        ih->setIsSeqValid(true);
        ih->setIsIpModified(true);
        AddressManagement::AddressChange ac = am.getAddressEntriesOfSeqNo(mit->id,am.getSeqNo(mit->id));
        ih->setIpAddingField(ac.addedAddresses);
        ih->setIPaddressesArraySize(ac.addedAddresses);
        if(ac.addedAddresses > 0) {
            for(int i=0; i<ac.addedAddresses; i++) {
                ih->setIPaddresses(i,ac.getAddedIPv6AddressList.at(i));
            }
        }
        ih->setIpRemovingField(0);
        ih->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*ac.addedAddresses));
        sendToLowerLayer(ih, dest);
    }
    scheduleAt(mit->nextScheduledTime, msg);
}

void ControlAgent::createAgentUpdate(uint64 mobileId, uint seq)
{
    EV << "CA: Create Agent update message to any DataAgent" << endl;
    for(IPv6Address nodeAddr : agentAddressList) {
        cMessage *msg = new cMessage("sendingDAseqUpdate", MSG_AGENT_UPDATE);
        TimerKey key(nodeAddr, -1, TIMERKEY_SEQ_UPDATE, mobileId, seq);
        SequenceUpdateTimer *sut = (SequenceUpdateTimer *) getExpiryTimer(key, TIMERTYPE_SEQ_UPDATE);
        sut->dest = nodeAddr;
        sut->seq = seq;
        sut->timer = msg;
        sut->id = mobileId;
        sut->ackTimeout = TIMEOUT_SEQ_UPDATE;
        sut->nextScheduledTime = simTime();
        msg->setContextPointer(sut);
        scheduleAt(sut->nextScheduledTime, msg);
    }
}

void ControlAgent::sendAgentUpdate(cMessage *msg)
{
//    EV << "CA: Send agent update" << endl;
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
    const IPv6Address &dest =  sut->dest;
    sut->nextScheduledTime = simTime() + sut->ackTimeout;
    sut->ackTimeout = (sut->ackTimeout)*1.5;
    IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, am.getSeqNo(sut->id), am.getSeqNo(sut->id), sut->id);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsIpModified(true);
    AddressManagement::AddressChange ac = am.getAddressEntriesOfSeqNo(sut->id,sut->seq);
//    EV << "CA: update message: changes: " << ac.addedAddresses << endl;
    ih->setIpAddingField(ac.addedAddresses);
    ih->setIPaddressesArraySize(ac.addedAddresses);
    if(ac.addedAddresses > 0) {
//        if(ac.addedAddresses != ac.getAddedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqUpd: value of Add list must have size of integer.");
        for(int i=0; i<ac.addedAddresses; i++) {
            ih->setIPaddresses(i,ac.getAddedIPv6AddressList.at(i));
        }
    }
    ih->setIpRemovingField(0);
    ih->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*ac.addedAddresses));
    sendToLowerLayer(ih, dest); // TODO select interface
    scheduleAt(sut->nextScheduledTime, msg);
}

void ControlAgent::sendSequenceUpdateAck(uint64 mobileId)
{ // sending over all registered links update messages to MA
    AddressManagement::IPv6AddressList addressList = am.getAddressList(mobileId);
    for (IPv6Address ip : addressList ) {
        IPv6Address destAddr = ip; // address to be responsed
        IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE,am.getSeqNo(mobileId), 0, agentId);
        ih->setIsIdInitialized(true);
        ih->setIsIdAcked(true);
        ih->setIsSeqValid(true);
        sendToLowerLayer(ih,destAddr);
    }
}

void ControlAgent::sendSessionInitResponse(IPv6Address destAddr)
{
    IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, 0, 0, agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    sendToLowerLayer(ih,destAddr);
}

void ControlAgent::sendSequenceInitResponse(IPv6Address destAddr, uint64 mobileId, uint seq)
{
    IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, seq, 0, agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    sendToLowerLayer(ih,destAddr);
//    createAgentInit(mobileId);
}

void ControlAgent::sendSequenceUpdateResponse(IPv6Address destAddr, uint64 mobileId, uint seq)
{ // is same to seqInitResponse
    IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, seq, 0, agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    sendToLowerLayer(ih,destAddr);
//    createAgentUpdate(mobileId); // we notify all dataAgents when we response to mobile agents seq update
}

void ControlAgent::sendFlowRequestResponse(IPv6Address destAddr, uint64 mobileId, uint seq, IPv6Address agentAddr, IPv6Address nodeAddr)
{
    IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, seq, 0, agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsWithNodeAddr(true);
    ih->setIsWithAgentAddr(true);
    ih->setIPaddressesArraySize(2);
    ih->setIPaddresses(0,nodeAddr);
    ih->setIPaddresses(1,agentAddr);
    ih->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR*2);
    createAgentInit(mobileId);
    sendToLowerLayer(ih,destAddr,0.1); // TODO remove delay
}

    // ================================================================================
    // controlAgent message processing
    // ================================================================================
void ControlAgent::processAgentMessage(IdentificationHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    IPv6Address destAddr = controlInfo->getSourceAddress().toIPv6(); // address to be responsed
//    IPv6Address sourceAddr = controlInfo->getDestinationAddress().toIPv6();
    if(agentHeader->getIsDataAgent() && agentHeader->getNextHeader() == IP_PROT_NONE) {
        if(agentHeader->getIsIdInitialized() && !agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid() && !agentHeader->getIsIpModified())
            performAgentInitResponse(agentHeader, destAddr); // we need here the source address to remove the timers
        else if(agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid() && !agentHeader->getIsIpModified())
            performAgentUpdateResponse(agentHeader, destAddr); // we need here the source address to remove the timers
        else if(agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid() && agentHeader->getIsIpModified())
            performSeqUpdate(agentHeader, destAddr);
        else
            throw cRuntimeError("CA: DA msg not known.");
    } else
        if(agentHeader->getIsMobileAgent() && agentHeader->getNextHeader() == IP_PROT_NONE) {
            if(agentHeader->getIsIdInitialized() && !agentHeader->getIsIdAcked() && !agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid()) // check for other bits // init process request
                initializeSession(agentHeader, destAddr);
            else
                if(agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid())  //TODO check from here if id is registered.
                    initializeSequence(agentHeader, destAddr);
                else
                    if(agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && agentHeader->getIsAckValid()) {
                        if(agentHeader->getIsWithNodeAddr() && !agentHeader->getIsIpModified())
                            performFlowRequest(agentHeader, destAddr);
                        else
                            if(agentHeader->getIsIpModified())
                                performSeqUpdate(agentHeader, destAddr);
                            else
                                throw cRuntimeError("CA: Header param from MA is not known.");
                    } else
                        throw cRuntimeError("CA: Header config from MA is wrong.");
        } else
            throw cRuntimeError("CA: Not MA or DA type. What it is else");
    delete agentHeader;
    delete controlInfo;
}


void ControlAgent::initializeSession(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
        EV << "CA: Id already exists. Skipping initializing, check for mistakes." << endl;
    } else {
        EV << "CA: Received session Init message. Adding id to list: " << agentHeader->getId() << endl;
        mobileIdList.push_back(agentHeader->getId()); // adding id to list (registering id)
        sendSessionInitResponse(destAddr);
    }
}

void ControlAgent::initializeSequence(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
        bool addrMgmtEntry = am.insertNewId(agentHeader->getId(), agentHeader->getIpSequenceNumber(), agentHeader->getIPaddresses(0));//first number
        if(addrMgmtEntry) { // check if seq and id is inserted
            int s = agentHeader->getIpSequenceNumber();
            EV << "CA: Received sequence initialize message. Initialized seq: " << s << " with ip: " << agentHeader->getIPaddresses(0) << endl;
            sendSequenceInitResponse(destAddr, agentHeader->getId(), am.getSeqNo(agentHeader->getId()));
        }
        else
            if(am.isIdInitialized(agentHeader->getId()))
                EV << "CA: ERROR: Id has been initialized before. Why do you send me again an seq init message?" << endl;
            else
                throw cRuntimeError("CA: Initialization of sequence number failed, CA could not insert id in AddrMgmt-Unit.");
    }
    else
        throw cRuntimeError("CA should initialize sequence number but cannot find id in list.");
}

void ControlAgent::performSeqUpdate(IdentificationHeader *agentHeader, IPv6Address destAddr)
{ // this method can be optimized
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
        if(agentHeader->getIsMobileAgent()) {
            if(agentHeader->getIpSequenceNumber() > am.getAckNo(agentHeader->getId())) {
                if(agentHeader->getIpSequenceNumber() > am.getSeqNo(agentHeader->getId())) {
                    if(agentHeader->getIsIpModified()) { // indicate a sequence update
                        if(agentHeader->getIpAddingField() > 0) { // check size of field
                            for(int i=0; i<agentHeader->getIpAddingField(); i++){
                                am.addIpToMap(agentHeader->getId(), agentHeader->getIPaddresses(i));
                            }
                        }
                        if(agentHeader->getIpRemovingField() > 0) {
                            for(int i=agentHeader->getIpAddingField(); i<(agentHeader->getIpRemovingField()+agentHeader->getIpAddingField()); i++) {
                                am.removeIpFromMap(agentHeader->getId(), agentHeader->getIPaddresses(i));
                            }
                        }
                        int s = agentHeader->getIpSequenceNumber();
                        int a = am.getAckNo(agentHeader->getId());
                        EV << "CA: Updating table to SEQ: " << s << " ACK: " << a << endl;
                        EV << "CA: IP Table: " << am.to_string() << endl;
                        // first all DA's are updated. after update confirmation, CA's ack is incremented and subsequently MA is confirmed.
                        if(agentAddressList.size() > 0)
                            createAgentUpdate(agentHeader->getId(), am.getSeqNo(agentHeader->getId()));
                        else {
                            am.setAckNo(agentHeader->getId(),am.getSeqNo(agentHeader->getId()));
                            sendSequenceUpdateResponse(destAddr, agentHeader->getId(), am.getSeqNo(agentHeader->getId()));
                        }
                    }
                } else if (agentHeader->getIpSequenceNumber() < am.getSeqNo(agentHeader->getId())) { // get retransmission from MA
                    EV << "CA: Received seq no of seq update is lower than the current seq no of CA. This should not occur. Check that." << endl;
                } else { // following code is executed if DA's hasnt acked but CA has updated its seq no. Just resend update message to DA, so that MA can be confirmed.
                    EV << "CA: Resend update message to DA because MA retransmitted seq update. update to seq: " << agentHeader->getIpSequenceNumber() << " from ack: " << am.getAckNo(agentHeader->getId()) << endl;
                    if(agentAddressList.size() > 0)
                        createAgentUpdate(agentHeader->getId(), am.getSeqNo(agentHeader->getId()));
                    else {
                        am.setAckNo(agentHeader->getId(),am.getSeqNo(agentHeader->getId()));
                        sendSequenceUpdateResponse(destAddr, agentHeader->getId(), am.getSeqNo(agentHeader->getId()));
                    }
                }
            } else {
                EV << "CA: Received an older seq update. The seq no is lower than the current ack no of CA. This should be not the regular case." << endl;
            }
        } else if (agentHeader->getIsDataAgent())
        { // updating seq no update by DA
            if(agentHeader->getIpSequenceNumber() > am.getAckNo(agentHeader->getId())) { // check if CA's ack is older than the received seq. if seq is not greater , CA updated all agents databas with new value but this message is an older one.
                if(agentHeader->getIpSequenceNumber() > am.getSeqNo(agentHeader->getId())) { //
                    if(agentHeader->getIsIpModified()) {  // indicate a sequence update: Attention: remValid is false
                        AddressManagement::IPv6AddressList ipList;
                        if(agentHeader->getIpAddingField() > 0) { // check size of field
                            if(agentHeader->getIpAddingField() != agentHeader->getIPaddressesArraySize()) throw cRuntimeError("CA:Hdr: field of array and field not same. Check header.");
                            for(int i=0; i<agentHeader->getIpAddingField(); i++){ // copy array in vector list
                                ipList.push_back(agentHeader->getIPaddresses(i)); // inserting elements of array into vector list due to function
                            }
                            am.insertSeqTableToMap(agentHeader->getId(), ipList, agentHeader->getIpSequenceNumber());
                            createAgentUpdate(agentHeader->getId(), agentHeader->getIpSequenceNumber());
                            EV << "CA: Received seq upd message earlier than MA. Updating other DA's." << endl;
                        } else { EV << "CA ERROR Received seq update message from DA but AddField is 0." << endl; }
                    } else { EV << "CA ERROR Received seq update message from DA but AddValid is false. (RemValid!=false)" << endl; }
                } else { EV << "CA: Received old seq update. seq: " << agentHeader->getIpSequenceNumber() << " seq_CA: " << am.getSeqNo(agentHeader->getId()) << endl; }
            } else { EV << "CA: Received seq update that is lower than ack. seq: " << agentHeader->getIpSequenceNumber() << " seq_CA: " << am.getSeqNo(agentHeader->getId()) << endl; }
        } else {throw cRuntimeError("CA: identifier of message wrong"); }
    } else {throw cRuntimeError("CA:seqUpdate: Id not found in list."); }
}

void ControlAgent::performFlowRequest(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    // you would here lookup for the best data agent, returning for simplicity just one address
    // check createAgentInit function and check array size of ipaddresses()
    IPv6Address nodeAddress = agentHeader->getIPaddresses(0);
    L3Address daAddr;
    const char *dataAgentAddr;
    dataAgentAddr = par("dataAgentAddress");
    L3AddressResolver().tryResolve(dataAgentAddr, daAddr); // TODO make it flexible
    EV << "CA: Received flow request. Node addr: " << nodeAddress << "; MATCHING agent addr: " << daAddr.toIPv6() << endl;
    sendFlowRequestResponse(destAddr, agentHeader->getId(), am.getSeqNo(agentHeader->getId()), daAddr.toIPv6(), nodeAddress);
}

void ControlAgent::performAgentInitResponse(IdentificationHeader *agentHeader, IPv6Address sourceAddr)
{
    if(cancelAndDeleteExpiryTimer(sourceAddr,-1, TIMERKEY_MA_INIT, agentHeader->getId())) {
        EV << "CA: Received Init Acknowledgment from DataAgent. Removed timer." << endl;
        // TODO sendFlowRequest should be executed from here
    } else {
        EV << "CA: No TIMER for DA init exists. So no initialization can be done." << endl;
    }
}

void ControlAgent::performAgentUpdateResponse(IdentificationHeader *agentHeader, IPv6Address sourceAddr)
{
    // if this is true, then a timer key existed. so a timer can only exist when the data agent were not initialized.
    // if false, then data agents has been initialized. only a timer for update can exists.
    cancelAndDeleteExpiryTimer(sourceAddr,-1,TIMERKEY_SEQ_UPDATE, agentHeader->getId(), agentHeader->getIpSequenceNumber(), am.getAckNo(agentHeader->getId()));
    bool allDataAgentsUpdated = true;
    for(IPv6Address ip : agentAddressList) {
        if(pendingExpiryTimer(ip,-1,TIMERKEY_SEQ_UPDATE, agentHeader->getId(), agentHeader->getIpSequenceNumber()))
            allDataAgentsUpdated = false;
    }
    if(allDataAgentsUpdated) {
        if(agentHeader->getIpSequenceNumber() > am.getAckNo(agentHeader->getId())) {
            am.setAckNo(agentHeader->getId(), agentHeader->getIpSequenceNumber()); // we set here ack no of CA when all DA's has been updated.
            AddressManagement::AddressChange ac = am.getAddressEntriesOfSeqNo(agentHeader->getId(),am.getSeqNo(agentHeader->getId()));
            if(ac.addedAddresses > 0) { // any interafce is provided but it's not sure if these are also reachable. so mobile node has to ensure that seq update is receiveable
                for(int i=0; i<ac.addedAddresses; i++) {
                    sendSequenceUpdateResponse(ac.getAddedIPv6AddressList.at(i), agentHeader->getId(), agentHeader->getIpSequenceNumber());
                }
                EV << "CA: Response update message from MA over " << ac.addedAddresses << " interfaces. Seq: " << am.getAckNo(agentHeader->getId()) << endl;
            }
        }
        EV << "CA: Update message from all DA's received." << endl;
    } else {
        EV << "CA: Received from DataAgent. Removed timer for seq: " << agentHeader->getIpSequenceNumber() << endl;
    }
}


InterfaceEntry *ControlAgent::getInterface() { // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal())
            return ie;
    }
    return ie;
}

void ControlAgent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, simtime_t delayTime) {
    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
    ctrlInfo->setProtocol(IP_PROT_IPv6EXT_ID); // todo must be adjusted
    ctrlInfo->setDestAddr(destAddr);
    ctrlInfo->setHopLimit(255);
    InterfaceEntry *ie = getInterface();
    if(ie) {
        ctrlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
        ctrlInfo->setInterfaceId(ie->getInterfaceId());
    } else { throw cRuntimeError("CA:send2LowerLayer no interface provided."); }
    msg->setControlInfo(ctrlInfo);
    cGate *outgate = gate("toLowerLayer");
//    EV << "CA2IP: Dest=" << ctrlInfo->getDestAddr() << " Src=" << ctrlInfo->getSrcAddr() << " If=" << ctrlInfo->getInterfaceId() << " Delay=" << delayTime.str() << endl;
    if (delayTime > 0)
        sendDelayed(msg, delayTime, outgate);
    else
        send(msg, outgate);
}

} //namespace
