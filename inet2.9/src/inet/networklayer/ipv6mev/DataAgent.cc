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
#include "inet/transportlayer/tcp_common/TCPSegment.h"

namespace inet {

Define_Module(DataAgent);

simsignal_t DataAgent::numMobileAgents = registerSignal("numMobileAgents");
simsignal_t DataAgent::numFlows = registerSignal("numFlows");
simsignal_t DataAgent::incomingTrafficPktNode = registerSignal("incomingTrafficPktNode");
simsignal_t DataAgent::outgoingTrafficPktNode = registerSignal("outgoingTrafficPktNode");
simsignal_t DataAgent::incomingTrafficSizeNode = registerSignal("incomingTrafficSizeNode");
simsignal_t DataAgent::outgoingTrafficSizeNode = registerSignal("outgoingTrafficSizeNode");
simsignal_t DataAgent::incomingTrafficPktAgent = registerSignal("incomingTrafficPktAgent");
simsignal_t DataAgent::outgoingTrafficPktAgent = registerSignal("outgoingTrafficPktAgent");
simsignal_t DataAgent::incomingTrafficSizeAgent = registerSignal("incomingTrafficSizeAgent");
simsignal_t DataAgent::outgoingTrafficSizeAgent = registerSignal("outgoingTrafficSizeAgent");

DataAgent::~DataAgent() {
    auto it = expiredTimerList.begin();
    while(it != expiredTimerList.end()) {
        TimerKey key = it->first;
        it++;
        cancelAndDeleteExpiryTimer(key.dest,key.interfaceID,key.type);
    }
}

void DataAgent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        WATCH(numMobileAgentStat);
        WATCH(numFlowStat);
        WATCH(incomingTrafficPktNodeStat);
        WATCH(outgoingTrafficPktNodeStat);
        WATCH(incomingTrafficPktAgentStat);
        WATCH(outgoingTrafficPktAgentStat);
        WATCH(*this);
        if(hasPar("enableNodeRequesting"))
            enableNodeRequesting = par("enableNodeRequesting").boolValue();
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
        IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
        ipSocket.registerProtocol(IP_PROT_IPv6_ICMP);
        ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
        IPSocket ipSocket2(gate("udpOut")); // TODO test if one can register from first socket
        ipSocket2.registerProtocol(IP_PROT_UDP);
        IPSocket ipSocket3(gate("tcpOut"));
        ipSocket3.registerProtocol(IP_PROT_TCP);
        sessionState = UNASSOCIATED;
    }
    if (stage == INITSTAGE_APPLICATION_LAYER) {
    }
}

void DataAgent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {
        if(msg->getKind() == MSG_SEQ_UPDATE_NOTIFY)
            sendSequenceUpdateNotification(msg);
        else if(msg->getKind() == MSG_UPDATE_ACK) { // from CA for retransmitting of da_init
            sendAgentUpdateResponse(msg);
        }
        else
            throw cRuntimeError("handleMessage: Unknown selfMessage received. This could be an unknown timer with wrong kind.");
    }
    else if(msg->arrivedOn("fromLowerLayer")) {
        if (dynamic_cast<IdentificationHeader *> (msg)) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processAgentMessage((IdentificationHeader *) msg, controlInfo);
        } else {
            EV_WARN << "handleMessage: Received of gate <fromLowerLayer> unknown message type." << endl;
            delete msg;
        }
    } else if(msg->arrivedOn("icmpIpIn")) {
        if (dynamic_cast<ICMPv6Message *> (msg)) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processIncomingIcmpPacket((ICMPv6Message *)msg, controlInfo);
        } else {
            EV_WARN << "handleMessage: Received of gate <icmpIpIn> unknown message type." << endl;
            delete msg;
        }
    } else if(msg->arrivedOn("udpIn")) {
        processUdpFromNode(msg);
    } else if(msg->arrivedOn("tcpIn")) {
        processTcpFromNode(msg);
    } else if(msg->arrivedOn("icmpIn")) {
        processOutgoingIcmpPacket(msg);
    } else
        throw cRuntimeError("handleMessage: Unknown message received.");
}

