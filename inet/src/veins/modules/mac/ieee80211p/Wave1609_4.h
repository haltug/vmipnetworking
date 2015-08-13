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

#ifndef __INET_WAVE1609_4_H_
#define __INET_WAVE1609_4_H_

#include <assert.h>
#include <omnetpp.h>
#include <queue>
#include <stdint.h>
#include "veins/base/modules/BaseLayer.h"
#include "veins/base/phyLayer/MacToPhyControlInfo.h"
#include "veins/modules/phy/PhyLayer80211p.h"
#include "veins/modules/mac/ieee80211p/WaveAppToMac1609_4Interface.h"
#include "veins/modules/utility/Consts80211p.h"
#include "veins/base/utils/FindModule.h"
#include "veins/modules/messages/Mac80211Pkt_m.h"
#include "veins/modules/messages/WaveShortMessage_m.h"
#include "veins/base/modules/BaseMacLayer.h"
#include "veins/modules/utility/ConstsPhy.h"

#include "inet/networklayer/ipv6/IPv6Datagram.h"
#include "inet/linklayer/common/MACAddress.h"
#include "inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h"
/**
 * TODO - Generated class
 */
class INET_API Wave1609_4 : public BaseMacLayer, public WaveAppToMac1609_4Interface
{
public:

    enum t_access_category {
        AC_BK = 0,
        AC_BE = 1,
        AC_VI = 2,
        AC_VO = 3
    };

    class EDCA {
        public:
            class EDCAQueue {
                public:

                    std::queue<inet::ieee80211::Ieee80211DataOrMgmtFrame*> queue;
                    int aifsn; //number of aifs slots for this queue
                    int cwMin; //minimum contention window
                    int cwMax; //maximum contention size
                    int cwCur; //current contention window
                    int currentBackoff; //current Backoff value for this queue
                    bool txOP;

                    EDCAQueue() {   };
                    EDCAQueue(int aifsn,int cwMin, int cwMax, t_access_category ac):aifsn(aifsn),cwMin(cwMin),cwMax(cwMax),cwCur(cwMin),currentBackoff(0),txOP(false) {
                    };
            };

            EDCA(t_channel channelType,int maxQueueLength = 0):numQueues(0),maxQueueSize(maxQueueLength),channelType(channelType) {
                statsNumInternalContention = 0;
                statsNumBackoff = 0;
                statsSlotsBackoff = 0;
            };
            /*
             * Currently you have to call createQueue in the right order. First Call is priority 0, second 1 and so on...
             */
            int createQueue(int aifsn, int cwMin, int cwMax,t_access_category);
            int queuePacket(t_access_category AC,inet::ieee80211::Ieee80211DataOrMgmtFrame* cmsg);
            void backoff(t_access_category ac);
            simtime_t startContent(simtime_t idleSince, bool guardActive);
            void stopContent(bool allowBackoff, bool generateTxOp);
            void postTransmit(t_access_category);
            void revokeTxOPs();

            void cleanUp();

            /** @brief return the next packet to send, send all lower Queues into backoff */
            inet::ieee80211::Ieee80211DataOrMgmtFrame* initiateTransmit(simtime_t idleSince);

        public:
            std::map<t_access_category,EDCAQueue> myQueues;
            int numQueues;
            uint32_t maxQueueSize;
            simtime_t lastStart; //when we started the last contention;
            t_channel channelType;

            /** @brief Stats */
            long statsNumInternalContention;
            long statsNumBackoff;
            long statsSlotsBackoff;

            /** @brief Id for debug messages */
            std::string myId;
    };

public:
    ~Wave1609_4() { };

    void changeServiceChannel(int channelNumber);

    /**
     * @brief Change the default tx power the NIC card is using
     *
     * @param txPower_mW the tx power to be set in mW
     */
    void setTxPower(double txPower_mW);

    /**
     * @brief Change the default MCS the NIC card is using
     *
     * @param mcs the default modulation and coding scheme
     * to use
     */
    void setMCS(enum PHY_MCS mcs);

