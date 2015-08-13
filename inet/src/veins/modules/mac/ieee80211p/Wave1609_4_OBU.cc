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

#include "veins/modules/mac/ieee80211p/Wave1609_4_OBU.h"

#include "veins/modules/phy/DeciderResult80211.h"
#include "veins/base/phyLayer/PhyToMacControlInfo.h"
#include "veins/modules/messages/PhyControlMessage_m.h"
#include <iterator>

#include "inet/linklayer/common/Ieee802Ctrl.h"


#define DBG_MAC EV

Define_Module(Wave1609_4_OBU);

void Wave1609_4_OBU::initialize(int stage)
{
    BaseMacLayer::initialize(stage);
    if (stage == 0) {

        phy11p = FindModule<Mac80211pToPhy11pInterface*>::findSubModule(getParentModule());
        assert(phy11p);

        //this is required to circumvent double precision issues with constants from CONST80211p.h
        assert(simTime().getScaleExp() == -12);

        sigChannelBusy = registerSignal("sigChannelBusy");
        sigCollision = registerSignal("sigCollision");

        txPower = par("txPower").doubleValue();
        bitrate = par("bitrate").longValue();
        n_dbps = 0;
        setParametersForBitrate(bitrate);

        //mac-adresses
        myMacAddress = myMacAddr;
        myId = getParentModule()->getParentModule()->getFullPath();
        //create frequency mappings
        frequency.insert(std::pair<int, double>(Channels::CRIT_SOL, 5.86e9));
        frequency.insert(std::pair<int, double>(Channels::SCH1, 5.87e9));
        frequency.insert(std::pair<int, double>(Channels::SCH2, 5.88e9));
        frequency.insert(std::pair<int, double>(Channels::CCH, 5.89e9));
        frequency.insert(std::pair<int, double>(Channels::SCH3, 5.90e9));
        frequency.insert(std::pair<int, double>(Channels::SCH4, 5.91e9));
        frequency.insert(std::pair<int, double>(Channels::HPPS, 5.92e9));

        //create two edca systems

        myEDCA[type_CCH] = new EDCA(type_CCH,par("queueSize").longValue());
        myEDCA[type_CCH]->myId = myId;
        myEDCA[type_CCH]->myId.append(" CCH");

        myEDCA[type_CCH]->createQueue(2,(((CWMIN_11P+1)/4)-1),(((CWMIN_11P +1)/2)-1),AC_VO);
        myEDCA[type_CCH]->createQueue(3,(((CWMIN_11P+1)/2)-1),CWMIN_11P,AC_VI);
        myEDCA[type_CCH]->createQueue(6,CWMIN_11P,CWMAX_11P,AC_BE);
        myEDCA[type_CCH]->createQueue(9,CWMIN_11P,CWMAX_11P,AC_BK);

        myEDCA[type_SCH] = new EDCA(type_SCH,par("queueSize").longValue());
        myEDCA[type_SCH]->myId = myId;
        myEDCA[type_SCH]->myId.append(" SCH");
        myEDCA[type_SCH]->createQueue(2,(((CWMIN_11P+1)/4)-1),(((CWMIN_11P +1)/2)-1),AC_VO);
        myEDCA[type_SCH]->createQueue(3,(((CWMIN_11P+1)/2)-1),CWMIN_11P,AC_VI);
        myEDCA[type_SCH]->createQueue(6,CWMIN_11P,CWMAX_11P,AC_BE);
        myEDCA[type_SCH]->createQueue(9,CWMIN_11P,CWMAX_11P,AC_BK);

        useSCH = par("useServiceChannel").boolValue();
        serviceCH = par("serviceChannel").longValue();
        mQueue = par("queue").longValue();
        if (useSCH) {
            //set the initial service channel
            switch (serviceCH) {
                case 1: mSCH = Channels::SCH1; break;
                case 2: mSCH = Channels::SCH2; break;
                case 3: mSCH = Channels::SCH3; break;
                case 4: mSCH = Channels::SCH4; break;
                default: opp_error("Service Channel must be between 1 and 4"); break;
            }
        }

        headerLength = par("headerLength");

        nextMacEvent = new cMessage("next Mac Event");

        if (useSCH) {
            // introduce a little asynchronization between radios, but no more than .3 milliseconds
            uint64_t currenTime = simTime().raw();
            uint64_t switchingTime = SWITCHING_INTERVAL_11P.raw();
            double timeToNextSwitch = (double)(switchingTime
                               - (currenTime % switchingTime)) / simTime().getScale();
            if ((currenTime / switchingTime) % 2 == 0) {
                setActiveChannel(type_CCH);
            }
            else {
                setActiveChannel(type_SCH);
            }

            // channel switching active
            nextChannelSwitch = new cMessage("Channel Switch");
            simtime_t offset = dblrand() * par("syncOffset").doubleValue();
            scheduleAt(simTime() + offset + timeToNextSwitch, nextChannelSwitch);
        }
        else {
            // no channel switching
            nextChannelSwitch = 0;
            setActiveChannel(type_CCH);
        }


        //stats
        statsReceivedPackets = 0;
        statsReceivedBroadcasts = 0;
        statsSentPackets = 0;
        statsTXRXLostPackets = 0;
        statsSNIRLostPackets = 0;
        statsDroppedPackets = 0;
        statsNumTooLittleTime = 0;
        statsNumInternalContention = 0;
        statsNumBackoff = 0;
        statsSlotsBackoff = 0;
        statsTotalBusyTime = 0;

        idleChannel = true;
        lastBusy = simTime();
        channelIdle(true);
    }
    if (stage == inet::INITSTAGE_LAST) {
        rsuMacAddress.setAddress(par("rsuMacAddress").stringValue());
    }
}

