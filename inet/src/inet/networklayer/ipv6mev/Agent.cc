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
#include "inet/networklayer/ipv6mev/Agent.h"

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

#include "inet/common/NotifierConsts.h"
#include "inet/common/lifecycle/NodeStatus.h"

namespace inet {

Define_Module(Agent);

void Agent::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_NETWORK_LAYER) {
        isMA = par("isMA").boolValue();
        isCA = par("isCA").boolValue();
        isDA = par("isDA").boolValue();
        ctrlAgentAddr = par("controlAgentAddress");
        ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        if(isMA) {
            sessionState = UNASSOCIATED;
            seqnoState = UNASSOCIATED;
            srand(123); // TODO must be changed
            mobileId = (uint64) rand(); // TODO should be placed in future
            am.initiateAddressMap(mobileId, rand());
        }
        if(isCA) {}
    }
    if(stage == INITSTAGE_TRANSPORT_LAYER) {
        IPSocket ipSocket(gate("toLowerLayer")); // register own protocol
        ipSocket.registerProtocol(IP_PROT_IPv6EXT_ID);
        interfaceNotifier = getContainingNode(this);
        interfaceNotifier->subscribe(NF_INTERFACE_STATE_CHANGED,this); // register signal listener
        interfaceNotifier->subscribe(NF_INTERFACE_IPv6CONFIG_CHANGED,this);
    }
    if (stage == INITSTAGE_APPLICATION_LAYER) {
        if(isMA) {
            startTime = par("startTime");
            cMessage *timeoutMsg = new cMessage("sessionStart");
            timeoutMsg->setKind(MSG_START_TIME);
//            scheduleAt(startTime, timeoutMsg); // delaying start
        }
    }

    WATCH(mobileId);
    WATCH(CA_Address);
//    WATCH(isMA);
//    WATCH(isCA);
//    WATCHMAP(interfaceToIPv6AddressList);
//    WATCHMAP(directAddressList);
}

void Agent::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()) {
//        EV << "Self message received: " << msg->getKind() << endl;
        if(msg->getKind() == MSG_START_TIME) {
//            EV << "Starter msg received" << endl;
            createSessionInit();
            delete msg;
        }
        else if(msg->getKind() == MSG_SESSION_INIT) {
//            EV << "A: CA_init_msg received" << endl;
            sendSessionInit(msg);
        }
        else if(msg->getKind() == MSG_SEQNO_INIT) {
//            EV << "A: CA_init_msg received" << endl;
            sendSequenceInit(msg);
        }
        else if(msg->getKind() == MSG_IF_DOWN) {
//            EV << "MA: Interface down timer received" << endl;
            handleInterfaceDownMessage(msg);
        }
        else if(msg->getKind() == MSG_IF_UP) {
//            EV << "MA: Interface up timer received" << endl;
            handleInterfaceUpMessage(msg);
        }
        else if(msg->getKind() == MSG_SEQ_UPDATE) {
            sendSequenceUpdate(msg);
        }
        else if(msg->getKind() == MSG_SEQ_UPDATE_DELAYED) {
            createSequenceUpdate();
            delete msg;
        }

        else
            throw cRuntimeError("handleMessage: Unknown timer expired. Which timer msg is unknown?");
    }
//    else if (dynamic_cast<IPv6Datagram *>(msg)) {
//        EV << "IPv6Datagram received." << endl;
//        IPv6ExtensionHeader *eh = (IPv6ExtensionHeader *)msg->getContextPointer();
//        IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
//        IPv6Datagram *datagram = (IPv6Datagram *)msg;
//        if(dynamic_cast<MobileAgentOptionHeader *>(eh)) {
//            EV << "MobileAgentOptionHeader received." << endl;
//            MobileAgentOptionHeader *optHeader = (MobileAgentOptionHeader *)eh;
//            messageProcessingUnitMA(optHeader, datagram, controlInfo);
//        }
//        else if(dynamic_cast<ControlAgentOptionHeader *>(eh)) {
//            EV << "ControlAgentOptionHeader received." << endl;
//            ControlAgentOptionHeader *optHeader = (ControlAgentOptionHeader *)eh;
//            messageProcessingUnitCA(optHeader, datagram, controlInfo);
//        }
//        else if(dynamic_cast<ControlAgentOptionHeader *>(eh)) {
//            EV << "DataAgentOptionHeader received. Not implemented yet." << endl;
//        }
//    }
    else if (dynamic_cast<IdentificationHeader *> (msg)) {
//        EV << "A: Received id_header" << endl;
        IPv6ControlInfo *ctrlInfo = check_and_cast<IPv6ControlInfo *>(msg->removeControlInfo());
        IdentificationHeader *idHdr = (IdentificationHeader *) msg;
        if (dynamic_cast<ControlAgentHeader *>(idHdr)) {
            ControlAgentHeader *ca = (ControlAgentHeader *) idHdr;
            processControlAgentMessage(ca, ctrlInfo);
        } else if (dynamic_cast<DataAgentHeader *>(idHdr)) {
//            DataAgentHeader *da = (DataAgentHeader *) idHdr;
//            processDAMessages(da, ctrlInfo);
        } else if (dynamic_cast<MobileAgentHeader *>(idHdr)) {
            MobileAgentHeader *ma = (MobileAgentHeader *) idHdr;
            processMobileAgentMessages(ma,ctrlInfo);
        } else {
            throw cRuntimeError("A:handleMsg: Extension Hdr Type not known. What did you send?");
        }
    }
    else
        throw cRuntimeError("A:handleMsg: cMessage Type not known. What did you send?");
}

