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

simsignal_t ControlAgent::numDataAgents = registerSignal("numDataAgents");
simsignal_t ControlAgent::numMobileAgents = registerSignal("numMobileAgents");
simsignal_t ControlAgent::numFlowRequests = registerSignal("numFlowRequests");
simsignal_t ControlAgent::numSequenceUpdate = registerSignal("numSequenceUpdate");
simsignal_t ControlAgent::numSequenceResponse = registerSignal("numSequenceResponse");
simsignal_t ControlAgent::txTraffic = registerSignal("txTraffic");
simsignal_t ControlAgent::rxTraffic = registerSignal("rxTraffic");

ControlAgent::~ControlAgent() {
    auto it = expiredTimerList.begin();
    while(it != expiredTimerList.end()) {
        TimerKey key = it->first;
        it++;
        cancelAndDeleteExpiryTimer(key.dest,key.interfaceID,key.type);
    }
}

void ControlAgent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        WATCH(numDataAgentsStat);
        WATCH(numMobileAgentsStat);
        WATCH(numFlowRequestsStat);
        WATCH(numSequenceUpdateStat);
        WATCH(numSequenceResponseStat);
        WATCH(txTrafficStat);
        WATCH(rxTrafficStat);
        WATCH(*this);
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
            IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
            ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
            srand(time(0));
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
        else if(msg->getKind() == MSG_SEQ_UPDATE_ACK) {
            sendSequenceUpdateAck(msg);
        }
        else
            throw cRuntimeError("handleMessage: Unknown timer expired. Which timer msg is unknown?");
    }
    else if(msg->arrivedOn("fromLowerLayer")) {
        if (dynamic_cast<IdentificationHeader *> (msg)) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
            processAgentMessage((IdentificationHeader *) msg, controlInfo);
        } else {
//            throw cRuntimeError("A:handleMsg: Extension Hdr Type not known. What did you send?");
            delete msg;
        }
    }
    else
        throw cRuntimeError("A:handleMsg: cMessage Type not known. What did you send?");
}

void ControlAgent::createAgentInit(uint64 mobileId)
{
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
            EV_INFO << "CA_createAgentInit: Sending initialization message to Data Agent " << daAddr.toIPv6().str() << " for registering Mobile Agent(" << mobileId << ")" << endl;
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
    MobileInitTimer *mit = (MobileInitTimer *) msg->getContextPointer();
    InterfaceEntry *ie = getInterface();
    if(!ie) throw cRuntimeError("CA: no interface provided.");
    const IPv6Address &dest = mit->dest;
    mit->nextScheduledTime = simTime() + mit->ackTimeout;
    mit->ackTimeout = (mit->ackTimeout)*2;
    for(auto &item : getAddressMap()) {
        EV_DEBUG << "CA: Send initialization message to any DataAgent." << endl;
        IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, getSeqNo(item.first), 0, item.first);
        ih->setIsIdInitialized(true);
        ih->setIsSeqValid(true);
        ih->setIsIpModified(true);
        AddressDiff ad = getAddressList(mit->id,getSeqNo(mit->id));
        ih->setIPaddressesArraySize(ad.insertedList.size());
        ih->setIpAddingField(ad.insertedList.size());
        if(ad.insertedList.size() > 0) {
            for(int i=0; i < ad.insertedList.size(); i++)
                ih->setIPaddresses(i,ad.insertedList.at(i).address);
        }
        ih->setIpRemovingField(0);
        ih->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*ad.insertedList.size()));
        txTrafficStat = ih->getByteLength();
        emit(txTraffic, txTrafficStat);
        numDataAgentsStat++;
        emit(numDataAgents,numDataAgentsStat);
        sendToLowerLayer(ih, dest);
    }
    scheduleAt(mit->nextScheduledTime, msg);
}

void ControlAgent::createAgentUpdate(uint64 mobileId, uint seq)
{
//    EV << "CA: Create Agent update message to any DataAgent" << endl;
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
        EV_DEBUG << "CA_createAgentUpdate: Sending sequence update(" << seq << ") of Mobile Agent(" << mobileId<< ") to Data Agent " << nodeAddr.str() << endl;
    }
}