void Wave1609_4_OBU::handleSelfMsg(cMessage *msg)
{
    if (msg == nextChannelSwitch) {
        ASSERT(useSCH);

        scheduleAt(simTime() + SWITCHING_INTERVAL_11P, nextChannelSwitch);

        switch (activeChannel) {
            case type_CCH:
                DBG_MAC << "CCH --> SCH" << std::endl;
                channelBusySelf(false);
                setActiveChannel(type_SCH);
                channelIdle(true);
                phy11p->changeListeningFrequency(frequency[mSCH]);
                break;
            case type_SCH:
                DBG_MAC << "SCH --> CCH" << std::endl;
                channelBusySelf(false);
                setActiveChannel(type_CCH);
                channelIdle(true);
                phy11p->changeListeningFrequency(frequency[Channels::CCH]);
                break;
        }
        DBG_MAC << "Schedule next channel switch in 50ms." << std::endl;
        //schedule next channel switch in 50ms

    }
    else if (msg ==  nextMacEvent) {

        //we actually came to the point where we can send a packet
        DBG_MAC << "MacEvent received. Trying to send packet with priority " << lastAC << "; lastIdle=" << lastIdle << std::endl;
        channelBusySelf(true);
        //send the packet
        inet::ieee80211::Ieee80211DataOrMgmtFrame* frame = myEDCA[activeChannel]->initiateTransmit(lastIdle);
        inet::ieee80211::Ieee80211DataOrMgmtFrame* mac = frame->dup(); // duplicate message because it is deleted also when its not send

        enum PHY_MCS mcs;
        double txPower_mW;
        uint64_t datarate;
        PhyControlMessage *controlInfo = dynamic_cast<PhyControlMessage *>(mac->getControlInfo());
        if (controlInfo) {
            //if MCS is not specified, just use the default one
            mcs = (enum PHY_MCS)controlInfo->getMcs();
            if (mcs != MCS_DEFAULT) {
                datarate = getOfdmDatarate(mcs, BW_OFDM_10_MHZ);
            }
            else {
                datarate = bitrate;
            }
            //apply the same principle to tx power
            txPower_mW = controlInfo->getTxPower_mW();
            if (txPower_mW < 0) {
                txPower_mW = txPower;
            }
        }
        else {
            mcs = MCS_DEFAULT;
            txPower_mW = txPower;
            datarate = bitrate;
            DBG_MAC << "Sending with pre-configured settings." << std::endl;
        }

        simtime_t sendingDuration = RADIODELAY_11P + getFrameDuration(mac->getBitLength(), mcs);
        DBG_MAC << "Sending duration will be " << sendingDuration << std::endl;
        if ((!useSCH) || (timeLeftInSlot() > sendingDuration)) {
            if (useSCH) DBG_MAC << "Time in this slot left:  " << timeLeftInSlot() << std::endl;
            // give time for the radio to be in Tx state before transmitting
            phy->setRadioState(Radio::TX);
            double freq = (activeChannel == type_CCH) ? frequency[Channels::CCH] : frequency[mSCH];
            attachSignal(mac, simTime()+RADIODELAY_11P, freq, datarate, txPower_mW);
            DBG_MAC << "Sending frame to PhysicalLayer module. Frequency " << freq << " Priority " << lastAC << std::endl;
            MacToPhyControlInfo* phyInfo = dynamic_cast<MacToPhyControlInfo*>(mac->getControlInfo());
            assert(phyInfo);
            sendDelayed(mac, RADIODELAY_11P, lowerLayerOut);
            statsSentPackets++;
        }
        else {   //not enough time left now
            DBG_MAC << "Too little Time left. This packet cannot be send in this slot." << std::endl;
            statsNumTooLittleTime++;
            //revoke TXOP
            myEDCA[activeChannel]->revokeTxOPs();
            delete mac;
            channelIdle();
            //do nothing. contention will automatically start after channel switch
        }
    }
}