Agent::Agent() {}

Agent::~Agent() {
    auto it = expiredTimerList.begin();
    while(it != expiredTimerList.end()) {
        TimerKey key = it->first;
        it++;
        cancelAndDeleteExpiryTimer(key.dest,key.interfaceID,key.type);
    }
}

void Agent::createSessionInit() {
    EV << "MA: Create CA_init" << endl;
    L3Address caAddr;
    L3AddressResolver().tryResolve(ctrlAgentAddr, caAddr);
    CA_Address = caAddr.toIPv6();
    if(CA_Address.isGlobal()) {
        if(isMA && sessionState == UNASSOCIATED) sessionState = INITIALIZE;
        InterfaceEntry *ie = getInterface(CA_Address); // TODO ie not correctly set
        if(!ie) { throw cRuntimeError("MA: No interface exists."); }
        cMessage *msg = new cMessage("sendingCAinit",MSG_SESSION_INIT);
        TimerKey key(CA_Address,ie->getInterfaceId(),TIMERKEY_SESSION_INIT);
        SessionInitTimer *sit = (SessionInitTimer *) getExpiryTimer(key,TIMERTYPE_SESSION_INIT);
        sit->dest = CA_Address;
        sit->ie = ie;
        sit->timer = msg;
        sit->ackTimeout = TIMEOUT_SESSION_INIT;
        sit->nextScheduledTime = simTime();
        msg->setContextPointer(sit);
        scheduleAt(simTime(),msg);
    } else {
        EV << "MA: Create CA_init is delayed." << endl;
        cMessage *timeoutMsg = new cMessage("sessionStart");
        timeoutMsg->setKind(MSG_START_TIME);
        scheduleAt(simTime()+TIMEOUT_SESSION_INIT, timeoutMsg);
    }
}

void Agent::createSequenceInit() {
    EV << "MA: Create CA_seq_init" << endl;
    if(!isMA && sessionState != REGISTERED) { throw cRuntimeError("MA: Not registered at CA. Cannot run seq init."); }
    if(isMA && seqnoState == UNASSOCIATED) { seqnoState = INITIALIZE; }
    cMessage *msg = new cMessage("sendingCAseqInit", MSG_SEQNO_INIT);
    InterfaceEntry *ie = getInterface(CA_Address);
    if(!ie) { throw cRuntimeError("MA: No interface exists."); }
    TimerKey key(CA_Address,ie->getInterfaceId(),TIMERKEY_SEQNO_INIT);
    SequenceInitTimer *sit = (SequenceInitTimer *) getExpiryTimer(key, TIMERTYPE_SEQNO_INIT);
    sit->dest = CA_Address;
    sit->ie = ie;
    sit->timer = msg;
    sit->ackTimeout = TIMEOUT_SEQNO_INIT;
    sit->nextScheduledTime = simTime();
    msg->setContextPointer(sit);
    scheduleAt(sit->nextScheduledTime, msg);
}

