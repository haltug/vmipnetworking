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

void DataAgent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
        IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
        ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
        IPSocket ipSocket2(gate("udpOut")); // TODO test if one can register from first socket
        ipSocket2.registerProtocol(IP_PROT_UDP);
        IPSocket ipSocket3(gate("tcpOut"));
        ipSocket3.registerProtocol(IP_PROT_TCP);
    }
    if (stage == INITSTAGE_APPLICATION_LAYER) {
    }
}

void DataAgent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {
        if(msg->getKind() == MSG_SEQ_UPDATE_NOTIFY) {
            sendSequenceUpdateNotification(msg);
        }
//        else if(msg->getKind() == MSG_SEQ_UPDATE_NOTIFY) {
//           sendSequenceUpdateNotification(msg);
//        }
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
    } else if(msg->arrivedOn("udpIn")) {
        proccessRegularNodeMessage(msg, IP_PROT_UDP);
    } else if(msg->arrivedOn("tcpIn")) {
        proccessRegularNodeMessage(msg, IP_PROT_TCP);
    }
    else
        throw cRuntimeError("A:handleMsg: cMessage Type not known. What did you send?");
}

void DataAgent::createSequenceUpdateNotificaiton(uint64 mobileId, uint seq)
{
    if(!caAddress.isGlobal())
        throw cRuntimeError("DA:SeqUpdNotify: CA_Address is not set. Cannot send update message without address");
    EV << "DA: creating seq No update for CA" << endl;
    TimerKey key(caAddress,-1,TIMERKEY_SEQ_UPDATE_NOT, mobileId, am.getCurrentSequenceNumber(mobileId));
    UpdateNotifierTimer *unt = (UpdateNotifierTimer *) getExpiryTimer(key,TIMERTYPE_SEQ_UPDATE_NOT); // this timer must explicitly deleted
    if(!unt->active) { // a timer has not been created earlier
        cMessage *msg = new cMessage("sendingCAseqUpd", MSG_SEQ_UPDATE_NOTIFY);
        unt->dest = caAddress;
        unt->timer = msg;
        unt->ackTimeout = TIMEOUT_SEQ_UPDATE;
        unt->nextScheduledTime = simTime();
        unt->id = mobileId;
        unt->ack = am.getLastAcknowledgemnt(mobileId);
        unt->seq = seq;
        unt->liftime = simTime();
        unt->active = true;
        msg->setContextPointer(unt);
        scheduleAt(unt->nextScheduledTime,msg);
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
    EV << "CA: Send agent update" << endl;
    UpdateNotifierTimer *unt = (UpdateNotifierTimer *) msg->getContextPointer();
    const IPv6Address &dest =  unt->dest;
    unt->nextScheduledTime = simTime() + unt->ackTimeout;
    unt->ackTimeout = (unt->ackTimeout)*1.5;
    MobileAgentHeader *mah = new MobileAgentHeader("ca_seq_upd");
    mah->setId(unt->id);
    mah->setIdInit(true);
    mah->setIdAck(false);
    mah->setSeqValid(true);
    mah->setAckValid(false);
    mah->setAddValid(true);
    mah->setRemValid(false);
    mah->setNextHeader(IP_PROT_NONE);
    mah->setIpSequenceNumber(unt->seq);
    mah->setIpAcknowledgementNumber(0);
    AddressManagement::AddressChange ac = am.getAddressEntriesOfSequenceNumber(unt->id,unt->seq);
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
    sendToLowerLayer(mah, dest);
    scheduleAt(unt->nextScheduledTime, msg);
}

void DataAgent::sendAgentInitResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId, uint seq)
{
    DataAgentHeader *dah = new DataAgentHeader("da_seq_init_ack");
    dah->setIdInit(true);
    dah->setIdAck(true);
    dah->setSeqValid(true);
    dah->setIdValid(true);
    dah->setId(mobileId);
    dah->setIpSequenceNumber(seq);
    dah->setByteLength(SIZE_AGENT_HEADER);
    sendToLowerLayer(dah,destAddr);
}

void DataAgent::sendAgentUpdateResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId, uint seq)
{
    sendAgentInitResponse(destAddr, sourceAddr, mobileId, seq);
}