void Wave1609_4_OBU::handleUpperControl(cMessage* msg) {
    EV << "ERROR" << endl;
    cRuntimeError("Wave1609_4_OBU should not receive any upper control message. Not defined.");
}

void Wave1609_4_OBU::handleUpperMsg(cMessage* msg) {

    inet::ieee80211::Ieee80211DataFrameWithSNAP *frame = new inet::ieee80211::Ieee80211DataFrameWithSNAP(msg->getName());
    frame->setToDS(true);
    frame->setReceiverAddress(rsuMacAddress);
    frame->setTransmitterAddress(myMacAddress);
    EV << "Config: rsu=" << rsuMacAddress << " mMac=" << myMacAddress << endl;
    cObject *controlInfoPtr = msg->removeControlInfo();
    if (dynamic_cast<inet::Ieee802Ctrl *>(controlInfoPtr) != nullptr){
        inet::Ieee802Ctrl *ctrl = (inet::Ieee802Ctrl *) controlInfoPtr;
        ASSERT(!ctrl->getDest().isUnspecified());
        frame->setAddress3(ctrl->getDest());
        frame->setEtherType(ctrl->getEtherType());
    }
    cPacket *pk = PK(msg);
    frame->encapsulate(pk);
    t_access_category ac;
    switch (mQueue) {
        case 0: ac = AC_BK; break;
        case 1: ac = AC_BE; break;
        case 2: ac = AC_VI; break;
        case 3: ac = AC_VO; break;
        default:
            ac = AC_VO;
            cRuntimeError("Set a queue between 0 and 3");
            break;
    }
    lastAC = ac;
    DBG_MAC << "Received a message from upper layer for channel 174 in"
            << " Access Category (Priority):  "
            << ac << std::endl;
    // all packets are sent over service channel
    t_channel chan = type_SCH;

    int num = myEDCA[chan]->queuePacket(ac,frame);
    if (num == -1) {
        //packet was dropped in Mac
        statsDroppedPackets++;
        return;
    }
    //if this packet is not at the front of a new queue we dont have to reevaluate times
    DBG_MAC << "Queue state: Size=" << num << "; Channel state:  Is idle?: " << idleChannel << "; activeChannel " << activeChannel << " sendingChannel " << chan << endl;
    if (num == 1 && idleChannel == true && chan == activeChannel) {
        DBG_MAC << "Result: num == 1 && idleChannel == true && chan == activeChannel" << endl;
        simtime_t nextEvent = myEDCA[chan]->startContent(lastIdle,guardActive());
        if (nextEvent != -1) {
            if ((!useSCH) || (nextEvent <= nextChannelSwitch->getArrivalTime())) {
                if (nextMacEvent->isScheduled()) {
                    cancelEvent(nextMacEvent);
                }
                scheduleAt(nextEvent,nextMacEvent);
                DBG_MAC << "Updated nextMacEvent:" << nextMacEvent->getArrivalTime().raw() << std::endl;
            }
            else {
                DBG_MAC << "Too little time in this interval. Will not schedule nextMacEvent" << std::endl;
                //it is possible that this queue has an txop. we have to revoke it
                myEDCA[activeChannel]->revokeTxOPs();
                statsNumTooLittleTime++;
            }
        }
        else {
            cancelEvent(nextMacEvent);
        }
    }
    if (num == 1 && idleChannel == false && myEDCA[chan]->myQueues[ac].currentBackoff == 0 && chan == activeChannel) {
        DBG_MAC << "Result: num == 1 && idleChannel == false && myEDCA[chan]->myQueues[ac].currentBackoff == 0 && chan == activeChannel" << endl;
        myEDCA[chan]->backoff(ac);
    }
    delete controlInfoPtr;
}