void Agent::createSequenceUpdate() {
    EV << "MA: Create SeqUpdateMsg" << endl;
    if(!isMA && sessionState != REGISTERED &&  seqnoState != REGISTERED) { throw cRuntimeError("MA: Not registered at CA. Cannot run seq init."); }
    cMessage *msg = new cMessage("sendingCAseqUpdate", MSG_SEQ_UPDATE);
    InterfaceEntry *ie = getInterface(CA_Address);
    if(!ie) {
        EV << "MA: Delaying seq update. no interface provided." << endl;
        cMessage *timeoutMsg = new cMessage("sessionStart");
        timeoutMsg->setKind(MSG_SEQ_UPDATE_DELAYED);
        scheduleAt(simTime()+TIMEOUT_SEQ_UPDATE, timeoutMsg);
    } else {
        EV << "MA: Sending seq update without delay. interface provided." << endl;
        TimerKey key(CA_Address, -1, TIMERKEY_SEQ_UPDATE);
        SequenceUpdateTimer *sut = (SequenceUpdateTimer *) getExpiryTimer(key, TIMERTYPE_SEQ_UPDATE);
        sut->dest = CA_Address;
        sut->ie = ie;
        sut->timer = msg;
        sut->ackTimeout = TIMEOUT_SEQ_UPDATE;
        sut->nextScheduledTime = simTime();
        msg->setContextPointer(sut);
        scheduleAt(sut->nextScheduledTime, msg);
    }
}

void Agent::sendSequenceUpdate(cMessage* msg)
{
    EV << "MA: Send SeqUpdate" << endl;
    SequenceUpdateTimer *sut = (SequenceUpdateTimer *) msg->getContextPointer();
    InterfaceEntry *ie = sut->ie;
    const IPv6Address &dest =  CA_Address;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress(); // to get src ip
    sut->nextScheduledTime = simTime() + sut->ackTimeout;
    sut->ackTimeout = (sut->ackTimeout)*2;
    MobileAgentHeader *mah = new MobileAgentHeader("ca_seq_upd");
    mah->setIdInit(true);
    mah->setIdAck(true);
    mah->setSeqValid(true);
    mah->setAckValid(true);
    mah->setAddValid(true);
    mah->setRemValid(true);
    mah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    mah->setIpAcknowledgementNumber(am.getLastAcknowledgemnt(mobileId));
    AddressManagement::AddressChange ac = am.getUnacknowledgedIPv6AddressList(mobileId,am.getLastAcknowledgemnt(mobileId),am.getCurrentSequenceNumber(mobileId));
    mah->setIpAddingField(ac.addedAddresses);
    mah->setAddedAddressesArraySize(ac.addedAddresses);
    if(ac.addedAddresses > 0) {
        if(ac.addedAddresses != ac.getUnacknowledgedAddedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqUpd: value of Add list must have size of integer.");
        for(int i=0; i<ac.addedAddresses; i++) {
//            EV << "ID_Hdr_ADD: " << ac.getUnacknowledgedAddedIPv6AddressList.at(i) << endl;
            mah->setAddedAddresses(i,ac.getUnacknowledgedAddedIPv6AddressList.at(i));
        }
    }
    mah->setIpRemovingField(ac.removedAddresses);
    mah->setRemovedAddressesArraySize(ac.removedAddresses);
    if(ac.removedAddresses > 0) {
        if(ac.removedAddresses != ac.getUnacknowledgedRemovedIPv6AddressList.size()) throw cRuntimeError("MA:sendSeqUpd: value of Rem list must have size of integer.");
        for(int i=0; i<ac.removedAddresses; i++) {
//            EV << "ID_Hdr_REM: " << ac.getUnacknowledgedRemovedIPv6AddressList.at(i) << endl;
            mah->setRemovedAddresses(i,ac.getUnacknowledgedRemovedIPv6AddressList.at(i));
        }
    }
    mah->setId(mobileId);
    mah->setByteLength(SIZE_AGENT_HEADER+(SIZE_ADDING_ADDR_TO_HDR*(ac.addedAddresses+ac.removedAddresses)));
    sendToLowerLayer(mah, dest, src);
    scheduleAt(sut->nextScheduledTime, msg);
}

void Agent::sendSessionInit(cMessage* msg) {
    EV << "Send CA_init" << endl;
    SessionInitTimer *sit = (SessionInitTimer *) msg->getContextPointer();
    InterfaceEntry *ie = sit->ie;
// TODO check for global address if not skip this round
    const IPv6Address &dest =  CA_Address;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress(); // to get src ip
    sit->dest = CA_Address;
    sit->nextScheduledTime = simTime() + sit->ackTimeout;
    sit->ackTimeout = (sit->ackTimeout)*2;
    if(CA_Address.isGlobal()) { // not necessary
        MobileAgentHeader *mah = new MobileAgentHeader("ca_init");
        mah->setIdInit(true);
        mah->setIdAck(false);
        mah->setId(mobileId);
        mah->setByteLength(SIZE_AGENT_HEADER);
        sendToLowerLayer(mah, dest, src);
    }
    scheduleAt(sit->nextScheduledTime, msg);
}

void Agent::sendSequenceInit(cMessage *msg) {
    EV << "MA: Send Seq_init_to_CA" << endl;
    SequenceInitTimer *sit = (SequenceInitTimer *) msg->getContextPointer();
    InterfaceEntry *ie = sit->ie;
    const IPv6Address &dest = sit->dest;
    const IPv6Address &src = ie->ipv6Data()->getPreferredAddress();
    sit->nextScheduledTime = simTime() + sit->ackTimeout;
    sit->ackTimeout = (sit->ackTimeout)*2;
    MobileAgentHeader *mah = new MobileAgentHeader("ca_seq_init");
    mah->setIdInit(true);
    mah->setIdAck(true);
    mah->setSeqValid(true);
    mah->setAckValid(false);
    mah->setAddValid(true);
    mah->setRemValid(false);
    mah->setIpSequenceNumber(am.getCurrentSequenceNumber(mobileId));
    mah->setIpAcknowledgementNumber(0);
    mah->setAddedAddressesArraySize(1);
    mah->setAddedAddresses(0,src);
    mah->setId(mobileId);
    mah->setByteLength(SIZE_AGENT_HEADER+SIZE_ADDING_ADDR_TO_HDR);
    sendToLowerLayer(mah, dest, src);
    scheduleAt(sit->nextScheduledTime, msg);
}



void Agent::sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr, int interfaceId, simtime_t delayTime) {
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
    EV << "ControlInfo: DestAddr=" << ctrlInfo->getDestAddr() << " SrcAddr=" << ctrlInfo->getSrcAddr() << " InterfaceId=" << ctrlInfo->getInterfaceId() << endl;
    if (delayTime > 0) {
        EV << "delayed sending" << endl;
        sendDelayed(msg, delayTime, outgate);
    }
    else {
        send(msg, outgate);
    }
}