void DataAgent::createSequenceUpdateNotificaiton(uint64 mobileId, uint seq)
{
    if(!ControlAgentAddress.isGlobal())
        throw cRuntimeError("DA_createSequenceUpdateNotificaiton: Address od Control Agent is not set. Cannot send an update message without an address.");
    TimerKey key(ControlAgentAddress,-1,TIMERKEY_SEQ_UPDATE_NOT, mobileId, getSeqNo(mobileId));
    UpdateNotifierTimer *unt = (UpdateNotifierTimer *) getExpiryTimer(key,TIMERTYPE_SEQ_UPDATE_NOT); // this timer must explicitly deleted
    if(!unt->active) { // a timer has not been created earlier
        cMessage *msg = new cMessage("sendingCAseqUpd", MSG_SEQ_UPDATE_NOTIFY);
        unt->dest = ControlAgentAddress;
        unt->timer = msg;
        unt->ackTimeout = TIMEOUT_SEQ_UPDATE;
        unt->nextScheduledTime = simTime();
        unt->id = mobileId;
//        unt->ack = am.getAckNo(mobileId);
        unt->ack = getAckNo(mobileId);
        unt->seq = seq;
        unt->liftime = simTime();
        unt->active = true;
        msg->setContextPointer(unt);
        scheduleAt(unt->nextScheduledTime,msg);
        EV_DEBUG << "DA_createSequenceUpdateNotificaiton: Sending sequence update acknowledgment to Control Agent for Mobile Agent ("<< mobileId << ")" << endl;
    } else {
        if((simTime() - unt->liftime) >= TIMEOUT_SEQ_UPDATE) {
            // if a certain time past, delete the message in queue (next schedule time could be to high) and send the message again.
            unt->active = false;
            cancelAndDelete(unt->timer);
            createSequenceUpdateNotificaiton(unt->id, unt->seq);
        } else {
            // Just wait
        }
    }
}

void DataAgent::sendSequenceUpdateNotification(cMessage *msg)
{
    UpdateNotifierTimer *unt = (UpdateNotifierTimer *) msg->getContextPointer();
    const IPv6Address &dest =  unt->dest;
    unt->nextScheduledTime = simTime() + unt->ackTimeout;
    unt->ackTimeout = (unt->ackTimeout)*1.5;
    IdentificationHeader *ih = getAgentHeader(3, IP_PROT_NONE, getSeqNo(unt->id), 0, unt->id);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsIpModified(true);
    AddressDiff ad = getAddressList(unt->id,unt->seq);
    ih->setIPaddressesArraySize(ad.insertedList.size());
    ih->setIpAddingField(ad.insertedList.size());
    if(ad.insertedList.size() > 0) {
        for(int i=0; i < ad.insertedList.size(); i++)
            ih->setIPaddresses(i,ad.insertedList.at(i).address);
    }
    ih->setIpRemovingField(0);
    ih->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*ad.insertedList.size()));
    sendToLowerLayer(ih, dest);
    scheduleAt(unt->nextScheduledTime, msg);
}

void DataAgent::sendAgentInitResponse(IPv6Address destAddr, uint64 mobileId, uint seq)
{
    IdentificationHeader *ih = getAgentHeader(3, IP_PROT_NONE, getSeqNo(mobileId), 0, mobileId);
    ih->setIsIdInitialized(true);
    ih->setIsSeqValid(true);
    sendToLowerLayer(ih,destAddr);
}

void DataAgent::createAgentUpdateResponse(IPv6Address destAddr, uint64 mobileId, uint seq)
{
    TimerKey key(ControlAgentAddress,-1,TIMERKEY_UPDATE_ACK, mobileId, seq);
    UpdateAckTimer *uat = (UpdateAckTimer *) getExpiryTimer(key,TIMERTYPE_UPDATE_ACK); // this timer must explicitly deleted
    cMessage *msg = new cMessage("sendingCAseqUpdAck", MSG_UPDATE_ACK);
    uat->dest = ControlAgentAddress;
    uat->timer = msg;
    uat->nextScheduledTime = simTime();
    uat->id = mobileId;
    uat->seq = seq;
    uat->ackTimeout = TIMEOUT_SEQ_UPDATE;
    uat->nextScheduledTime = simTime();
    msg->setContextPointer(uat);
    scheduleAt(uat->nextScheduledTime,msg);
    EV_DEBUG << "DA_createAgentUpdateResponse: Sending sequence update message (" << seq << ") for Mobile Agent(" << mobileId << ") to Control Agent." << endl;
}

void DataAgent::sendAgentUpdateResponse(cMessage *msg)
{
    UpdateAckTimer *uat = (UpdateAckTimer *) msg->getContextPointer();
    const IPv6Address &dest =  uat->dest;
    uat->nextScheduledTime = simTime() + uat->ackTimeout;
    uat->ackTimeout = (uat->ackTimeout)*1.2;
    IdentificationHeader *ih = getAgentHeader(3, IP_PROT_NONE, getSeqNo(uat->id), 0, uat->id);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    sendToLowerLayer(ih, dest);
    scheduleAt(uat->nextScheduledTime, msg);
}