void ControlAgent::sendAgentUpdate(cMessage *msg)
{
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
    const IPv6Address &dest =  sut->dest;
    sut->nextScheduledTime = simTime() + sut->ackTimeout;
    sut->ackTimeout = (sut->ackTimeout)*1.5;
    IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, getSeqNo(sut->id), getSeqNo(sut->id), sut->id);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    ih->setIsIpModified(true);
    AddressDiff ad = getAddressList(sut->id,sut->seq);
    ih->setIPaddressesArraySize(ad.insertedList.size());
    ih->setIpAddingField(ad.insertedList.size());
    if(ad.insertedList.size() > 0) {
        for(int i=0; i < ad.insertedList.size(); i++)
            ih->setIPaddresses(i,ad.insertedList.at(i).address);
    }
    ih->setIpRemovingField(0);
    ih->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*ad.insertedList.size()));
    txTrafficStat = ih->getByteLength();
    emit(txTraffic, txTrafficStat);
    numSequenceUpdateStat++;
    emit(numSequenceUpdate,numSequenceUpdateStat);
    sendToLowerLayer(ih, dest); // TODO select interface
    scheduleAt(sut->nextScheduledTime, msg);
}

void ControlAgent::sendSessionInitResponse(IPv6Address destAddr)
{
    IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, 0, 0, agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    txTrafficStat = ih->getByteLength();
    emit(txTraffic, txTrafficStat);
    numMobileAgentsStat++;
    emit(numMobileAgents,numMobileAgentsStat);
    sendToLowerLayer(ih,destAddr);
}

void ControlAgent::sendSequenceInitResponse(IPv6Address destAddr, uint64 mobileId, uint seq)
{
    IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, seq, 0, agentId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    txTrafficStat = ih->getByteLength();
    emit(txTraffic, txTrafficStat);
    sendToLowerLayer(ih,destAddr);
}

void ControlAgent::createSequenceUpdateAck(uint64 mobileId) { // does not support interface check
    cMessage *msg = new cMessage("creatingSeqUpdAck", MSG_SEQ_UPDATE_ACK);
    TimerKey key(IPv6Address::UNSPECIFIED_ADDRESS,-1,TIMERKEY_SEQ_UPDATE_ACK);
    SequenceUpdateAckTimer *suat = (SequenceUpdateAckTimer *) getExpiryTimer(key, TIMERTYPE_SEQ_UPDATE_ACK);
    suat->timer = msg;
    suat->id = mobileId;
    suat->ackTimeout = TIMEDELAY_SEQ_UPDATE_ACK;
    suat->nextScheduledTime = simTime();
    msg->setContextPointer(suat);
    scheduleAt(suat->nextScheduledTime, msg);
}

// sending over all registered links update messages to MA
void ControlAgent::sendSequenceUpdateAck(cMessage *msg)
{
    SequenceUpdateAckTimer *suat = (SequenceUpdateAckTimer *) msg->getContextPointer();
    uint64 mobileId = suat->id;
    suat->nextScheduledTime = simTime() + suat->ackTimeout;
    suat->ackTimeout = (suat->ackTimeout)*2;
    AddressDiff ad = getAddressList(mobileId);
    AddressList list = ad.insertedList;
    for (AddressTuple tuple : list ) {
        EV_DEBUG << "CA_sendSequenceUpdateAck: Sending sequence update acknowledgment to Mobile Agent(" << mobileId << ")." << endl;
        IPv6Address destAddr = tuple.address; // address to be responsed
        IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, getSeqNo(mobileId), 0, agentId);
        ih->setIsIdInitialized(true);
        ih->setIsIdAcked(true);
        ih->setIsSeqValid(true);
        txTrafficStat = ih->getByteLength();
        emit(txTraffic, txTrafficStat);
        numSequenceResponseStat++;
        emit(numSequenceResponse,numSequenceResponseStat);
        sendToLowerLayer(ih,destAddr);
    }
    scheduleAt(suat->nextScheduledTime, msg);
}

// this functions sends ack's to dataAgents
void ControlAgent::sendSequenceUpdateResponse(IPv6Address destAddr, uint64 mobileId, uint seq)
{
    EV_DEBUG << "CA_sendSequenceUpdateResponse: Sending sequence update acknowledgment(" << seq << ") to Mobile Agent(" << mobileId<< ")." << endl;
    IdentificationHeader *ih = getAgentHeader(2, IP_PROT_NONE, seq, 0, mobileId);
    ih->setIsIdInitialized(true);
    ih->setIsIdAcked(true);
    ih->setIsSeqValid(true);
    txTrafficStat = ih->getByteLength();
    emit(txTraffic, txTrafficStat);
    numSequenceUpdateStat++;
    emit(numSequenceUpdate,numSequenceUpdateStat);
    sendToLowerLayer(ih,destAddr);
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
    txTrafficStat = ih->getByteLength();
    emit(txTraffic, txTrafficStat);
    numFlowRequestsStat++;
    emit(numFlowRequests,numFlowRequestsStat);
    sendToLowerLayer(ih,destAddr,0.1); // TODO remove delay
}

    // ================================================================================
    // controlAgent message processing
    // ================================================================================