void Wave1609_4_OBU::handleLowerControl(cMessage* msg) {
    EV << "HandleLowerControl in WAVE1609_4" << endl;
    assert(msg);
    if (msg->getKind() == MacToPhyInterface::TX_OVER) {
        DBG_MAC << "Successfully transmitted a frame on " << lastAC << std::endl;
        phy->setRadioState(Radio::RX);
        //message was sent
        //update EDCA queue. go into post-transmit backoff and set cwCur to cwMin
        myEDCA[activeChannel]->postTransmit(lastAC);
        //channel just turned idle.
        //don't set the chan to idle. the PHY layer decides, not us.

        if (guardActive()) {
            opp_error("We shouldnt have sent a packet in guard!");
        }
    }
    else if (msg->getKind() == Mac80211pToPhy11pInterface::CHANNEL_BUSY) {
        channelBusy();
    }
    else if (msg->getKind() == Mac80211pToPhy11pInterface::CHANNEL_IDLE) {
        channelIdle();
    }
    else if (msg->getKind() == Decider80211p::BITERROR || msg->getKind() == Decider80211p::COLLISION) {
        statsSNIRLostPackets++;
        DBG_MAC << "A packet was not received due to biterrors" << std::endl;
    }
    else if (msg->getKind() == Decider80211p::RECWHILESEND) {
        statsTXRXLostPackets++;
        DBG_MAC << "A packet was not received because we were sending while receiving" << std::endl;
    }
    else if (msg->getKind() == MacToPhyInterface::RADIO_SWITCHING_OVER) {
        DBG_MAC << "Phylayer said radio switching is done" << std::endl;
    }
    else if (msg->getKind() == BaseDecider::PACKET_DROPPED) {
        phy->setRadioState(Radio::RX);
        DBG_MAC << "Phylayer said packet was dropped" << std::endl;
    }
    else {
        DBG_MAC << "Invalid control message type (type=NOTHING) : name=" << msg->getName() << " modulesrc=" << msg->getSenderModule()->getFullPath() << "." << std::endl;
        cRuntimeError("Wave1609_4_OBU received control message from lower layer that has unknown type.");
    }

    if (msg->getKind() == Decider80211p::COLLISION) {
        emit(sigCollision, true);
    }

    delete msg;
}

void Wave1609_4_OBU::setActiveChannel(t_channel state) {
    activeChannel = state;
    assert(state == type_CCH || (useSCH && state == type_SCH));
}

void Wave1609_4_OBU::finish() {
    //clean up queues.

    for (std::map<t_channel,EDCA*>::iterator iter = myEDCA.begin(); iter != myEDCA.end(); iter++) {
        statsNumInternalContention += iter->second->statsNumInternalContention;
        statsNumBackoff += iter->second->statsNumBackoff;
        statsSlotsBackoff += iter->second->statsSlotsBackoff;
        iter->second->cleanUp();
        delete iter->second;
    }

    myEDCA.clear();

    if (nextMacEvent->isScheduled()) {
        cancelAndDelete(nextMacEvent);
    }
    else {
        delete nextMacEvent;
    }
    if (nextChannelSwitch && nextChannelSwitch->isScheduled())
        cancelAndDelete(nextChannelSwitch);

    //stats
    recordScalar("ReceivedUnicastPackets",statsReceivedPackets);
    recordScalar("ReceivedBroadcasts",statsReceivedBroadcasts);
    recordScalar("SentPackets",statsSentPackets);
    recordScalar("SNIRLostPackets",statsSNIRLostPackets);
    recordScalar("RXTXLostPackets",statsTXRXLostPackets);
    recordScalar("TotalLostPackets",statsSNIRLostPackets+statsTXRXLostPackets);
    recordScalar("DroppedPacketsInMac",statsDroppedPackets);
    recordScalar("TooLittleTime",statsNumTooLittleTime);
    recordScalar("TimesIntoBackoff",statsNumBackoff);
    recordScalar("SlotsBackoff",statsSlotsBackoff);
    recordScalar("NumInternalContention",statsNumInternalContention);
    recordScalar("totalBusyTime",statsTotalBusyTime.dbl());

}

void Wave1609_4_OBU::attachSignal(inet::ieee80211::Ieee80211DataOrMgmtFrame* mac, simtime_t startTime, double frequency, uint64_t datarate, double txPower_mW) {

    simtime_t duration = getFrameDuration(mac->getBitLength());

    Signal* s = createSignal(startTime, duration, txPower_mW, datarate, frequency);
    MacToPhyControlInfo* cinfo = new MacToPhyControlInfo(s);

    mac->setControlInfo(cinfo);
}

Signal* Wave1609_4_OBU::createSignal(simtime_t start, simtime_t length, double power, uint64_t bitrate, double frequency) {
    simtime_t end = start + length;
    //create signal with start at current simtime and passed length
    Signal* s = new Signal(start, length);

    //create and set tx power mapping
    ConstMapping* txPowerMapping = createSingleFrequencyMapping(start, end, frequency, 5.0e6, power);
    s->setTransmissionPower(txPowerMapping);

    Mapping* bitrateMapping = MappingUtils::createMapping(DimensionSet::timeDomain, Mapping::STEPS);

    Argument pos(start);
    bitrateMapping->setValue(pos, bitrate);

    pos.setTime(phyHeaderLength / bitrate);
    bitrateMapping->setValue(pos, bitrate);

    s->setBitrate(bitrateMapping);

    return s;
}