void Agent::processMobileAgentMessages(MobileAgentHeader* agentHeader, IPv6ControlInfo* controlInfo)
{
    EV << "CA: processing message from MA" << endl;
    if(isCA)
    {
        IPv6Address destAddr = controlInfo->getSrcAddr(); // address to be responsed
        IPv6Address sourceAddr = controlInfo->getDestAddr();
        if(agentHeader->getIdInit() && !agentHeader->getIdAck()) // check for other bits
        { // init process request
            if(agentHeader->isName("ca_init"))
                EV << "CA: Received init message from MA." << endl;
//            std::find(newIPv6AddressList.begin(), newIPv6AddressList.end(), addr) != newIPv6AddressList.end()) // check if at any position given ip addr exists
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) { EV << "CA: Id already exists. check for mistakes" << endl; }
            else {
                EV << "CA: Adding id to list: " << agentHeader->getId() << endl;
                mobileIdList.push_back(agentHeader->getId());
                ControlAgentHeader *cah = new ControlAgentHeader("ma_init_ack");
                cah->setIdInit(true);
                cah->setIdAck(true);
//                cah->setHeaderLength(SIZE_AGENT_HEADER); // not necessary since 8 B is defined as default
                // TODO remove below assignment from here
                sendToLowerLayer(cah,destAddr, sourceAddr);
            }
        } else if (agentHeader->getSeqValid() && !agentHeader->getAckValid() && agentHeader->getIdInit() && agentHeader->getIdAck()) // || agentHdr->getIdInit() && agentHdr->getIdAck()
        { // init sequence number/address management
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end()) {
                EV << "CA: Received sequence initialize message from MA. Inserting in AddrMgmt-Unit." << endl;
                bool addrMgmtEntry = am.insertNewId(agentHeader->getId(), agentHeader->getIpSequenceNumber(), agentHeader->getAddedAddresses(0));//first number
                if(addrMgmtEntry) {
                    EV << "CA: Initialized sequence number: " << agentHeader->getIpSequenceNumber() << "; ip: " << agentHeader->getAddedAddresses(0) << endl;
//                    if(agentHeader->getAddedAddresses(0) == destAddr) { EV << "CA: registered ip addr same as to sender addr" << endl; }
//                    else { EV << "ERROR: sender addr not equal to listed ip addr:" << agentHeader->getAddedAddresses(0) << "!=" << destAddr << endl; }
                    ControlAgentHeader *cah = new ControlAgentHeader("ma_seq_ack");
                    cah->setIdInit(true);
                    cah->setIdAck(true);
                    cah->setSeqValid(true);
                    cah->setIpSequenceNumber(am.getCurrentSequenceNumber(agentHeader->getId()));
                    sendToLowerLayer(cah,destAddr, sourceAddr);
                }
                else {
                    if(am.isIdInListgiven(agentHeader->getId())) { EV << "+++++ERROR+++++ CA: Id has been initialized before. Why do you send me again an init message?" << endl; }
                    else { throw cRuntimeError("CA: Initialization of sequence number failed, CA could not insert id in AddrMgmt-Unit."); }
                }
            }
            else { throw cRuntimeError("CA should initialize sequence number but cannot find id in list."); }
        } else if(agentHeader->getSeqValid() && agentHeader->getAckValid() && agentHeader->getIdInit() && agentHeader->getIdAck()) { // all fields of header must be one
            if(std::find(mobileIdList.begin(), mobileIdList.end(), agentHeader->getId()) != mobileIdList.end())
            {
//                EV << "AM_CA_prev: " << am.to_string() << endl;
//                EV << "ID_HDR:ADD_V:" << agentHeader->getAddValid() << ";REM_V:" << agentHeader->getRemValid()
//                        << ";ADD:" << agentHeader->getIpAddingField()
//                        << ";REM:" << agentHeader->getIpRemovingField()
//                        << ";ACK:" << (uint8) agentHeader->getIpAcknowledgementNumber()
//                        << ";SEQ:" << (uint8) agentHeader->getIpSequenceNumber();
//                for(int i=0; i<agentHeader->getAddedAddressesArraySize(); i++){
//                    EV << ";IP_a:" << agentHeader->getAddedAddresses(i);
//                }
//                for(int i=0; i<agentHeader->getRemovedAddressesArraySize(); i++){
//                    EV << ";IP_r:" << agentHeader->getRemovedAddresses(i);
//                }
//                EV << ";" << endl;
                if(agentHeader->getAddValid() || agentHeader->getRemValid()) { // indicate a sequence update
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
                }
                if(agentHeader->getIpSequenceNumber() != am.getCurrentSequenceNumber(agentHeader->getId())) {
                    EV << "ERROR CA:Hdr: value sent is not same with value of am db" << endl;
                } else {
                    am.setLastAcknowledgemnt(agentHeader->getId(), agentHeader->getIpSequenceNumber());
                }
                if(agentHeader->getDirectAddrInit() || agentHeader->getRedirectAddrInit()) { // check if redirect address is requested
                    throw cRuntimeError("CA:hdr: not implemented yet");
                }
                EV << "CA: Initialized sequence number: " << agentHeader->getIpSequenceNumber() << "; ip: " << agentHeader->getAddedAddresses(0) << endl;
                ControlAgentHeader *cah = new ControlAgentHeader("ma_seq_update");
                cah->setIdInit(true);
                cah->setIdAck(true);
                cah->setSeqValid(true);
                cah->setIpSequenceNumber(am.getCurrentSequenceNumber(agentHeader->getId()));
                sendToLowerLayer(cah,destAddr, sourceAddr);
            } else { throw cRuntimeError("CA: Mobile id not known."); }
        } else { throw cRuntimeError("CA: Message not known. Parameter bits of header are not set correctly."); }
    } else { throw cRuntimeError("Received message that is supposed to be received by control agent. check message type that has to be processed."); }
    EV << "AM_CA: " << am.to_string() << endl;
    delete agentHeader;
    delete controlInfo;
}