void ControlAgent::processAgentMessage(IdentificationHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    rxTrafficStat = agentHeader->getByteLength();
    emit(rxTraffic, rxTrafficStat);
    IPv6Address destAddr = controlInfo->getSourceAddress().toIPv6(); // address to be responsed
//    IPv6Address sourceAddr = controlInfo->getDestinationAddress().toIPv6();
    if(agentHeader->getIsDataAgent() && agentHeader->getNextHeader() == IP_PROT_NONE) {
        if(agentHeader->getIsIdInitialized() && !agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid() && !agentHeader->getIsIpModified())
            performAgentInitResponse(agentHeader, destAddr); // we need here the source address to remove the timers
        else if(agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid() && !agentHeader->getIsIpModified())
            performAgentUpdateResponse(agentHeader, destAddr); // this function is called when a dataAgents requests a update acknowledgment
        else if(agentHeader->getIsIdInitialized() && agentHeader->getIsIdAcked() && agentHeader->getIsSeqValid() && !agentHeader->getIsAckValid() && agentHeader->getIsIpModified())
            performSeqUpdate(agentHeader, destAddr); // this functions is called when any dataAgent sends a update message.
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
        EV_WARN << "CA_initializeSession: Received session initialization message but ID exists already. Resending acknowledgement to address: " << destAddr.str() << endl;
    } else {
        EV_INFO << "CA_initializeSession: Received session initialization message from Mobile Agent with Id: " << agentHeader->getId() << " Address: " << destAddr.str() << endl;
        mobileIdList.push_back(agentHeader->getId()); // adding id to list (registering id)
    }
    sendSessionInitResponse(destAddr);
}

void ControlAgent::initializeSequence(IdentificationHeader *agentHeader, IPv6Address destAddr)
{
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
        if(isSeqNoInitialized(agentHeader->getId())) { // check if seqNo is initialized
            EV_INFO << "CA_initializeSequence: Sequence number from Mobile Agent(" << agentHeader->getId() << ") already initialized message . Resending sequence number " << getSeqNo(agentHeader->getId()) << " for IP: " << agentHeader->getIPaddresses(0) << endl;
            sendSequenceInitResponse(destAddr, agentHeader->getId(), getSeqNo(agentHeader->getId())); // resend message
        } else {
            EV_INFO << "CA_initializeSequence: Received sequence initialization message from Mobile Agent(" << agentHeader->getId() << "). Registering sequence number " << agentHeader->getIpSequenceNumber() << " for IP: " << agentHeader->getIPaddresses(0) << endl;
            bool addrMgmtEntry = initAddressMap(agentHeader->getId(), agentHeader->getIpSequenceNumber(), agentHeader->getIPaddresses(0)); //first number
            if(addrMgmtEntry) { // check if seq and id is inserted
                sendSequenceInitResponse(destAddr, agentHeader->getId(), getSeqNo(agentHeader->getId()));
            }
            else
                if(isIdInitialized(agentHeader->getId()))
                    EV_WARN << "CA_initializeSequence: Received sequence initialization message from Mobile Agent(" << agentHeader->getId() << ") but sequence number was already registered. Check Mobile Agent procedure." << endl;
                else
                    throw cRuntimeError("CA_initializeSequence: Initialization of sequence number failed.");
        }
    }
    else
        throw cRuntimeError("CA_initializeSequence: CA should initialize sequence number but cannot find ID in list.");
}

