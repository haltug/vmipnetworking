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

#include "inet/common/NotifierConsts.h"
#include "inet/common/lifecycle/NodeStatus.h"

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
            IdentificationHeader *idHdr = (IdentificationHeader *) msg;
            if (dynamic_cast<DataAgentHeader *>(idHdr)) {
                DataAgentHeader *da = (DataAgentHeader *) idHdr;
                processDataAgentMessage(da, controlInfo);
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

void ControlAgent::createAgentInit(uint64 mobileId)
{
    EV << "CA: Create DA_MobileId_init" << endl;
    const char *dataAgentAddr = par("dataAgentAddress");
    cStringTokenizer tokenizer(dataAgentAddr);
    while (tokenizer.hasMoreTokens()) {
        L3Address daAddr;
        L3AddressResolver().tryResolve(tokenizer.nextToken(), daAddr);
        if(daAddr.toIPv6().isGlobal()) {
            cMessage *msg = new cMessage("sendingDAinit", MSG_MA_INIT);
            InterfaceEntry *ife = getInterface(daAddr.toIPv6());
            TimerKey key(daAddr.toIPv6(),ife->getInterfaceId(),TIMERKEY_MA_INIT, mobileId);
            if(std::find(nodeAddressList.begin(), nodeAddressList.end(), daAddr.toIPv6()) == nodeAddressList.end())
                nodeAddressList.push_back(daAddr.toIPv6());
            MobileInitTimer *mit = (MobileInitTimer *) getExpiryTimer(key,TIMERTYPE_MA_INIT);
            mit->dest = daAddr.toIPv6();
            mit->ie = ife;
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
{
    EV << "CA: Send init_to_DA" << endl;
    MobileInitTimer *mit = (MobileInitTimer *) msg->getContextPointer();
    InterfaceEntry *ie = mit->ie;
    const IPv6Address &dest = mit->dest;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress();
    mit->nextScheduledTime = simTime() + mit->ackTimeout;
    mit->ackTimeout = (mit->ackTimeout)*2;
    for(auto &item : am.getAddressMap()) {
        MobileAgentHeader *mah = new MobileAgentHeader("da_seq_init");
        mah->setId(item.first);
        mah->setIdInit(true);
        mah->setIdAck(false);
        mah->setSeqValid(true);
        mah->setAckValid(false);
        mah->setAddValid(true);
        mah->setRemValid(false);
        mah->setNextHeader(IP_PROT_NONE);
        mah->setIpSequenceNumber(am.getCurrentSequenceNumber(item.first));
        mah->setIpAcknowledgementNumber(0);
        mah->setIpAddingField(1);
        mah->setAddedAddressesArraySize(1);
        mah->setAddedAddresses(0,src);
        mah->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
        sendToLowerLayer(mah, dest, src);
    }
    scheduleAt(mit->nextScheduledTime, msg);
}

void ControlAgent::createAgentUpdate(uint64 mobileId)
{
    EV << "CA: Create Agent update message to all DA's" << endl;
    for(IPv6Address nodeAddr : nodeAddressList) {
        cMessage *msg = new cMessage("sendingDAseqUpdate", MSG_AGENT_UPDATE);
        InterfaceEntry *ie = getInterface(nodeAddr);
        TimerKey key(nodeAddr, ie->getInterfaceId(), TIMERKEY_SEQ_UPDATE, mobileId);
        SequenceUpdateTimer *sut = (SequenceUpdateTimer *) getExpiryTimer(key, TIMERTYPE_SEQ_UPDATE);
        sut->dest = nodeAddr;
        sut->ie = ie;
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
    EV << "CA: Send agent update" << endl;
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
    InterfaceEntry *ie = sut->ie;
    const IPv6Address &dest =  sut->dest;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress(); // to get src ip
    sut->nextScheduledTime = simTime() + sut->ackTimeout;
    sut->ackTimeout = (sut->ackTimeout)*2;
    MobileAgentHeader *mah = new MobileAgentHeader("da_seq_upd");
    mah->setId(sut->id);
    mah->setIdInit(true);
    mah->setIdAck(false);
    mah->setSeqValid(true);
    mah->setAckValid(false);
    mah->setAddValid(true);
    mah->setRemValid(false);
    mah->setNextHeader(IP_PROT_NONE);
    mah->setIpSequenceNumber(am.getCurrentSequenceNumber(sut->id));
    mah->setIpAcknowledgementNumber(0);
    uint seq = am.getCurrentSequenceNumber(sut->id);
    AddressManagement::AddressChange ac = am.getAddessEntriesOfSequenceNumber(sut->id,seq);
    mah->setIpAddingField(ac.addedAddresses);
    mah->setAddedAddressesArraySize(ac.addedAddresses);
    if(ac.addedAddresses > 0) {
        if(ac.addedAddresses != ac.getUnacknowledgedAddedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqUpd: value of Add list must have size of integer.");
        for(int i=0; i<ac.addedAddresses; i++) {
            mah->setAddedAddresses(i,ac.getUnacknowledgedAddedIPv6AddressList.at(i));
        }
    }
    mah->setIpRemovingField(0);
    mah->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*ac.addedAddresses));
    sendToLowerLayer(mah, dest, src); // TODO select interface
    scheduleAt(sut->nextScheduledTime, msg);
}

void ControlAgent::sendSequenceUpdateAck(uint64 mobileId)
{
    InterfaceEntry *ie = getInterface();
    IPv6Address sourceAddr = ie->ipv6Data()->getPreferredAddress();
    AddressManagement::IPv6AddressList addressList = am.getCurrentAddressList(mobileId);
    for (IPv6Address ip : addressList ) {
        IPv6Address destAddr = ip; // address to be responsed
        ControlAgentHeader *cah = new ControlAgentHeader("ma_seq_upd_ack");
        cah->setIdInit(true);
        cah->setIdAck(true);
        cah->setSeqValid(true);
        cah->setCacheAddrAck(false);
        cah->setForwardAddrAck(false);
        cah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
        sendToLowerLayer(cah,destAddr, sourceAddr);
    }
}

void ControlAgent::sendSessionInitResponse(IPv6Address destAddr, IPv6Address sourceAddr)
{
    ControlAgentHeader *cah = new ControlAgentHeader("ma_init_ack");
    cah->setIdInit(true);
    cah->setIdAck(true);
    cah->setSeqValid(false);
    cah->setIpSequenceNumber(0);
    sendToLowerLayer(cah,destAddr,sourceAddr);
}

void ControlAgent::sendSequenceInitResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId)
{
    ControlAgentHeader *cah = new ControlAgentHeader("ma_seq_ack");
    cah->setIdInit(true);
    cah->setIdAck(true);
    cah->setSeqValid(true);
    cah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    sendToLowerLayer(cah,destAddr, sourceAddr);
    createAgentInit(mobileId);
}

void ControlAgent::sendSequenceUpdateResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId)
{
    ControlAgentHeader *cah = new ControlAgentHeader("ma_seq_update");
    cah->setIdInit(true);
    cah->setIdAck(true);
    cah->setSeqValid(true);
    cah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    sendToLowerLayer(cah,destAddr,sourceAddr);
//    createAgentUpdate(mobileId); // we notify all dataAgents when we response to mobile agents seq update
}

void ControlAgent::sendFlowRequestResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId, IPv6Address nodeAddr, IPv6Address agentAddr)
{
    ControlAgentHeader *cah = new ControlAgentHeader("ma_flow_req");
    cah->setIdInit(true);
    cah->setIdAck(true);
    cah->setSeqValid(true);
    cah->setCacheAddrAck(true);
    cah->setForwardAddrAck(false);
    cah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    cah->setNodeAddress(nodeAddr);
    cah->setAgentAddress(agentAddr);
    cah->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR*2);
    sendToLowerLayer(cah,destAddr,sourceAddr);
}