// TODO introduce state for data agent, for init.
void DataAgent::processAgentMessage(IdentificationHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    IPv6Address destAddr = controlInfo->getSourceAddress().toIPv6(); // address to be responsed
    if(agentHeader->getIsControlAgent() && (agentHeader->getNextHeader() == IP_PROT_NONE)) {
        if(sessionState == UNASSOCIATED) {
            sessionState = ASSOCIATED; // indicates if data agent is associated to a CA
            if(ControlAgentAddress.isUnspecified()) ControlAgentAddress = destAddr;
        }
        if(agentHeader->getIsIdInitialized() && !agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid()  && agentHeader->getIsIpModified())
            performAgentInit(agentHeader, destAddr);
        else if(agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid()  && agentHeader->getIsIpModified())
            performAgentUpdate(agentHeader, destAddr);
        else if(agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid()  && !agentHeader->getIsIpModified())
            performSequenceUpdateResponse(agentHeader, destAddr);
        else
            throw cRuntimeError("DA: Header of CA msg is not known");
    } else if(agentHeader->getIsMobileAgent() && agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && agentHeader->getIsAckValid()) {
        if(agentHeader->getIsIpModified())
            performSeqUpdate(agentHeader);
        if(agentHeader->getNextHeader() == IP_PROT_UDP && agentHeader->getIsWithNodeAddr())
            processUdpFromAgent(agentHeader, destAddr);
        else if(agentHeader->getNextHeader() == IP_PROT_TCP && agentHeader->getIsWithNodeAddr())
            processTcpFromAgent(agentHeader, destAddr);
        else if(agentHeader->getNextHeader() == IP_PROT_IPv6_ICMP && agentHeader->getIsWithNodeAddr())
            processIcmpFromAgent(agentHeader, destAddr);
        else
            throw cRuntimeError("DA: packet type from MA not known");
    } else
        throw cRuntimeError("DA: received header has wrong paramters. no assignment possible");
    delete agentHeader;
    delete controlInfo;
}

void DataAgent::performAgentInit(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) == mobileIdList.end())
    { // if mobile is not registered. insert a new entry in map
        mobileIdList.push_back(agentHeader->getId());
        EV_DEBUG << "DA_performAgentInit: Received initialization message from Control Agent. Adding ID " << agentHeader->getId() << endl;
        if(agentHeader->getIpAddingField() > 0) { // check size of field
            bool addrMgmtEntry = initAddressMap(agentHeader->getId(), agentHeader->getIpSequenceNumber(), agentHeader->getIPaddresses(0)); //first number
            if(!addrMgmtEntry) // if we receive for the first time an init message, then response to this one
                throw cRuntimeError("DA_performAgentInit: Could not insert id.");
            AddressList list;
            if(agentHeader->getIpAddingField() > 0) { // check size of field
                if(agentHeader->getIpAddingField() != agentHeader->getIPaddressesArraySize()) throw cRuntimeError("DA_performAgentInit: Header field is not correctly declared.");
                for(int i=0; i<agentHeader->getIpAddingField(); i++){ // copy array in vector list
                    AddressTuple tuple(0, agentHeader->getIPaddresses(i));
                    list.push_back(tuple); // inserting elements of array into vector list due to function
                }
            } else
                throw cRuntimeError("DA_performAgentInit: Header contains no IP address.");
            insertTable(agentHeader->getId(), agentHeader->getIpSequenceNumber(), list);
        } else
            throw cRuntimeError("DA_performAgentInit: Initialization message must contain the start message of Mobile Agent.");
        setAckNo(agentHeader->getId(), agentHeader->getIpSequenceNumber());
    }
    numMobileAgentStat++;
    emit(numMobileAgents,numMobileAgentStat);
    sendAgentInitResponse(destAddr, agentHeader->getId(), getSeqNo(agentHeader->getId()));
}

void DataAgent::performAgentUpdate(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(ControlAgentAddress.isUnspecified())
        throw cRuntimeError("DA:procMA: CA address must be known at this stage. Stage= seq update.");
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
        AddressList list;
        if(agentHeader->getIpAddingField() > 0) { // check size of field
            if(agentHeader->getIpAddingField() != agentHeader->getIPaddressesArraySize()) throw cRuntimeError("DA_performAgentUpdate: Header field is not correctly declared.");
            for(int i=0; i<agentHeader->getIpAddingField(); i++){ // copy array in vector list
                AddressTuple tuple(0, agentHeader->getIPaddresses(i));
                list.push_back(tuple); // inserting elements of array into vector list due to function
            }
        }
        insertTable(agentHeader->getId(),agentHeader->getIpSequenceNumber(), list);
        cancelAndDeleteExpiryTimer(ControlAgentAddress, -1, TIMERKEY_SEQ_UPDATE_NOT, agentHeader->getId(), agentHeader->getIpSequenceNumber(), getAckNo(agentHeader->getId()));
        setAckNo(agentHeader->getId(),agentHeader->getIpSequenceNumber()); //
        createAgentUpdateResponse(ControlAgentAddress, agentHeader->getId(),agentHeader->getIpSequenceNumber());
    } else
        throw cRuntimeError("DA:procMA: Id is not inserted in map. Register id before seq update.");
}