void ControlAgent::performSeqUpdate(IdentificationHeader *agentHeader, IPv6Address destAddr)
{ // this method can be optimized
    if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
        if(agentHeader->getIsMobileAgent()) {
            if(agentHeader->getIpSequenceNumber() > getAckNo(agentHeader->getId())) {
                if(agentHeader->getIpSequenceNumber() > getSeqNo(agentHeader->getId())) {
                    if(agentHeader->getIsIpModified()) { // indicate a sequence update
                        // when ackNo of update message is behind current ackNo, sequence number is decremented for starting the update process at ackNo.
                        // Otherwise update process starts at seqNo, but header only declares difference from ackNo to seqNo ( and not from current seqNo
                        if(agentHeader->getIpAcknowledgementNumber() < getAckNo(agentHeader->getId()))
                            setSeqNo(agentHeader->getId(),agentHeader->getIpAcknowledgementNumber());
                        if(agentHeader->getIpAddingField() > 0) { // check size of field
                            for(int i=0; i<agentHeader->getIpAddingField(); i++){
                                insertAddress(agentHeader->getId(), 0, agentHeader->getIPaddresses(i));
                            }
                        }
                        if(agentHeader->getIpRemovingField() > 0) {
                            for(int i=agentHeader->getIpAddingField(); i<(agentHeader->getIpRemovingField()+agentHeader->getIpAddingField()); i++) {
                                deleteAddress(agentHeader->getId(), 0, agentHeader->getIPaddresses(i));
                            }
                        }
                        int s = agentHeader->getIpSequenceNumber(); int a = getAckNo(agentHeader->getId()); int in = agentHeader->getIpAddingField(); int out =  agentHeader->getIpRemovingField();
                        EV_INFO << "CA_performSeqUpdate: Received sequence update message from Mobile Agent(" << agentHeader->getId() << ").\nUpdating table to SeqNo " << s << "and AckNo " << a << "(+" << in << ", -" << out << ")." << endl;
                        // first all DA's are updated. after update confirmation, CA's ack is incremented and subsequently MA is confirmed.
                        if(agentAddressList.size() > 0) {
                            createAgentUpdate(agentHeader->getId(), getSeqNo(agentHeader->getId()));
                        } else {
                            setAckNo(agentHeader->getId(), getSeqNo(agentHeader->getId()));
                            createSequenceUpdateAck(agentHeader->getId());
                        }
                    }
                } else if (agentHeader->getIpSequenceNumber() < getSeqNo(agentHeader->getId())) { // get retransmission from MA
                    EV_WARN << "CA_performSeqUpdate: Received sequence update message from Mobile Agent(" << agentHeader->getId() << ")  but header field provides a sequence number that is lower as the registered sequence number." << endl;
                } else { // following code is executed if DA's hasnt acked but CA has updated its seq no. Just resend update message to DA, so that MA can be confirmed.
                    EV_DEBUG << "CA_performSeqUpdate: Received sequence update message from Mobile Agent(" << agentHeader->getId() << ") but it seems that not all Data Agents has been updated."<< endl;
                    if(agentAddressList.size() > 0)
                        createAgentUpdate(agentHeader->getId(), getSeqNo(agentHeader->getId()));
                    else {
                        setAckNo(agentHeader->getId(), getSeqNo(agentHeader->getId()));
                        createSequenceUpdateAck(agentHeader->getId());
                    }
                }
            } else {
                EV_WARN << "CA_performSeqUpdate: Received sequence update message from Mobile Agent(" << agentHeader->getId() << ")  but header field provides a sequence number that is lower as the registered acknowledgment number." << endl;
            }
        } else if (agentHeader->getIsDataAgent())
        { // updating seq no update by DA
            if(agentHeader->getIpSequenceNumber() > getAckNo(agentHeader->getId())) { // check if CA's ack is older than the received seq. if seq is not greater , CA updated all agents databas with new value but this message is an older one.
                if(agentHeader->getIpSequenceNumber() > getSeqNo(agentHeader->getId())) { //
                    if(agentHeader->getIsIpModified()) {  // indicate a sequence update: Attention: remValid is false
                        AddressList list;
                        if(agentHeader->getIpAddingField() > 0) { // check size of field
                            if(agentHeader->getIpAddingField() != agentHeader->getIPaddressesArraySize()) throw cRuntimeError("CA:Hdr: field of array and field not same. Check header.");
                            for(int i=0; i<agentHeader->getIpAddingField(); i++){ // copy array in vector list
                                AddressTuple tuple(0, agentHeader->getIPaddresses(i));
                                list.push_back(tuple); // inserting elements of array into vector list due to function
                            }
                        }
                        insertTable(agentHeader->getId(), agentHeader->getIpSequenceNumber(), list);
                        int s = agentHeader->getIpSequenceNumber(); int a = getAckNo(agentHeader->getId()); int in = agentHeader->getIpAddingField(); int out =  agentHeader->getIpRemovingField();
                        EV_INFO << "CA_performSeqUpdate: Received sequence update message from Data Agent(" << destAddr.str() << ") on behalf of Mobile Agent(" << agentHeader->getId() << ").\nUpdating table to SeqNo " << s << "and AckNo " << a << "(+" << in << ", -" << out << ")." << endl;
                        createAgentUpdate(agentHeader->getId(), agentHeader->getIpSequenceNumber());
                    } else {
                        EV_WARN << "CA_performSeqUpdate: Received sequence update message from Data Agent(" << destAddr.str() << ") on behalf of Mobile Agent(" << agentHeader->getId() << ") but header fields are not valid." << endl;
                    }
                } else {
                    createAgentUpdate(agentHeader->getId(), agentHeader->getIpSequenceNumber());
                }
            } else {
                EV_DEBUG << "CA_performSeqUpdate: Received sequence update message from Data Agent(" << destAddr.str() << ") on behalf of Mobile Agent(" << agentHeader->getId() << ") but SeqNo <= AckNo." << endl;
//                EV << "CA: Received seqNo update contains lower ack. seq: " << agentHeader->getIpSequenceNumber() << " seq_CA: " << am.getSeqNo(agentHeader->getId()) << endl;
            }
        } else {throw cRuntimeError("CA_performSeqUpdate: Message type is wrong. It should be either MA or DA."); }
    } else {throw cRuntimeError("CA_performSeqUpdate: Id is not found in list."); }
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
    EV_INFO << "CA_performFlowRequest: Received flow request from Mobile Agent(" << agentHeader->getId() << ").\nRequesting Correspondent Node " << nodeAddress << " \nFound best matching Data Agent " << daAddr.toIPv6() << endl;
    sendFlowRequestResponse(destAddr, agentHeader->getId(), getSeqNo(agentHeader->getId()), daAddr.toIPv6(), nodeAddress);
}