/* checks if guard is active */
bool Wave1609_4_OBU::guardActive() const {
    if (!useSCH) return false;
    if (simTime().dbl() - nextChannelSwitch->getSendingTime() <= GUARD_INTERVAL_11P)
        return true;
    return false;
}

/* returns the time until the guard is over */
simtime_t Wave1609_4_OBU::timeLeftTillGuardOver() const {
    ASSERT(useSCH);
    simtime_t sTime = simTime();
    if (sTime - nextChannelSwitch->getSendingTime() <= GUARD_INTERVAL_11P) {
        return GUARD_INTERVAL_11P
               - (sTime - nextChannelSwitch->getSendingTime());
    }
    else
        return 0;
}

/* returns the time left in this channel window */
simtime_t Wave1609_4_OBU::timeLeftInSlot() const {
    ASSERT(useSCH);
    return nextChannelSwitch->getArrivalTime() - simTime();
}

/* Will change the Service Channel on which the mac layer is listening and sending */
void Wave1609_4_OBU::changeServiceChannel(int cN) {
    ASSERT(useSCH);
    if (cN != Channels::SCH1 && cN != Channels::SCH2 && cN != Channels::SCH3 && cN != Channels::SCH4) {
        opp_error("This Service Channel doesnt exit: %d",cN);
    }

    mSCH = cN;

    if (activeChannel == type_SCH) {
        //change to new chan immediately if we are in a SCH slot,
        //otherwise it will switch to the new SCH upon next channel switch
        phy11p->changeListeningFrequency(frequency[mSCH]);
    }
}

void Wave1609_4_OBU::setTxPower(double txPower_mW) {
    txPower = txPower_mW;
}
void Wave1609_4_OBU::setMCS(enum PHY_MCS mcs) {
    ASSERT2(mcs != MCS_DEFAULT, "invalid MCS selected");
    bitrate = getOfdmDatarate(mcs, BW_OFDM_10_MHZ);
    setParametersForBitrate(bitrate);
}

void Wave1609_4_OBU::setCCAThreshold(double ccaThreshold_dBm) {
    phy11p->setCCAThreshold(ccaThreshold_dBm);
}

void Wave1609_4_OBU::handleLowerMsg(cMessage* msg) {
    DBG_MAC << "Received frame from lower layer." << endl;
    inet::ieee80211::Ieee80211DataOrMgmtFrame* frame = check_and_cast<inet::ieee80211::Ieee80211DataOrMgmtFrame *>(msg);
    assert(frame);
    cPacket *payload = frame->decapsulate();

    //pass information about received frame to the upper layers
//    DeciderResult80211 *macRes = dynamic_cast<DeciderResult80211 *>(PhyToMacControlInfo::getDeciderResult(msg));
//    ASSERT(macRes);
//    DeciderResult80211 *res = new DeciderResult80211(*macRes);

    inet::Ieee802Ctrl *ctrl = new inet::Ieee802Ctrl();
    ctrl->setSrc(inet::MACAddress(frame->getAddress3()));
    ctrl->setDest(inet::MACAddress(frame->getReceiverAddress()));
    inet::ieee80211::Ieee80211DataFrameWithSNAP *frameWithSNAP = dynamic_cast<inet::ieee80211::Ieee80211DataFrameWithSNAP *>(frame);
    if (frameWithSNAP)
        ctrl->setEtherType(frameWithSNAP->getEtherType());
    payload->setControlInfo(ctrl);

    inet::MACAddress dest = frame->getReceiverAddress();

    DBG_MAC << "Frame name= " << frame->getName()
            << ", myState=" << " src=" << frame->getTransmitterAddress()
            << " dst=" << frame->getReceiverAddress() << " myAddr="
            << myMacAddress << std::endl;

    if (frame->getReceiverAddress() == myMacAddress) {
        DBG_MAC << "Received a data packet addressed to me." << std::endl;
        statsReceivedPackets++;
        sendUp(payload);
    }
    else if (dest == inet::MACAddress::BROADCAST_ADDRESS) {
        statsReceivedBroadcasts++;
        sendUp(payload);
    }
    else {
        DBG_MAC << "Packet not for me, deleting..." << std::endl;
        delete payload;
    }
    delete frame;
}
// hier weiter und mach upperMSG vollenden
int Wave1609_4_OBU::EDCA::queuePacket(t_access_category ac,inet::ieee80211::Ieee80211DataOrMgmtFrame* msg) {

    if (maxQueueSize && myQueues[ac].queue.size() >= maxQueueSize) {
        delete msg;
        return -1;
    }
    myQueues[ac].queue.push(msg);
    return myQueues[ac].queue.size();
}