void DataAgent::performSequenceUpdateResponse(IdentificationHeader *agentHeader, IPv6Address destAddr)
{ // just remove the timer for agentUpdateResponse.
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end())
        cancelAndDeleteExpiryTimer(ControlAgentAddress, -1, TIMERKEY_UPDATE_ACK, agentHeader->getId(), agentHeader->getIpSequenceNumber(), getAckNo(agentHeader->getId()));
    else
       throw cRuntimeError("DA_performSequenceUpdateResponse: Map contains no matching ID.");
}

void DataAgent::performSeqUpdate(IdentificationHeader *agentHeader)
{
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
       if(agentHeader->getIpAcknowledgementNumber() > getAckNo(agentHeader->getId())) { // check for error
           throw cRuntimeError("DA_performSeqUpdate: AckNo in header of Mobile Agent is lower than the registered AckNo.");
        } else {
            if(agentHeader->getIpSequenceNumber() > getAckNo(agentHeader->getId())) {
                if(agentHeader->getIpSequenceNumber() > getSeqNo(agentHeader->getId())) {
                    if(agentHeader->getIsIpModified()) { // indicate a sequence update
                        // when ackNo of update message is behind current ackNo, sequence number is decremented for starting the update process at ackNo.
                        // Otherwise update process starts at seqNo, but header only declares difference from ackNo to seqNo ( and not from current seqNo to the pointed seqNo);
                        if(agentHeader->getIpAcknowledgementNumber() < getAckNo(agentHeader->getId()))
                            setSeqNo(agentHeader->getId(),agentHeader->getIpAcknowledgementNumber());
                        if(agentHeader->getIpAddingField() > 0) { // check size of field
                            for(int i=0; i<agentHeader->getIpAddingField(); i++){
                                insertAddress(agentHeader->getId(), 0, agentHeader->getIPaddresses(i+1));
                            }
                        }
                        if(agentHeader->getIpRemovingField() > 0) {
                            for(int i=0; i<agentHeader->getIpRemovingField(); i++) {
                                deleteAddress(agentHeader->getId(), 0, agentHeader->getIPaddresses(i+agentHeader->getIpAddingField()+1));
                            }
                        }
                        int s = agentHeader->getIpSequenceNumber(); int a = getAckNo(agentHeader->getId()); int in = agentHeader->getIpAddingField(); int out =  agentHeader->getIpRemovingField();
                        EV_INFO << "DA_performSeqUpdate: Extracted higher sequence number from Mobile Agent(" << agentHeader->getId() << ").\nUpdating table to SeqNo " << s << "and AckNo " << a << "(+" << in << ", -" << out << ")." << endl;
                        createSequenceUpdateNotificaiton(agentHeader->getId(), getSeqNo(agentHeader->getId()));
                    } else
                        throw cRuntimeError("DA_performSeqUpdate: Received msg from MA with different Seq/Ack number but no changed IP Addresses are provided");
                } else if (agentHeader->getIpSequenceNumber() > getSeqNo(agentHeader->getId())) {
                    createSequenceUpdateNotificaiton(agentHeader->getId(), getSeqNo(agentHeader->getId()));
                } else
                    throw cRuntimeError("DA_performSeqUpdate: Seq number of DA is greater than seq number of MA. Not possible. MA is incrementing");
            }
        }
    } else
        throw cRuntimeError("DA: Received message from MA but it's not in the id list");
}