void Agent::processControlAgentMessage(ControlAgentHeader* agentHeader, IPv6ControlInfo* controlInfo) {
    EV << "MA: Entering CA processing message" << endl;
    if(isMA) {
        if(agentHeader->getIdInit() && agentHeader->getIdAck() && !agentHeader->getSeqValid()) { // session init
            if(sessionState == INITIALIZE) {
                sessionState = REGISTERED;
                if(agentHeader->isName("ma_init_ack")) { EV << "MA: Received session ack from CA. Session started.: " << endl; }
                IPv6Address &caAddr = controlInfo->getSrcAddr();
                InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
                cancelAndDeleteExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_SESSION_INIT);
                EV << "MA: Session init timer removed. Process successfully finished." << endl;
                createSequenceInit();
            }
            else { EV << "MA: Session created yet. Why do I received it again?" << endl; }
        }
        else if (agentHeader->getIdInit() && agentHeader->getIdAck() && agentHeader->getSeqValid()) {
            if(seqnoState == INITIALIZE) {
                seqnoState = REGISTERED;
                am.setLastAcknowledgemnt(mobileId, agentHeader->getIpSequenceNumber());
                if(agentHeader->isName("ma_seq_ack"))
                    EV << "MA: Received seqno ack. SeqNo initialized." << endl;
                IPv6Address &caAddr = controlInfo->getSrcAddr();
                InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
                cancelAndDeleteExpiryTimer(caAddr,ie->getInterfaceId(), TIMERKEY_SEQNO_INIT);
                EV << "MA: Seqno init timer removed. Process successfully finished." << endl;
            }
            else {
                if(agentHeader->isName("ma_seq_update"))
                    EV << "MA: Received seqno update. SeqNo is up to date." << endl;
                am.setLastAcknowledgemnt(mobileId, agentHeader->getIpSequenceNumber());
                IPv6Address &caAddr = controlInfo->getSrcAddr();
//                InterfaceEntry *ie = ift->getInterfaceById(controlInfo->getInterfaceId());
                cancelAndDeleteExpiryTimer(caAddr,-1, TIMERKEY_SEQ_UPDATE); // TODO interface should be set to -1 because the ack should independent of src. fix this also at sent method.
                EV << "MA: Seqno update timer removed. Process successfully finished." << endl;
//                EV << "AM_MA: " << am.to_string() << endl;
            }
        }
    }
    delete agentHeader; // delete at this point because it's not used any more
    delete controlInfo;
}