int Wave1609_4_OBU::EDCA::createQueue(int aifsn, int cwMin, int cwMax,t_access_category ac) {

    if (myQueues.find(ac) != myQueues.end()) {
        opp_error("You can only add one queue per Access Category per EDCA subsystem");
    }

    EDCAQueue newQueue(aifsn,cwMin,cwMax,ac);
    myQueues[ac] = newQueue;

    return ++numQueues;
}

Wave1609_4_OBU::t_access_category Wave1609_4_OBU::mapPriority(int prio) {
    //dummy mapping function
    switch (prio) {
        case 0: return AC_BK;
        case 1: return AC_BE;
        case 2: return AC_VI;
        case 3: return AC_VO;
        default: opp_error("MacLayer received a packet with unknown priority"); break;
    }
    return AC_VO;
}

inet::ieee80211::Ieee80211DataOrMgmtFrame* Wave1609_4_OBU::EDCA::initiateTransmit(simtime_t lastIdle) {

    //iterate through the queues to return the packet we want to send
    inet::ieee80211::Ieee80211DataOrMgmtFrame* pktToSend = NULL;

    simtime_t idleTime = simTime() - lastIdle;

    DBG_MAC << "Initiating transmit at " << simTime() << ". I've been idle since " << idleTime << std::endl;

    for (std::map<t_access_category, EDCAQueue>::iterator iter = myQueues.begin(); iter != myQueues.end(); iter++) {
        if (iter->second.queue.size() != 0) {
            if (idleTime >= iter->second.aifsn* SLOTLENGTH_11P + SIFS_11P && iter->second.txOP == true) {

                DBG_MAC << "Queue " << iter->first << " is ready to send!" << std::endl;

                iter->second.txOP = false;
                //this queue is ready to send
                if (pktToSend == NULL) {
                    pktToSend = iter->second.queue.front();
                }
                else {
                    //there was already another packet ready. we have to go increase cw and go into backoff. It's called internal contention and its wonderful

                    statsNumInternalContention++;
                    iter->second.cwCur = std::min(iter->second.cwMax,iter->second.cwCur*2);
                    iter->second.currentBackoff = intuniform(0,iter->second.cwCur);
                    DBG_MAC << "Internal contention for queue " << iter->first  << " : "<< iter->second.currentBackoff << ". Increase cwCur to " << iter->second.cwCur << std::endl;
                }
            }
        }
    }

    if (pktToSend == NULL) {
        opp_error("No packet was ready");
    }
    return pktToSend;
}

simtime_t Wave1609_4_OBU::EDCA::startContent(simtime_t idleSince,bool guardActive) {

    DBG_MAC << "Restarting contention." << std::endl;
    simtime_t nextEvent = -1;
    simtime_t idleTime = SimTime().setRaw(std::max((int64_t)0,(simTime() - idleSince).raw()));;
    lastStart = idleSince;
    DBG_MAC << "Channel is already idle for:" << idleTime << " since " << idleSince << std::endl;
    //this returns the nearest possible event in this EDCA subsystem after a busy channel
    for (std::map<t_access_category, EDCAQueue>::iterator iter = myQueues.begin(); iter != myQueues.end(); iter++) {
        if (iter->second.queue.size() != 0) {
            /* 1609_4 says that when attempting to send (backoff == 0) when guard is active, a random backoff is invoked */
            if (guardActive == true && iter->second.currentBackoff == 0) {
                //cw is not increased
                iter->second.currentBackoff = intuniform(0,iter->second.cwCur);
                statsNumBackoff++;
            }
            simtime_t DIFS = iter->second.aifsn * SLOTLENGTH_11P + SIFS_11P;
            //the next possible time to send can be in the past if the channel was idle for a long time, meaning we COULD have sent earlier if we had a packet
            simtime_t possibleNextEvent = DIFS + iter->second.currentBackoff * SLOTLENGTH_11P;
            DBG_MAC << "Waiting Time for Queue " << iter->first <<  ":" << possibleNextEvent << "=" << iter->second.aifsn << " * "  << SLOTLENGTH_11P << " + " << SIFS_11P << "+" << iter->second.currentBackoff << "*" << SLOTLENGTH_11P << "; Idle time: " << idleTime << std::endl;
            if (idleTime > possibleNextEvent) {
                DBG_MAC << "Could have already send if we had it earlier" << std::endl;
                //we could have already sent. round up to next boundary
                simtime_t base = idleSince + DIFS;
                possibleNextEvent =  simTime() - simtime_t().setRaw((simTime() - base).raw() % SLOTLENGTH_11P.raw()) + SLOTLENGTH_11P;
            }
            else {
                //we are gonna send in the future
                DBG_MAC << "Sending in the future" << std::endl;
                possibleNextEvent =  idleSince + possibleNextEvent;
            }
            nextEvent == -1? nextEvent =  possibleNextEvent : nextEvent = std::min(nextEvent,possibleNextEvent);
        }
    }
    return nextEvent;
}