void DataAgent::processUdpFromAgent(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
        incomingTrafficPktAgentStat++;
        emit(incomingTrafficPktAgent, incomingTrafficPktAgentStat);
        incomingTrafficSizeAgentStat = agentHeader->getByteLength();
        emit(incomingTrafficSizeAgent, incomingTrafficSizeAgentStat);
        if(agentHeader->getIPaddressesArraySize() < 1)
            throw cRuntimeError("processUdpFromAgent: ArraySize = 0. Check message processing unit from MA. At least one IP address of target must be contained.");
        IPv6Address nodeAddress = agentHeader->getIPaddresses(0);
        cPacket *packet = agentHeader->decapsulate(); // decapsulate(remove) agent header
        if (dynamic_cast<UDPPacket *>(packet) != nullptr) {
            UDPPacket *udpPacket =  (UDPPacket *) packet;
            FlowTuple tuple;
            tuple.protocol = IP_PROT_UDP;
            tuple.destAddress = nodeAddress;
            tuple.interfaceId = nodeAddress.getInterfaceId();
            if(enableNodeRequesting) {
                tuple.destPort = 0;
                tuple.sourcePort = 0;
            } else {
                tuple.destPort = udpPacket->getDestinationPort();
                tuple.sourcePort = udpPacket->getSourcePort();
            }
//            EV << "DA: Creating UDP FlowTuple ADDR:" << tuple.destAddress << " DP:" << tuple.destPort << " SP:"<< tuple.sourcePort << endl;
            FlowUnit *funit = getFlowUnit(tuple);
            if(funit->state == UNREGISTERED) {
                EV_DEBUG << "DA_processUdpFromAgent: Initializing flow unit from: "<< destAddr.str() << endl;
                funit->state = REGISTERED;
                funit->isFlowActive = true;
                funit->isAddressCached = false;
                funit->id = agentHeader->getId();
                funit->mobileAgent = destAddr;
                funit->nodeAddress = nodeAddress;
                numFlowStat++;
                emit(numFlows,numFlowStat);
                // other fields are not initialized.
            }
            if (funit->state == REGISTERED) {
                funit->mobileAgent = destAddr; // just update return address
                InterfaceEntry *ie = getInterface();
                IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
                ipControlInfo->setProtocol(IP_PROT_UDP);
                ipControlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
                ipControlInfo->setDestAddr(funit->nodeAddress);
                ipControlInfo->setInterfaceId(ie->getInterfaceId());
                ipControlInfo->setHopLimit(255);
                udpPacket->setControlInfo(ipControlInfo);
                outgoingTrafficPktNodeStat++;
                emit(outgoingTrafficPktNode, outgoingTrafficPktNodeStat);
                outgoingTrafficSizeNodeStat = udpPacket->getByteLength();
                emit(outgoingTrafficSizeNode, outgoingTrafficSizeNodeStat);
                cGate *outgate = gate("toLowerLayer");
                send(udpPacket, outgate);
            } else
                throw cRuntimeError("DA:procMAmsg: UDP packet could not processed. FlowUnit unknown.");
        } else
            throw cRuntimeError("DA:procMAmsg: UDP packet could not be cast.");
    } else
        throw cRuntimeError("DA: Received udp packet from MA but its id not in the list");
}

void DataAgent::processTcpFromAgent(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
        incomingTrafficPktAgentStat++;
        emit(incomingTrafficPktAgent, incomingTrafficPktAgentStat);
        incomingTrafficSizeAgentStat = agentHeader->getByteLength();
        emit(incomingTrafficSizeAgent, incomingTrafficSizeAgentStat);
        if(agentHeader->getIPaddressesArraySize() < 1)
            throw cRuntimeError("DA: ArraySize = 0. Check message processing unit from MA. At least one IP address of target must be contained.");
        IPv6Address nodeAddress = agentHeader->getIPaddresses(0);
        cPacket *packet = agentHeader->decapsulate(); // decapsulate(remove) agent header
        if (dynamic_cast<tcp::TCPSegment *>(packet) != nullptr) {
            tcp::TCPSegment *tcpseg =  (tcp::TCPSegment *) packet;
            FlowTuple tuple;
            tuple.protocol = IP_PROT_TCP;
            if(enableNodeRequesting) {
                tuple.destPort = 0;
                tuple.sourcePort = 0;
            } else {
                tuple.destPort = tcpseg->getDestinationPort();
                tuple.sourcePort = tcpseg->getSourcePort();
            }
            tuple.destAddress = nodeAddress;
            tuple.interfaceId = nodeAddress.getInterfaceId();
//            EV << "DA: Creating TCP FlowTuple ADDR:" << tuple.destAddress << " DP:" << tuple.destPort << " SP:"<< tuple.sourcePort << endl;
            FlowUnit *funit = getFlowUnit(tuple);
            if(funit->state == UNREGISTERED) {
                EV_DEBUG << "DA_processTcpFromAgent: Initializing flow unit from: "<< destAddr.str() << endl;
                funit->state = REGISTERED;
                funit->isFlowActive = true;
                funit->isAddressCached = false;
                funit->id = agentHeader->getId();
                funit->mobileAgent = destAddr;
                funit->nodeAddress = nodeAddress;
            }
            if (funit->state == REGISTERED) {
//                EV << "DA: flow unit exists. Preparing TCP packet transmission: "<< destAddr << " to " << nodeAddress << endl;
                funit->mobileAgent = destAddr; // just update return address
                InterfaceEntry *ie = getInterface();
                IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
                ipControlInfo->setProtocol(IP_PROT_TCP);
                ipControlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
                ipControlInfo->setDestAddr(funit->nodeAddress);
                ipControlInfo->setInterfaceId(ie->getInterfaceId());
                ipControlInfo->setHopLimit(255);
                tcpseg->setControlInfo(ipControlInfo);
                outgoingTrafficPktNodeStat++;
                emit(outgoingTrafficPktNode, outgoingTrafficPktNodeStat);
                outgoingTrafficSizeNodeStat = tcpseg->getByteLength();
                emit(outgoingTrafficSizeNode, outgoingTrafficSizeNodeStat);
                cGate *outgate = gate("toLowerLayer");
                send(tcpseg, outgate);
//                EV << "Forwarding pkt from MA to Node: Dest=" << ipControlInfo->getDestAddr() << " Src=" << ipControlInfo->getSrcAddr() << " If=" << ipControlInfo->getInterfaceId() << endl;
            } else
                throw cRuntimeError("DA:procMAmsg: TCP packet could not processed. FlowUnit unknown.");
        } else
            throw cRuntimeError("DA:procMAmsg: TCP packet could not be cast.");
    } else
        throw cRuntimeError("DA: Received tcp packet from MA but its id not in the list");
}