// TODO this function must be adjusted for signal strentgh. it's returning any interface currently.
InterfaceEntry *Agent::getInterface(IPv6Address destAddr, int destPort, int sourcePort, short protocol) { // const IPv6Address &destAddr,
    InterfaceEntry *ie = nullptr;
    for (int i=0; i<ift->getNumInterfaces(); i++) {
        ie = ift->getInterface(i);
        if(!(ie->isLoopback()) && ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal()) { return ie; }
    }
    return ie;
}

void Agent::createInterfaceDownMessage(int id)
{
    IPv6Address ip;
    if(!cancelAndDeleteExpiryTimer(ip.UNSPECIFIED_ADDRESS, id,TIMERKEY_IF_DOWN)) // no l2 disassociation timer exists
    {
        TimerKey key(ip.UNSPECIFIED_ADDRESS,id,TIMERKEY_IF_DOWN);
        InterfaceDownTimer *l2dt = (InterfaceDownTimer*) getExpiryTimer(key, TIMERTYPE_IF_DOWN);
        int i;
        for (i=0; i<ift->getNumInterfaces(); i++) {
            if(ift->getInterface(i)->getInterfaceId() == id) break;
        }
        cMessage *msg = new cMessage("InterfaceDownTimer", MSG_IF_DOWN);
        l2dt->ie = ift->getInterface(i);
        l2dt->timer = msg;
        l2dt->nextScheduledTime = simTime()+TIMEDELAY_IF_DOWN;
        msg->setContextPointer(l2dt);
        scheduleAt(l2dt->nextScheduledTime, msg);
    }
    EV << "MA: Create interface down timer message: " << id << endl;
}

void Agent::handleInterfaceDownMessage(cMessage *msg)
{
    IPv6Address ip;
    InterfaceDownTimer *l2dt = (InterfaceDownTimer *) msg->getContextPointer();
    InterfaceEntry *ie = l2dt->ie;
    InterfaceUnit *iu = getInterfaceUnit(ie->getInterfaceId());
    iu->active = false;
    iu->priority = -1;
    EV << "MA: handle interface down message. Addr: " << iu->careOfAddress.str() << endl;
    updateAddressTable(ie->getInterfaceId(), iu);
    cancelExpiryTimer(ip.UNSPECIFIED_ADDRESS,ie->getInterfaceId(),TIMERKEY_IF_DOWN);
    delete msg;
}