void Wave1609_4_OBU::EDCA::stopContent(bool allowBackoff, bool generateTxOp) {
    //update all Queues
    DBG_MAC << "Stopping Contention at " << simTime().raw() << std::endl;
    simtime_t passedTime = simTime() - lastStart;
    DBG_MAC << "Channel was idle for " << passedTime << std::endl;
    lastStart = -1; //indicate that there was no last start
    for (std::map<t_access_category, EDCAQueue>::iterator iter = myQueues.begin(); iter != myQueues.end(); iter++) {
        if (iter->second.currentBackoff != 0 || iter->second.queue.size() != 0) {
            //check how many slots we already waited until the chan became busy

            int oldBackoff = iter->second.currentBackoff;

            std::string info;
            if (passedTime < iter->second.aifsn * SLOTLENGTH_11P + SIFS_11P) {
                //we didnt even make it one DIFS :(
                info.append(" No DIFS");
            }
            else {
                //decrease the backoff by one because we made it longer than one DIFS
                iter->second.currentBackoff--;

                //check how many slots we waited after the first DIFS
                int passedSlots = (int)((passedTime - SimTime(iter->second.aifsn * SLOTLENGTH_11P + SIFS_11P)) / SLOTLENGTH_11P);

                DBG_MAC << "Passed slots after DIFS: " << passedSlots << std::endl;


                if (iter->second.queue.size() == 0) {
                    //this can be below 0 because of post transmit backoff -> backoff on empty queues will not generate macevents,
                    //we dont want to generate a txOP for empty queues
                    iter->second.currentBackoff -= std::min(iter->second.currentBackoff,passedSlots);
                    info.append(" PostCommit Over");
                }
                else {
                    iter->second.currentBackoff -= passedSlots;
                    if (iter->second.currentBackoff <= -1) {
                        if (generateTxOp) {
                            iter->second.txOP = true; info.append(" TXOP");
                        }
                        //else: this packet couldnt be sent because there was too little time. we could have generated a txop, but the channel switched
                        iter->second.currentBackoff = 0;
                    }

                }
            }
            DBG_MAC << "Updating backoff for Queue " << iter->first << ": " << oldBackoff << " -> " << iter->second.currentBackoff << info <<std::endl;
        }
    }
}
void Wave1609_4_OBU::EDCA::backoff(t_access_category ac) {
    myQueues[ac].currentBackoff = intuniform(0,myQueues[ac].cwCur);
    statsSlotsBackoff += myQueues[ac].currentBackoff;
    statsNumBackoff++;
    DBG_MAC << "Going into Backoff because channel was busy when new packet arrived from upperLayer" << std::endl;
}

void Wave1609_4_OBU::EDCA::postTransmit(t_access_category ac) {
    DBG_MAC << "Removing last frame from queue " << ac << endl;
    inet::ieee80211::Ieee80211DataOrMgmtFrame* frame = myQueues[ac].queue.front();
    assert(frame);
    delete frame;
    // Erasing the front element from queue
    myQueues[ac].queue.pop();
    if(!myQueues[ac].queue.empty())
        DBG_MAC << "Further elements in queue contained" << endl;
    else
        DBG_MAC << "No further elements in queue exists" << endl;
    myQueues[ac].cwCur = myQueues[ac].cwMin;
    //post transmit backoff
    myQueues[ac].currentBackoff = intuniform(0,myQueues[ac].cwCur);
    statsSlotsBackoff += myQueues[ac].currentBackoff;
    statsNumBackoff++;
    DBG_MAC << "Queue " << ac << " will go into post-transmit backoff for " << myQueues[ac].currentBackoff << " slots" << std::endl;
}

void Wave1609_4_OBU::EDCA::cleanUp() {
    for (std::map<t_access_category, EDCAQueue>::iterator iter = myQueues.begin(); iter != myQueues.end(); iter++) {
        while (iter->second.queue.size() != 0) {
            delete iter->second.queue.front();
            iter->second.queue.pop();
        }
    }
    myQueues.clear();
}