void DataAgent::processIcmpFromAgent(IdentificationHeader *agentHeader, IPv6Address destAddr)
{ // all ICMP packets are forwarded.
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
        incomingTrafficPktAgentStat++;
        emit(incomingTrafficPktAgent, incomingTrafficPktAgentStat);
        incomingTrafficSizeAgentStat = agentHeader->getByteLength();
        emit(incomingTrafficSizeAgent, incomingTrafficSizeAgentStat);
        if(agentHeader->getIPaddressesArraySize() < 1)
            throw cRuntimeError("DA: ArraySize = 0. Check message processing unit from MA. At least one IP address of target must be contained.");
        IPv6Address nodeAddress = agentHeader->getIPaddresses(0);
        cPacket *packet = agentHeader->decapsulate(); // decapsulate(remove) agent header
        if (dynamic_cast<ICMPv6Message *>(packet) != nullptr) {
            ICMPv6Message *icmp =  (ICMPv6Message *) packet;
            FlowTuple tuple;
            tuple.protocol = IP_PROT_IPv6_ICMP;
            tuple.destPort = 0;
            tuple.sourcePort = 0;
            tuple.destAddress = nodeAddress;
            tuple.interfaceId = nodeAddress.getInterfaceId();
//            EV << "DA: Creating UDP FlowTuple ADDR:" << tuple.destAddress << " DP:" << tuple.destPort << " SP:"<< tuple.sourcePort << endl;
            FlowUnit *funit = getFlowUnit(tuple);
            if(funit->state == UNREGISTERED) {
                EV << "DA: Init flow unit with first packet from: "<< destAddr.str() << endl;
                funit->state = REGISTERED;
                funit->isFlowActive = true;
                funit->isAddressCached = false;
                funit->id = agentHeader->getId();
                funit->mobileAgent = destAddr;
                funit->nodeAddress = nodeAddress;
                // other fields are not initialized.
            }
            if (funit->state == REGISTERED) {
//                EV << "DA: flow unit exists. Preparing transmission: "<< destAddr.str() << endl;
                funit->mobileAgent = destAddr; // just update return address
                InterfaceEntry *ie = getInterface();
                IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
                ipControlInfo->setProtocol(IP_PROT_IPv6_ICMP);
                ipControlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
                ipControlInfo->setDestAddr(funit->nodeAddress);
                ipControlInfo->setInterfaceId(ie->getInterfaceId());
                ipControlInfo->setHopLimit(255);
                icmp->setControlInfo(ipControlInfo);
                outgoingTrafficPktNodeStat++;
                emit(outgoingTrafficPktNode, outgoingTrafficPktNodeStat);
                outgoingTrafficSizeNodeStat = icmp->getByteLength();
                emit(outgoingTrafficSizeNode, outgoingTrafficSizeNodeStat);
                cGate *outgate = gate("toLowerLayer");
                send(icmp, outgate);
//                EV << "DA: Forwarding icmp msg(MA) to CorresNode: Dest=" << ipControlInfo->getDestAddr() << " Src=" << ipControlInfo->getSrcAddr() << " If=" << ipControlInfo->getInterfaceId() << endl;
            } else
                throw cRuntimeError("DA:procMAmsg: icmp packet could not processed. FlowUnit unknown.");
        } else {
            throw cRuntimeError("DA: icmp function failed because received ICMPv6Message could not be cast.");
        }
    } else
        throw cRuntimeError("DA: Received Ping packet from MA but its id not in the list");
}