    /**
     * @brief Change the phy layer carrier sense threshold.
     *
     * @param ccaThreshold_dBm the cca threshold in dBm
     */
    void setCCAThreshold(double ccaThreshold_dBm);

protected:
    /** @brief States of the channel selecting operation.*/

protected:
    /** @brief Initialization of the module and some variables.*/
    virtual void initialize(int);

    /** @brief Delete all dynamically allocated objects of the module.*/
    virtual void finish();

    /** @brief Handle messages from lower layer.*/
    virtual void handleLowerMsg(cMessage*);

    /** @brief Handle messages from upper layer.*/
    virtual void handleUpperMsg(cMessage*);

    /** @brief Handle control messages from upper layer.*/
    virtual void handleUpperControl(cMessage* msg);


    /** @brief Handle self messages such as timers.*/
    virtual void handleSelfMsg(cMessage*);

    /** @brief Handle control messages from lower layer.*/
    virtual void handleLowerControl(cMessage* msg);

    /** @brief Set a state for the channel selecting operation.*/
    void setActiveChannel(t_channel state);

    simtime_t timeLeftInSlot() const;
    simtime_t timeLeftTillGuardOver() const;

    bool guardActive() const;

    void attachSignal(inet::ieee80211::Ieee80211DataOrMgmtFrame* mac, simtime_t startTime, double frequency, uint64_t datarate, double txPower_mW);
    Signal* createSignal(simtime_t start, simtime_t length, double power, uint64_t bitrate, double frequency);

    /** @brief maps a application layer priority (up) to an EDCA access category. */
    t_access_category mapPriority(int prio);

    void channelBusy();
    void channelBusySelf(bool generateTxOp);
    void channelIdle(bool afterSwitch = false);

    void setParametersForBitrate(uint64_t bitrate);

    simtime_t getFrameDuration(int payloadLengthBits, enum PHY_MCS mcs = MCS_DEFAULT) const;

protected:
    /** @brief Self message to indicate that the current channel shall be switched.*/
    cMessage* nextChannelSwitch;

    /** @brief Self message to wake up at next MacEvent */
    cMessage* nextMacEvent;

    /** @brief Last time the channel went idle */
    simtime_t lastIdle;
    simtime_t lastBusy;

    /** @brief Current state of the channel selecting operation.*/
    t_channel activeChannel;

    /** @brief access category of last sent packet */
    t_access_category lastAC;

    /** @brief Stores the frequencies in Hz that are associated to the channel numbers.*/
    std::map<int,double> frequency;

    int headerLength;

    bool useSCH;
    int serviceCH;
    int mQueue;
    int mSCH; // 174 176 180 182

    std::map<t_channel,EDCA*> myEDCA;

    bool idleChannel;

    /** @brief stats */
    long statsReceivedPackets;
    long statsReceivedBroadcasts;
    long statsSentPackets;
    long statsTXRXLostPackets;
    long statsSNIRLostPackets;
    long statsDroppedPackets;
    long statsNumTooLittleTime;
    long statsNumInternalContention;
    long statsNumBackoff;
    long statsSlotsBackoff;
    simtime_t statsTotalBusyTime;

    /** @brief This MAC layers MAC address.*/
    inet::MACAddress myMacAddress;
    inet::MACAddress rsuMacAddress;
    /** @brief The power (in mW) to transmit with.*/
    double txPower;

    /** @brief the bit rate at which we transmit */
    uint64_t bitrate;

    /** @brief N_DBPS, derived from bitrate, for frame length calculation */
    double n_dbps;

    /** @brief Id for debug messages */
    std::string myId;

    Mac80211pToPhy11pInterface* phy11p;

    //tell to anybody which is interested when the channel turns busy or idle
    simsignal_t sigChannelBusy;
    //tell to anybody which is interested when a collision occurred
    simsignal_t sigCollision;
};


#endif