void Wave1609_4_OBU::EDCA::revokeTxOPs() {
    for (std::map<t_access_category, EDCAQueue>::iterator iter = myQueues.begin(); iter != myQueues.end(); iter++) {
        if (iter->second.txOP == true) {
            iter->second.txOP = false;
            iter->second.currentBackoff = 0;
        }
    }
}

void Wave1609_4_OBU::channelBusySelf(bool generateTxOp) {

    //the channel turned busy because we're sending. we don't want our queues to go into backoff
    //internal contention is already handled in initiateTransmission

    if (!idleChannel) return;
    idleChannel = false;
    DBG_MAC << "Channel turned busy: Switch or Self-Send" << std::endl;

    lastBusy = simTime();

    //channel turned busy
    if (nextMacEvent->isScheduled() == true) {
        cancelEvent(nextMacEvent);
    }
    else {
        //the edca subsystem was not doing anything anyway.
    }
    myEDCA[activeChannel]->stopContent(false, generateTxOp);

    emit(sigChannelBusy, true);
}

void Wave1609_4_OBU::channelBusy() {

    if (!idleChannel) return;

    //the channel turned busy because someone else is sending
    idleChannel = false;
    DBG_MAC << "Channel turned busy: External sender" << std::endl;
    lastBusy = simTime();

    //channel turned busy
    if (nextMacEvent->isScheduled() == true) {
        cancelEvent(nextMacEvent);
    }
    else {
        //the edca subsystem was not doing anything anyway.
    }
    myEDCA[activeChannel]->stopContent(true,false);

    emit(sigChannelBusy, true);
}

void Wave1609_4_OBU::channelIdle(bool afterSwitch) {

    DBG_MAC << "Channel turned idle: Switch: " << afterSwitch << std::endl;

    if (nextMacEvent->isScheduled() == true) {
        //this rare case can happen when another node's time has such a big offset that the node sent a packet although we already changed the channel
        //the workaround is not trivial and requires a lot of changes to the phy and decider
        DBG_MAC << "Channel turned idle but contention timer was scheduled! This case is not handled." << endl;
        return;
        //opp_error("channel turned idle but contention timer was scheduled!");
    }

    idleChannel = true;

    simtime_t delay = 0;

    //account for 1609.4 guards
    if (afterSwitch) {
        //  delay = GUARD_INTERVAL_11P;
    }
    if (useSCH) {
        delay += timeLeftTillGuardOver();
    }

    //channel turned idle! lets start contention!
    lastIdle = delay + simTime();
    statsTotalBusyTime += simTime() - lastBusy;

    //get next Event from current EDCA subsystem
    simtime_t nextEvent = myEDCA[activeChannel]->startContent(lastIdle,guardActive());
    if (nextEvent != -1) {
        if ((!useSCH) || (nextEvent < nextChannelSwitch->getArrivalTime())) {
            scheduleAt(nextEvent,nextMacEvent);
            DBG_MAC << "next Event is at " << nextMacEvent->getArrivalTime().raw() << std::endl;
        }
        else {
            DBG_MAC << "Too little time in this interval. will not schedule macEvent" << std::endl;
            statsNumTooLittleTime++;
            myEDCA[activeChannel]->revokeTxOPs();
        }
    }
    else {
        DBG_MAC << "I don't have any new events in this EDCA sub system" << std::endl;
    }

    emit(sigChannelBusy, false);

}

void Wave1609_4_OBU::setParametersForBitrate(uint64_t bitrate) {
    for (unsigned int i = 0; i < NUM_BITRATES_80211P; i++) {
        if (bitrate == BITRATES_80211P[i]) {
            n_dbps = N_DBPS_80211P[i];
            return;
        }
    }
    opp_error("Chosen Bitrate is not valid for 802.11p: Valid rates are: 3Mbps, 4.5Mbps, 6Mbps, 9Mbps, 12Mbps, 18Mbps, 24Mbps and 27Mbps. Please adjust your omnetpp.ini file accordingly.");
}


simtime_t Wave1609_4_OBU::getFrameDuration(int payloadLengthBits, enum PHY_MCS mcs) const {
    simtime_t duration;
    if (mcs == MCS_DEFAULT) {
        // calculate frame duration according to Equation (17-29) of the IEEE 802.11-2007 standard
        duration = PHY_HDR_PREAMBLE_DURATION + PHY_HDR_PLCPSIGNAL_DURATION + T_SYM_80211P * ceil( (16 + payloadLengthBits + 6)/(n_dbps) );
    }
    else {
        uint32_t ndbps = getNDBPS(mcs);
        duration = PHY_HDR_PREAMBLE_DURATION + PHY_HDR_PLCPSIGNAL_DURATION + T_SYM_80211P * ceil( (16 + payloadLengthBits + 6)/(ndbps) );
    }

    return duration;
}