void Agent::createInterfaceUpMessage(int id)
{
    IPv6Address ip;
    if(!cancelAndDeleteExpiryTimer(ip.UNSPECIFIED_ADDRESS, id,TIMERKEY_IF_DOWN)) // no l2 disassociation timer exists
    {
        TimerKey key(ip.UNSPECIFIED_ADDRESS, id, TIMERKEY_IF_UP);
        InterfaceUpTimer *l2at = new InterfaceUpTimer();
        int i;
        for (i=0; i<ift->getNumInterfaces(); i++) {
            if(ift->getInterface(i)->getInterfaceId() == id) break;
        }
        cMessage *msg = new cMessage("L2AssociaitonTimer", MSG_IF_UP);
        l2at->ie = ift->getInterface(i);
        l2at->timer = msg;
        l2at->nextScheduledTime = simTime()+TIMEDELAY_IF_UP;
        msg->setContextPointer(l2at);
        scheduleAt(l2at->nextScheduledTime, msg);
    } else {
        EV << "Timer was configured. Is deleted." << endl;
    }
    EV << "MA: create interface up timer message" << endl;
}

void Agent::handleInterfaceUpMessage(cMessage *msg)
{
    IPv6Address ip;
    InterfaceUpTimer *l2at = (InterfaceUpTimer *) msg->getContextPointer();
    InterfaceEntry *ie = l2at->ie;
    InterfaceUnit *iu = getInterfaceUnit(ie->getInterfaceId());
    iu->active = true;
    iu->priority = 0;
    iu->careOfAddress = ie->ipv6Data()->getPreferredAddress();
    EV << "MA: Handle interface up message. Addr: " << iu->careOfAddress.str() << endl;
    updateAddressTable(ie->getInterfaceId(), iu);
    cancelExpiryTimer(ip.UNSPECIFIED_ADDRESS,ie->getInterfaceId(),TIMERKEY_IF_UP);
    delete msg;
}

void Agent::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    if(!isMA) { return; } // just execute for mobile node
    Enter_Method_Silent();
    if(signalID == NF_INTERFACE_STATE_CHANGED) { // is triggered when carrier setting is changed
//        if(dynamic_cast<InterfaceEntryChangeDetails *>(obj)) {
//            EV << "NF_INTERFACE_STATE_CHANGED" << endl;
//            InterfaceEntry *ie = check_and_cast<const InterfaceEntryChangeDetails *>(obj)->getInterfaceEntry();
//            if(ie->isUp() && ie->ipv6Data()->getPreferredAddress().isGlobal()) {
//                createInterfaceUpMessage(ie->getInterfaceId());
//            } else {
//            }
//            createInterfaceDownMessage(ie->getInterfaceId());
//        }
    }
    if(signalID == NF_INTERFACE_IPv6CONFIG_CHANGED) { // is triggered when carrier setting is changed
        if(dynamic_cast<InterfaceEntryChangeDetails *>(obj)) {
            InterfaceEntry *ie = check_and_cast<const InterfaceEntryChangeDetails *>(obj)->getInterfaceEntry();
//            EV << "NF_INTERFACE_IPv6CONFIG_CHANGED:" << endl; // << ie->ipv6Data()->getPreferredAddress().str() << endl;
            if(ie->isUp()) { createInterfaceUpMessage(ie->getInterfaceId()); }
            else { createInterfaceDownMessage(ie->getInterfaceId()); }
//            createInterfaceUpMessage(ie->getInterfaceId());
//            if(ie->isUp()) { createInterfaceUpMessage(ie->getInterfaceId()); }
//            else {  }
        }
    }

}

Agent::InterfaceUnit *Agent::getInterfaceUnit(int id)
{
    InterfaceUnit *iu;
    auto it = addressTable.find(id);
    if(it != addressTable.end()) { // addressTable contains an instance of interfaceUnit
        iu = it->second;
        return iu;
    } else {
        iu = new InterfaceUnit();
        iu->active = false;
        iu->priority = -1;
        return iu;
    }
}

void Agent::updateAddressTable(int id, InterfaceUnit *iu)
{
    auto it = addressTable.find(id);
    if(it != addressTable.end()) { // check if interface is provided in address table
        if(it->first != id) throw cRuntimeError("ERROR in updateAddressTable: provided id should be same with entry");
        (it->second)->active = iu->active;
        (it->second)->priority = iu->priority;
        (it->second)->careOfAddress = iu->careOfAddress;
        if(iu->active) {    // presents an interface that has been associated
            am.addIPv6AddressToAddressMap(mobileId, iu->careOfAddress);
        } else { // presents an interface has been disassociated
            am.removeIPv6AddressFromAddressMap(mobileId, iu->careOfAddress);
        }
        createSequenceUpdate();
    } else {
        addressTable.insert(std::make_pair(id,iu)); // if not, include this new
        am.addIPv6AddressToAddressMap(mobileId, iu->careOfAddress);
        if(sessionState == UNASSOCIATED) { createSessionInit(); }
    }
    EV << "AM_MA: " << am.to_string() << endl;
}