void ControlAgent::processMobileAgentMessage(MobileAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
//        EV << "CA: processing message from MA" << endl;
    // ================================================================================
    // controlAgent message processing
    // ================================================================================
    IPv6Address destAddr = controlInfo->getSrcAddr(); // address to be responsed
    IPv6Address sourceAddr = controlInfo->getDestAddr();
    if(agentHeader->getIdInit() && !agentHeader->getIdAck()) // check for other bits
    {   // init process request
        if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
            EV << "CA: Id already exists. Skipping initializing, check for mistakes." << endl;
        } else {
            EV << "CA: Received session Init message. Adding id to list: " << agentHeader->getId() << endl;
            mobileIdList.push_back(agentHeader->getId()); // adding id to list (registering id)
            sendSessionInitResponse(destAddr, sourceAddr);
        }
    } else
        if (agentHeader->getSeqValid() && !agentHeader->getAckValid() && agentHeader->getIdInit() && agentHeader->getIdAck()) // || agentHdr->getIdInit() && agentHdr->getIdAck()
    {   // init sequence number/address management
        if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
            bool addrMgmtEntry = am.insertNewId(agentHeader->getId(), agentHeader->getIpSequenceNumber(), agentHeader->getAddedAddresses(0));//first number
            if(addrMgmtEntry) { // check if seq and id is inserted
                EV << "CA: Received sequence initialize message from MA. Initialized sequence number: " << agentHeader->getIpSequenceNumber() << "; ip: " << agentHeader->getAddedAddresses(0) << endl;
                sendSequenceInitResponse(destAddr, sourceAddr, agentHeader->getId());
            }
            else {
                if(am.isIdInitialized(agentHeader->getId()))
                    EV << "+++++ERROR+++++ CA: Id has been initialized before. Why do you send me again an init message?" << endl;
                else
                    throw cRuntimeError("CA: Initialization of sequence number failed, CA could not insert id in AddrMgmt-Unit.");
            }
        }
        else
            throw cRuntimeError("CA should initialize sequence number but cannot find id in list.");
    } else
        if(agentHeader->getSeqValid() && agentHeader->getAckValid() && agentHeader->getIdInit() && agentHeader->getIdAck()) {
            // Responsing to seqNo update
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end())
            { // updating seq no
                if(agentHeader->getAddValid() || agentHeader->getRemValid()) { // indicate a sequence update
                    if(agentHeader->getIpSequenceNumber() != am.getCurrentSequenceNumber(agentHeader->getId())) {
                        if(agentHeader->getIpAddingField() > 0) { // check size of field
                            if(agentHeader->getIpAddingField() != agentHeader->getAddedAddressesArraySize()) throw cRuntimeError("CA:Hdr: field of array and field not same. Check header.");
                            for(int i=0; i<agentHeader->getIpAddingField(); i++){
                                am.addIPv6AddressToAddressMap(agentHeader->getId(), agentHeader->getAddedAddresses(i));
                            }
                        }
                        if(agentHeader->getIpRemovingField() > 0) {
                            if(agentHeader->getIpRemovingField() != agentHeader->getRemovedAddressesArraySize()) throw cRuntimeError("CA:Hdr: field of array and field not same. Check header.");
                            for(int i=0; i<agentHeader->getIpRemovingField(); i++) {
                                am.removeIPv6AddressFromAddressMap(agentHeader->getId(), agentHeader->getRemovedAddresses(i));
                            }
                        }
                        am.setLastAcknowledgemnt(agentHeader->getId(), agentHeader->getIpSequenceNumber());
                        EV << "CA: Received update message. update to seq: " << agentHeader->getIpSequenceNumber() << endl;
                        createAgentUpdate(agentHeader->getId()); // we notify all dataAgents when we response to mobile agents seq update but we first start with DA and then inform MA
//                            sendSequenceUpdateResponse(destAddr, sourceAddr, agentHeader->getId());
                    }
                }
                // flow request response
                if(agentHeader->getCacheAddrInit() || agentHeader->getForwardAddrInit()) { // check if redirect address is requested
                    IPv6Address nodeAddress = agentHeader->getNodeAddress();
                    // you would here lookup for the best data agent, returning for simplicity just one address
                    L3Address daAddr;
                    const char *dataAgentAddr;
                    dataAgentAddr = par("dataAgentAddress");
                    L3AddressResolver().tryResolve(dataAgentAddr, daAddr);
                    sendFlowRequestResponse(destAddr, sourceAddr, agentHeader->getId(), nodeAddress, daAddr.toIPv6());
                }
        } else { throw cRuntimeError("CA: Mobile id not known."); }
    } else { throw cRuntimeError("CA: Message not known. Parameter bits of header are not set correctly."); }
    delete agentHeader;
    delete controlInfo;
}