void DataAgent::processIncomingIcmpPacket(ICMPv6Message *icmp, IPv6ControlInfo *controlInfo)
{
    FlowTuple tuple;
    tuple.protocol = IP_PROT_IPv6_ICMP;
    tuple.destPort = 0;
    tuple.sourcePort = 0;
    tuple.destAddress = controlInfo->getSourceAddress().toIPv6();
    tuple.interfaceId = controlInfo->getSourceAddress().toIPv6().getInterfaceId();
    FlowUnit *funit = getFlowUnit(tuple);
    if (funit->state == REGISTERED && icmp->getType() == ICMPv6_ECHO_REPLY) {
        incomingTrafficPktNodeStat++;
        emit(incomingTrafficPktNode, incomingTrafficPktNodeStat);
        incomingTrafficSizeNodeStat = icmp->getByteLength();
        emit(incomingTrafficSizeNode, incomingTrafficSizeNodeStat);
        IdentificationHeader *ih = getAgentHeader(3, IP_PROT_IPv6_ICMP, getSeqNo(funit->id), 0, tuple.interfaceId);
        ih->setIsIdInitialized(true);
        ih->setIsIdAcked(true);
        ih->setIsSeqValid(true);
        ih->setIsWithReturnAddr(true);
        ih->setIsReturnAddrCached(funit->isAddressCached);
        ih->encapsulate(icmp);
        outgoingTrafficPktAgentStat++;
        emit(outgoingTrafficPktAgent, outgoingTrafficPktAgentStat);
        outgoingTrafficSizeAgentStat = ih->getByteLength();
        emit(outgoingTrafficSizeAgent, outgoingTrafficSizeAgentStat);
        sendToLowerLayer(ih,funit->mobileAgent);
//        EV << "DA: Forwarding imcp echo packet to: "<< funit->mobileAgent << " from node:"<< funit->nodeAddress <<" via Id2: " << funit->id  << endl;
    } else {
//        EV << "DA: Forwarding message to ICMP module. Type=" << icmp->getType() << endl;
        icmp->setControlInfo(controlInfo);
        cGate *outgate = gate("icmpOut");
        send(icmp, outgate);
    }
}

void DataAgent::processUdpFromNode(cMessage *msg)
{
    IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
    FlowTuple tuple;
    if(dynamic_cast<UDPPacket *>(msg) != nullptr) {
        UDPPacket *udpPacket =  (UDPPacket *) msg;
        incomingTrafficPktNodeStat++;
        emit(incomingTrafficPktNode, incomingTrafficPktNodeStat);
        incomingTrafficSizeNodeStat = udpPacket->getByteLength();
        emit(incomingTrafficSizeNode, incomingTrafficSizeNodeStat);
        tuple.protocol = IP_PROT_UDP;
        if(enableNodeRequesting) {
            tuple.destPort = 0;
            tuple.sourcePort = 0;
        } else {
            tuple.destPort = udpPacket->getSourcePort();
            tuple.sourcePort = udpPacket->getDestinationPort();
        }
        tuple.destAddress = controlInfo->getSourceAddress().toIPv6();
        tuple.interfaceId = controlInfo->getSourceAddress().toIPv6().getInterfaceId();
        FlowUnit *funit = getFlowUnit(tuple);
        if((funit->state == REGISTERED && funit->isFlowActive) || enableNodeRequesting) {
            IdentificationHeader *ih = getAgentHeader(3, IP_PROT_UDP, getSeqNo(funit->id), 0, tuple.interfaceId);
            ih->setIsIdInitialized(true);
            ih->setIsIdAcked(true);
            ih->setIsSeqValid(true);
            ih->setIsWithReturnAddr(true);
            ih->setIsReturnAddrCached(funit->isAddressCached);
//            EV << "DA: Forwarding regular UDP packet: "<< funit->mobileAgent << " agent: "<< funit->dataAgent << " node:"<< funit->nodeAddress <<" Id2: " << funit->id  <<endl;
            ih->encapsulate(udpPacket);
            outgoingTrafficPktAgentStat++;
            emit(outgoingTrafficPktAgent, outgoingTrafficPktAgentStat);
            outgoingTrafficSizeAgentStat = ih->getByteLength();
            emit(outgoingTrafficSizeAgent, outgoingTrafficSizeAgentStat);
            if(isIdInitialized(funit->id)) {
                if(isAddressInserted(funit->id, getSeqNo(funit->id), funit->mobileAgent)) {
                    sendToLowerLayer(ih,funit->mobileAgent);
                } else {
                    sendToLowerLayer(ih,getValidAddress(funit->id));
                }
            } else {
                EV_WARN << "DA_processTcpFromNode: Incoming message is dropped. No matching Mobile Agent is found." << endl;
            }
        } else
            throw cRuntimeError("DA_processUdpFromNode: No flow tuple exists. Mobile Agent needs to initiate the connection.");
    } else
        throw cRuntimeError("DA_processUdpFromNode: Incoming packet could not cast to TCPPacket.");
    delete controlInfo;
}