//=============================================================
//============================ Timer ==========================
// Returns the corresponding timer. If a timer does not exist, it is created and inserted to list.
// If a timer exists, it is canceled and should be overwritten
Agent::ExpiryTimer *Agent::getExpiryTimer(TimerKey& key, int timerType) {
    ExpiryTimer *timer;
    auto pos = expiredTimerList.find(key);
    if(pos != expiredTimerList.end()) {
        if(dynamic_cast<SessionInitTimer *>(pos->second)) {
            SessionInitTimer *sit = (SessionInitTimer *) pos->second;
            cancelAndDelete(sit->timer);
            timer = sit;
        } else if(dynamic_cast<SequenceInitTimer *>(pos->second)) {
            SequenceInitTimer *sit = (SequenceInitTimer *) pos->second;
            cancelAndDelete(sit->timer);
            timer = sit;
        } else if(dynamic_cast<SequenceUpdateTimer *>(pos->second)) {
            SequenceUpdateTimer *sut = (SequenceUpdateTimer *) pos->second;
            cancelAndDelete(sut->timer);
            timer = sut;
        } else if(dynamic_cast<LocationUpdateTimer *>(pos->second)) {
            LocationUpdateTimer *lut = (LocationUpdateTimer *) pos->second;
            cancelAndDelete(lut->timer);
            timer = lut;
        } else if(dynamic_cast<InterfaceDownTimer *>(pos->second)) {
//            L2DisassociationTimer *l2dt = (L2DisassociationTimer *) pos->second;
//            cancelAndDelete(l2dt->timer);
//            timer = l2dt;
            throw cRuntimeError("ERROR Invoked L2Disassociation timer creation although one in map exists. There shouldn't be a one in the list");
        } else if(dynamic_cast<InterfaceUpTimer *>(pos->second)) {
            throw cRuntimeError("ERROR Invoked L2Association timer creation although one in map exists. There shouldn't be a one in the list");
        } else { throw cRuntimeError("ERROR Received timer not known. It's not a subclass of ExpiryTimer. Therefore getExpir... throwed this exception."); }
        timer->timer = nullptr;
    }
    else { // no timer exist
        switch(timerType) {
            case TIMERTYPE_SESSION_INIT:
                timer = new SessionInitTimer();
                break;
            case TIMERTYPE_SEQNO_INIT:
                timer = new SequenceInitTimer();
                break;
            case TIMERTYPE_SEQ_UPDATE:
                timer = new SequenceUpdateTimer();
                break;
            case TIMERTYPE_LOC_UPDATE:
                timer = new LocationUpdateTimer();
                break;
            case TIMERTYPE_IF_DOWN:
                timer = new InterfaceDownTimer();
                break;
            case TIMERTYPE_IF_UP:
                timer = new InterfaceUpTimer();
                break;
            default:
                throw cRuntimeError("Timer is not known. Type of key is wrong, check that.");
        }
        timer->timer = nullptr; // setting new timer to null
        timer->ie = nullptr; // setting new timer to null
        expiredTimerList.insert(std::make_pair(key,timer)); // inserting timer in list
    }
    return timer;
}

bool Agent::cancelExpiryTimer(const IPv6Address &dest, int interfaceId, int timerType)
{
    TimerKey key(dest, interfaceId, timerType);
    auto pos = expiredTimerList.find(key);
    if(pos == expiredTimerList.end()) {
        return false; // list is empty
    }
    ExpiryTimer *timerToDelete = (pos->second);
    cancelEvent(timerToDelete->timer);
    expiredTimerList.erase(key);
    return true;
}

bool Agent::cancelAndDeleteExpiryTimer(const IPv6Address &dest, int interfaceId, int timerType)
{
    TimerKey key(dest, interfaceId, timerType);
    auto pos = expiredTimerList.find(key);
    if(pos == expiredTimerList.end()) {
        return false; // list is empty
    }
    ExpiryTimer *timerToDelete = (pos->second);
    cancelAndDelete(timerToDelete->timer);
    timerToDelete->timer = nullptr;
    expiredTimerList.erase(key);
    delete timerToDelete;
    return true;
}



bool Agent::pendingExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType) {
    TimerKey key(dest,interfaceId, timerType);
    auto pos = expiredTimerList.find(key);
    return pos != expiredTimerList.end();
}

} //namespace