void DataAgent::processMobileAgentMessage(MobileAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    IPv6Address destAddr = controlInfo->getSrcAddr(); // address to be responsed
    IPv6Address sourceAddr = controlInfo->getDestAddr();
    if(agentHeader->getIdInit() && !agentHeader->getIdAck() && agentHeader->getSeqValid() && !agentHeader->getAckValid()  && agentHeader->getAddValid() && !agentHeader->getRemValid() && (agentHeader->getNextHeader() == IP_PROT_NONE)) // check for other bits
    { // INIT PROCESSING: header flag bits 101010 + NextHeader=NONE
        if(caAddress.isUnspecified()) { caAddress = controlInfo->getSrcAddr(); }
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
                sendAgentInitResponse(destAddr, sourceAddr, agentHeader->getId(), am.getCurrentSequenceNumber(agentHeader->getId()));
        } else
        { // IF NOT AGENT INIT then JUST SEQ UPDATE
            if(caAddress.isUnspecified()) { throw cRuntimeError("DA:procMA: CA address must be known at this stage. Stage= seq update."); }
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                AddressManagement::IPv6AddressList ipList;
                if(agentHeader->getIpAddingField() > 0) { // check size of field
                EV << "DA: Received message from CA. Id exists. Updating seq map and sending response message." << endl;
                    if(agentHeader->getIpAddingField() != agentHeader->getAddedAddressesArraySize())
                        throw cRuntimeError("DA:Hdr: field of array and field not same. Check header.");
                    for(int i=0; i<agentHeader->getIpAddingField(); i++){ // copy array in vector list
                        ipList.push_back(agentHeader->getAddedAddresses(i)); // inserting elements of array into vector list due to function
                    }
                    // first inserting new
                    am.insertSequenceTableToAddressMap(agentHeader->getId(), ipList, agentHeader->getIpSequenceNumber());
                    cancelAndDeleteExpiryTimer(caAddress, -1, TIMERKEY_SEQ_UPDATE_NOT, agentHeader->getId(), agentHeader->getIpSequenceNumber(), am.getLastAcknowledgemnt(agentHeader->getId()));
                    am.setLastAcknowledgemnt(agentHeader->getId(),agentHeader->getIpSequenceNumber()); //
                    sendAgentUpdateResponse(caAddress, sourceAddr, agentHeader->getId(),agentHeader->getIpSequenceNumber());
                }
            } else
                throw cRuntimeError("DA:procMA: Id is not inserted in map. Register id before seq update.");
        }
    }
    else if(agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid() && agentHeader->getAckValid())
    { // regular message from mobile agent
//        EV << "DA: Received message from MA." << endl;
        if(sourceAddr != caAddress) {
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                if(agentHeader->getIpAcknowledgementNumber() > am.getLastAcknowledgemnt(agentHeader->getId())) { // check for error
                    throw cRuntimeError("DA: ack number of MA header is lower than ack of DA. Ack of DA must be same or higher as MA's ack.");
                } else
                {   // is both values are equal, this means that DA is up to date with the IP addresses of MA
                    if(agentHeader->getIpSequenceNumber() == am.getCurrentSequenceNumber(agentHeader->getId())) {
                        EV << "DA: Seq No of msg is same to addresstable." << endl;
                        // if ack number are unequal, DA has been updated but not acked by CA. So we update/notify CA
                        if(agentHeader->getIpAcknowledgementNumber() != am.getLastAcknowledgemnt(agentHeader->getId())) {
                            EV << "DA: Received message from MA contains unconfirmed update data. Sending notification to CA." << endl;
                           createSequenceUpdateNotificaiton(agentHeader->getId(), am.getCurrentSequenceNumber(agentHeader->getId()));
                        }
                    } // if both are unequal this means that MA has a new address. updating DAs table and informing CA
                    else if(agentHeader->getIpSequenceNumber() > am.getCurrentSequenceNumber(agentHeader->getId())) {
                        EV << "DA: Received message from MA has new address. Updating address table." << endl;
                        if(agentHeader->getAddValid() || agentHeader->getRemValid()) { // indicate a sequence update
                            if(agentHeader->getIpAddingField() > 0) { // check size of field
                                if(agentHeader->getIpAddingField() != agentHeader->getAddedAddressesArraySize()) throw cRuntimeError("DA:Hdr: field of array and field not same. Check header.");
                                for(int i=0; i<agentHeader->getIpAddingField(); i++){
                                    am.addIPv6AddressToAddressMap(agentHeader->getId(), agentHeader->getAddedAddresses(i));
                                }
                            }
                            if(agentHeader->getIpRemovingField() > 0) {
                                if(agentHeader->getIpRemovingField() != agentHeader->getRemovedAddressesArraySize()) throw cRuntimeError("DA:Hdr: field of array and field not same. Check header.");
                                for(int i=0; i<agentHeader->getIpRemovingField(); i++) {
                                    am.removeIPv6AddressFromAddressMap(agentHeader->getId(), agentHeader->getRemovedAddresses(i));
                                }
                            }
                            EV << "DA: Extracted address update. update to seq: " << agentHeader->getIpSequenceNumber() << " from ack: " << am.getLastAcknowledgemnt(agentHeader->getId()) << endl;
                            createSequenceUpdateNotificaiton(agentHeader->getId(), am.getCurrentSequenceNumber(agentHeader->getId()));
                        } else { throw cRuntimeError("DA: Determining seq difference but cannot valid header field. AddValid/RemValid not set"); }
                    } else { throw cRuntimeError("DA: seq number of DA is higher than seq of MA. How can this be correct? Only MA is incrementing seq."); }
                }
                if(agentHeader->getNextHeader() == IP_PROT_UDP) {
                    uint64 mobileId = agentHeader->getId();
                    const IPv6Address mobileAddress = controlInfo->getSourceAddress().toIPv6();
                    const IPv6Address nodeAddress = agentHeader->getNodeAddress();
//                    EV << "DA: Received UDP packet from MA. MA: "<< mobileAddress << " Node: "<< nodeAddress << endl;
                    if(!(agentHeader->getForwardAddrInit() && nodeAddress.isGlobal())) throw cRuntimeError("DA: procUDP: nodeAddress not in header.");
                    bool activateCache = agentHeader->getCacheAddrInit();
                    cPacket *packet = agentHeader->decapsulate(); // decapsulate(remove) agent header
                    if (dynamic_cast<UDPPacket *>(packet) != nullptr) {
                        UDPPacket *udpPacket =  (UDPPacket *) packet;
                        FlowTuple tuple;
                        tuple.protocol = IP_PROT_UDP;
                        tuple.destPort = udpPacket->getDestinationPort();
                        tuple.sourcePort = udpPacket->getSourcePort();
                        tuple.destAddress = nodeAddress;
                        EV << "DA: Creating UDP FlowTuple ADDR:" << tuple.destAddress << " DP:" << tuple.destPort << " SP:"<< tuple.sourcePort << endl;
                        FlowUnit *funit = getFlowUnit(tuple);
                        if(funit->state == UNREGISTERED) {
                            EV << "DA: Init flow unit with first packet: "<< mobileAddress << endl;
                            funit->state = REGISTERED;
                            funit->active = true;
                            funit->id = mobileId;
                            funit->cacheAddress = activateCache;
                            funit->mobileAgent = mobileAddress;
                            funit->nodeAddress = nodeAddress;
                            // other fields are not initialized.
                        }
                        if (funit->state == REGISTERED) {
                            EV << "DA: flow unit exists. Preparing transmission: "<< mobileAddress << endl;
                            funit->mobileAgent = mobileAddress; // just update return address
                            InterfaceEntry *ie = getInterface();
                            IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
                            ipControlInfo->setProtocol(IP_PROT_UDP);
                            ipControlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
                            ipControlInfo->setDestAddr(funit->nodeAddress);
                            ipControlInfo->setInterfaceId(ie->getInterfaceId());
                            ipControlInfo->setHopLimit(255);
                            udpPacket->setControlInfo(ipControlInfo);
                            cGate *outgate = gate("toLowerLayer");
                            send(udpPacket, outgate);
                            EV << "Forwarding pkt from MA to Node: Dest=" << ipControlInfo->getDestAddr() << " Src=" << ipControlInfo->getSrcAddr() << " If=" << ipControlInfo->getInterfaceId() << endl;
                        } else { throw cRuntimeError("DA:procMAmsg: UDP packet could not processed. FlowUnit unknown."); }
                    } else { throw cRuntimeError("DA:procMAmsg: UDP packet could not be cast."); }
                } else if(agentHeader->getNextHeader() == IP_PROT_TCP) {
                    uint64 mobileId = agentHeader->getId();
                    const IPv6Address mobileAddress = controlInfo->getSourceAddress().toIPv6();
                    const IPv6Address nodeAddress = agentHeader->getNodeAddress();
                    EV << "DA: Received TCP packet from MA. MA: "<< mobileAddress << " Node: "<< nodeAddress << endl;
                    if(!(agentHeader->getForwardAddrInit() && nodeAddress.isGlobal())) throw cRuntimeError("DA: procTCP: nodeAddress not in header.");
                    bool activateCache = agentHeader->getCacheAddrInit();
                    cPacket *packet = agentHeader->decapsulate(); // decapsulate(remove) agent header
                    if (dynamic_cast<tcp::TCPSegment *>(packet) != nullptr) {
                        tcp::TCPSegment *tcpseg =  (tcp::TCPSegment *) packet;
                        FlowTuple tuple;
                        tuple.protocol = IP_PROT_TCP;
                        tuple.destPort = tcpseg->getDestinationPort();
                        tuple.sourcePort = tcpseg->getSourcePort();
                        tuple.destAddress = nodeAddress;
                        EV << "DA: Creating TCP FlowTuple ADDR:" << tuple.destAddress << " DP:" << tuple.destPort << " SP:"<< tuple.sourcePort << endl;
                        FlowUnit *funit = getFlowUnit(tuple);
                        if(funit->state == UNREGISTERED) {
                            EV << "DA: Init flow unit with first packet: "<< mobileAddress << endl;
                            funit->state = REGISTERED;
                            funit->active = true;
                            funit->id = mobileId;
                            funit->cacheAddress = activateCache;
                            funit->mobileAgent = mobileAddress;
                            funit->nodeAddress = nodeAddress;
                            // other fields are not initialized.
                        }
                        if (funit->state == REGISTERED) {
                            EV << "DA: flow unit exists. Preparing TCP packet transmission: "<< mobileAddress << endl;
                            funit->mobileAgent = mobileAddress; // just update return address
                            InterfaceEntry *ie = getInterface();
                            IPv6ControlInfo *ipControlInfo = new IPv6ControlInfo();
                            ipControlInfo->setProtocol(IP_PROT_TCP);
                            ipControlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
                            ipControlInfo->setDestAddr(funit->nodeAddress);
                            ipControlInfo->setInterfaceId(ie->getInterfaceId());
                            ipControlInfo->setHopLimit(255);
                            tcpseg->setControlInfo(ipControlInfo);
                            cGate *outgate = gate("toLowerLayer");
                            send(tcpseg, outgate);
                            EV << "Forwarding pkt from MA to Node: Dest=" << ipControlInfo->getDestAddr() << " Src=" << ipControlInfo->getSrcAddr() << " If=" << ipControlInfo->getInterfaceId() << endl;
                        } else { throw cRuntimeError("DA:procMAmsg: TCP packet could not processed. FlowUnit unknown."); }
                    } else { throw cRuntimeError("DA:procMAmsg: TCP packet could not be cast."); }
                } else { throw cRuntimeError("DA:procMAmsg: header declaration not known."); }
            } else { throw cRuntimeError("DA:procMAmsg: DA is not initialized with given ID."); }
        } else { throw cRuntimeError("DA: Received message is from CA but has wrong header parameter."); }
    } else { throw cRuntimeError("DA: header field not correct. Message cannot be assigned. Check who sent wrong message."); }