void ControlAgent::processDataAgentMessage(DataAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    IPv6Address &caAddr = controlInfo->getSrcAddr();
    InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
    if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid() && agentHeader->getIdValid()) {
        if(!cancelAndDeleteExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_MA_INIT, agentHeader->getId())) { // check with pending
            // if this is true, then a timer key existed. so a timer can only exist when the data agent were not initialized.
            // if false, then data agents has been initialized. only a timer for update can exists.
            cancelAndDeleteExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_SEQ_UPDATE, agentHeader->getId());
            EV << "CA: Received DA update msg. Agent update timer removed. Process successfully finished." << endl;
        } else
            EV << "CA: Received DA ack. Agent init timer removed if there was one. Agent init process successfully finished." << endl;
        bool allDataAgentsUpdated = true;
        for(IPv6Address ip : nodeAddressList) {
            if(pendingExpiryTimer(ip,ie->getInterfaceId(),TIMERKEY_SEQ_UPDATE, agentHeader->getId()))
                allDataAgentsUpdated = false;
        }
        if(allDataAgentsUpdated) {
            IPv6Address sourceAddr = controlInfo->getDestAddr();
            AddressManagement::AddressChange ac = am.getAddessEntriesOfSequenceNumber(agentHeader->getId(),am.getCurrentSequenceNumber(agentHeader->getId()));
            if(ac.addedAddresses > 0) { // any interafce is provided but it's not sure if these are also reachable. so mobile node has to ensure that seq update is receiveable
                for(int i=0; i<ac.addedAddresses; i++) {
                    sendSequenceUpdateResponse(ac.getUnacknowledgedAddedIPv6AddressList.at(i), sourceAddr, agentHeader->getId());
                }
            }
        }
    } else
        EV << "MA: Session created yet. Why do I received it again?" << endl;
    delete agentHeader; // delete at this point because it's not used any more
    delete controlInfo;
}

InterfaceEntry *ControlAgent::getInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol) { // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal()) { return ie; }
    }
    return ie;
}

void ControlAgent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr, int interfaceId, simtime_t delayTime) {
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