void ControlAgent::performAgentInitResponse(IdentificationHeader *agentHeader, IPv6Address sourceAddr)
{
    if(cancelAndDeleteExpiryTimer(sourceAddr,-1, TIMERKEY_MA_INIT, agentHeader->getId())) {
        EV_DEBUG << "CA_performAgentInitResponse: Received Data Agent initialization acknowledgment. A timer existed and has been removed." << endl;
        // TODO sendFlowRequest should be executed from here
    } else {
//        EV << "CA: No TIMER for DA init exists. So no initialization can be done." << endl;
    }
}

void ControlAgent::performAgentUpdateResponse(IdentificationHeader *agentHeader, IPv6Address sourceAddr)
{
    // if this is true, then a timer key existed. so a timer can only exist when the data agent were not initialized.
    // if false, then data agents has been initialized. only a timer for update can exists.
    cancelAndDeleteExpiryTimer(sourceAddr,-1,TIMERKEY_SEQ_UPDATE, agentHeader->getId(), agentHeader->getIpSequenceNumber(), getAckNo(agentHeader->getId()));
    sendSequenceUpdateResponse(sourceAddr, agentHeader->getId(), agentHeader->getIpSequenceNumber());
    bool allDataAgentsUpdated = true;
    for(IPv6Address ip : agentAddressList) {
        if(pendingExpiryTimer(ip,-1,TIMERKEY_SEQ_UPDATE, agentHeader->getId(), agentHeader->getIpSequenceNumber()))
            allDataAgentsUpdated = false;
    }
    if(allDataAgentsUpdated) {
        if(agentHeader->getIpSequenceNumber() > getAckNo(agentHeader->getId())) {
            setAckNo(agentHeader->getId(), agentHeader->getIpSequenceNumber()); // we set here ack no of CA when all DA's has been updated.
        }
        createSequenceUpdateAck(agentHeader->getId());
        EV_DEBUG << "CA_performAgentUpdateResponse: All acknowledgments of sequence update messages from Data Agents are received." << endl;
    } else {
        EV_DEBUG << "CA_performAgentUpdateResponse: Received acknowledgment of sequence update message (" << agentHeader->getIpSequenceNumber() << ") from Data Agent " << sourceAddr << endl;
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
    if (delayTime > 0)
        sendDelayed(msg, delayTime, outgate);
    else
        send(msg, outgate);
}

} //namespace