delete agentHeader;
delete controlInfo;
}

void DataAgent::processControlAgentMessage(ControlAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    throw cRuntimeError("DA: Received CA header. For what purpose?");
}

void DataAgent::proccessRegularNodeMessage(cMessage *msg, short protocol) {
    IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
    FlowTuple tuple;
    if(protocol == IP_PROT_UDP) {
        if (dynamic_cast<UDPPacket *>(msg) != nullptr) {
            UDPPacket *udpPacket =  (UDPPacket *) msg;
            tuple.protocol = IP_PROT_UDP;
            tuple.destPort = udpPacket->getSourcePort();
            tuple.sourcePort = udpPacket->getDestinationPort();
            tuple.destAddress = controlInfo->getSourceAddress().toIPv6();
//            EV << "DA: Received regular message from Node, ADDR:" << tuple.destAddress << " DP:" << tuple.destPort << " SP:"<< tuple.sourcePort << endl;
            FlowUnit *funit = getFlowUnit(tuple);
            if(funit->state == REGISTERED && funit->active) {
                DataAgentHeader *dah = new DataAgentHeader("ma_udp");
                dah->setIdInit(true);
                dah->setIdAck(true);
                dah->setSeqValid(true);
                dah->setIdValid(true);
                dah->setNextHeader(IP_PROT_UDP);
                dah->setIpSequenceNumber(am.getCurrentSequenceNumber(funit->id));
//                bool ipAddressExists = false;
                AddressManagement::AddressChange ac = am.getAddressEntriesOfSequenceNumber(funit->id,am.getCurrentSequenceNumber(funit->id));
//                if(ac.addedAddresses > 0) { // check if address is yet available
//                    for(int i=0; i<ac.addedAddresses; i++) {
//                        EV << "DA: checking availability of ip addr." << endl;
//                        if(ac.getUnacknowledgedAddedIPv6AddressList.at(i) == funit->mobileAgent)
//                            ipAddressExists = true;
//                    }
//                    if(!ipAddressExists) { // if not available use another ip address
//                        EV << "DA: replacing ip addr:"<< am.to_string() << endl;
//                        for(int i=0; i<ac.addedAddresses; i++) {
//                            funit->mobileAgent = ac.getUnacknowledgedAddedIPv6AddressList.at(i);
//                            break;
//                        }
//                    }
//                }
                dah->encapsulate(udpPacket);
                EV << "DA: Forwarding regular UDP packet: "<< funit->mobileAgent << " agent: "<< funit->dataAgent << " node:"<< funit->nodeAddress <<" Id2: " << funit->id  <<endl;
                sendToLowerLayer(dah,funit->mobileAgent);
            } else {  throw cRuntimeError("DA:forward: could not find tuple of incoming udp packet."); }
        } else { throw cRuntimeError("DA:forward: could not cast to UDPPacket."); }

    } else if (protocol == IP_PROT_TCP) {
        if (dynamic_cast<tcp::TCPSegment *>(msg) != nullptr) {
            tcp::TCPSegment *tcpseg =  (tcp::TCPSegment *) msg;
            tuple.protocol = IP_PROT_TCP;
            tuple.destPort = tcpseg->getSourcePort();
            tuple.sourcePort = tcpseg->getDestinationPort();
            tuple.destAddress = controlInfo->getSourceAddress().toIPv6();
            EV << "DA: Received regular message from Node, ADDR:" << tuple.destAddress << " DP:" << tuple.destPort << " SP:"<< tuple.sourcePort << endl;
            FlowUnit *funit = getFlowUnit(tuple);
            if(funit->state == REGISTERED && funit->active) {
                DataAgentHeader *dah = new DataAgentHeader("ma_tcp");
                dah->setIdInit(true);
                dah->setIdAck(true);
                dah->setSeqValid(true);
                dah->setIdValid(true);
                dah->setNextHeader(IP_PROT_TCP);
                dah->setIpSequenceNumber(am.getCurrentSequenceNumber(funit->id));
//                bool ipAddressExists = false;
                AddressManagement::AddressChange ac = am.getAddressEntriesOfSequenceNumber(funit->id,am.getCurrentSequenceNumber(funit->id));
//                if(ac.addedAddresses > 0) { // check if address is yet available
//                    for(int i=0; i<ac.addedAddresses; i++) {
//                        EV << "DA: checking availability of ip addr." << endl;
//                        if(ac.getUnacknowledgedAddedIPv6AddressList.at(i) == funit->mobileAgent)
//                            ipAddressExists = true;
//                    }
//                    if(!ipAddressExists) { // if not available use another ip address
//                        EV << "DA: replacing ip addr:"<< am.to_string() << endl;
//                        for(int i=0; i<ac.addedAddresses; i++) {
//                            funit->mobileAgent = ac.getUnacknowledgedAddedIPv6AddressList.at(i);
//                            break;
//                        }
//                    }
//                }
                dah->encapsulate(tcpseg);
                EV << "DA: Forwarding regular UDP packet: "<< funit->mobileAgent << " agent: "<< funit->dataAgent << " node:"<< funit->nodeAddress <<" Id2: " << funit->id  <<endl;
                sendToLowerLayer(dah,funit->mobileAgent);
            } else {  throw cRuntimeError("DA:forward: could not find tuple of incoming tcp packet."); }
        } else { throw cRuntimeError("DA:forward: could not cast to UDPPacket."); }
    }
    delete controlInfo;
}

InterfaceEntry *DataAgent::getInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol) { // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal()) { return ie; }
    }
    return ie;
}

void DataAgent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, simtime_t delayTime) {
//    EV << "A: Creating IPv6ControlInfo to lower layer" << endl;
    IPv6ControlInfo *ctrlInfo = new IPv6ControlInfo();
    ctrlInfo->setProtocol(IP_PROT_IPv6EXT_ID); // todo must be adjusted
    ctrlInfo->setDestAddr(destAddr);
    ctrlInfo->setHopLimit(255);
    InterfaceEntry *ie = getInterface(destAddr);
    if(ie) {
        ctrlInfo->setInterfaceId(ie->getInterfaceId());
        ctrlInfo->setSrcAddr(ie->ipv6Data()->getPreferredAddress());
    }
    msg->setControlInfo(ctrlInfo);
    cGate *outgate = gate("toLowerLayer");
    EV << "DA2IP: Dest=" << ctrlInfo->getDestAddr() << " Src=" << ctrlInfo->getSrcAddr() << " If=" << ctrlInfo->getInterfaceId() << endl;
    if (delayTime > 0) {
        EV << "delayed sending" << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

} //namespace