void DataAgent::processTcpFromNode(cMessage *msg)
{
    IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
    FlowTuple tuple;
    if (dynamic_cast<tcp::TCPSegment *>(msg) != nullptr) {
        tcp::TCPSegment *tcpseg =  (tcp::TCPSegment *) msg;
        incomingTrafficPktNodeStat++;
        emit(incomingTrafficPktNode, incomingTrafficPktNodeStat);
        incomingTrafficSizeNodeStat = tcpseg->getByteLength();
        emit(incomingTrafficSizeNode, incomingTrafficSizeNodeStat);
        tuple.protocol = IP_PROT_TCP;
        if(enableNodeRequesting) {
            tuple.destPort = 0;
            tuple.sourcePort = 0;
        } else {
            tuple.destPort = tcpseg->getSourcePort();
            tuple.sourcePort = tcpseg->getDestinationPort();
        }
        tuple.destAddress = controlInfo->getSourceAddress().toIPv6();
        tuple.interfaceId = controlInfo->getSourceAddress().toIPv6().getInterfaceId();
//        EV << "DA: Received regular message from Node, ADDR:" << tuple.destAddress << " DP:" << tuple.destPort << " SP:"<< tuple.sourcePort << endl;
        FlowUnit *funit = getFlowUnit(tuple);
        if((funit->state == REGISTERED && funit->isFlowActive) || enableNodeRequesting) {
            IdentificationHeader *ih = getAgentHeader(3, IP_PROT_TCP, getSeqNo(funit->id), 0, tuple.interfaceId);
            ih->setIsIdInitialized(true);
            ih->setIsIdAcked(true);
            ih->setIsSeqValid(true);
            ih->setIsWithReturnAddr(true);
            ih->setIsReturnAddrCached(funit->isAddressCached);
            ih->encapsulate(tcpseg);
            outgoingTrafficPktAgentStat++;
            emit(outgoingTrafficPktAgent, outgoingTrafficPktAgentStat);
            outgoingTrafficSizeAgentStat = ih->getByteLength();
            emit(outgoingTrafficSizeAgent, outgoingTrafficSizeAgentStat);
//            EV << "DA: Forwarding regular TCP packet to: "<< funit->mobileAgent << " from node:"<< funit->nodeAddress <<" via Id2: " << funit->id  << " Size" << tcpseg->getByteLength() << endl;
            if(isIdInitialized(funit->id)) {
                if(isAddressInserted(funit->id, getSeqNo(funit->id), funit->mobileAgent)) {
                    sendToLowerLayer(ih,funit->mobileAgent);
                } else {
                    sendToLowerLayer(ih,getValidAddress(funit->id));
                }
            } else {
                EV_WARN << "DA_processTcpFromNode: Incoming message is dropped. No matching Mobile Agent is found." << endl;
            }
        } else
            throw cRuntimeError("DA_processTcpFromNode: No flow tuple exists. Mobile Agent needs to initiate the connection.");
    } else
        throw cRuntimeError("DA_processTcpFromNode: Incoming packet could not cast to TCPPacket.");
    delete controlInfo;
}

void DataAgent::processOutgoingIcmpPacket(cMessage *msg)
{
    cGate *outgate = gate("icmpIpOut");
    send(msg, outgate);
}

// returns any valid interface of node
InterfaceEntry *DataAgent::getInterface() { // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->ipv6Data()->getPreferredAddress().isGlobal()) {
            return ie;
        }
    }
    return ie;
}

// checks if address is in table inserted, if so, it returns given address.
IPv6Address DataAgent::getValidAddress(uint64 id)
{
    AddressDiff ad = getAddressList(id, getSeqNo(id));
    if(ad.insertedList.size() > 0) {
        for(int idx=0; idx < ad.insertedList.size(); idx++)
            return ad.insertedList.at(idx).address;
    } else
        throw cRuntimeError("DA_getValidAddress: Address list of Mobile Agent is empty.");

}


void DataAgent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, simtime_t delayTime) {
    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
    ctrlInfo->setProtocol(IP_PROT_IPv6EXT_ID);
    ctrlInfo->setDestAddr(destAddr);
    ctrlInfo->setHopLimit(255); // FIX this
    InterfaceEntry *ie = getInterface();
    if(ie) {
        ctrlInfo->setInterfaceId(ie->getInterfaceId());
        ctrlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
    }
    msg->setControlInfo(ctrlInfo);
    cGate *outgate = gate("toLowerLayer");
    if (delayTime > 0) {
        EV_DEBUG << "DA_sendToLowerLayer: Sending delayed." << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

} //namespace