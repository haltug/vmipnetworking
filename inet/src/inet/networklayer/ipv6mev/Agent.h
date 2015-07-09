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

#ifndef __INET_AGENT_H_
#define __INET_AGENT_H_

#include <map>
#include <vector>
#include "inet/common/INETDefs.h"
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/networklayer/ipv6mev/IdentificationHeader.h"

namespace inet {

//========== Timer key value ==========
#define TIMERKEY_CA_INIT  0 // ca init key for timer module

//========== Timer type
#define TIMERTYPE_INIT_MSG    50
#define TIMERTYPE_SESSION_REQ 51
#define TIMERTYPE_SEQ_UPDATE  52
#define TIMERTYPE_LOC_UPDATE  53

//========== Message type in handleMessage() ==========
#define MSG_CA_INIT       100 // ca init msg type for handling
#define MSG_START_TIME    101

//========== Retransmission time of messages ==========
#define TIME_CA_INIT      1 // retransmission time of ca init in sec

//========== Header SIZE ===========
#define FD_MIN_HEADER_SIZE    16
#define FD_IP_ADDR_SIZE       16
#define FD_REDIRECT_ADDR_REQ  16
#define FD_REDIRECT_ADDR_RESP 32
#define FD_LOCATION_SIZE      32
//#define USER_ID_SIZE          16 // Mobile ID length in char

class InterfaceEntry;
class IInterfaceTable;
class IPv6ControlInfo;
/**
 * TODO - Generated class
 */
class INET_API Agent : public cSimpleModule
{
  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    cMessage *timeoutMsg = nullptr;
    simtime_t startTime;
  public:
    Agent();
    virtual ~Agent();

    enum AgentState {
        UNASSOCIATED = 0,
        INITIALIZE   = 1,
        REGISTERED   = 2
    };
    AgentState state;

    IInterfaceTable *ift = nullptr; // for recognizing changes etc
//    AddressManagement *am = nullptr; // for sliding address mechanism
    IPv6Address CA_Address; // ip address of ca
    uint64 mobileId = 0;
    typedef std::vector<uint64>  MobileIdList;
    MobileIdList mobileIdList;
    bool isMA;
    bool isCA;
    bool isDA;

    typedef std::map<int, IPv6Address> InterfaceToIPv6AddressList; // not sure if type is set correct but it's stores association of dest addr of cn to data agent addr
    InterfaceToIPv6AddressList interfaceToIPv6AddressList;

    typedef std::map<IPv6Address, IPv6Address> DirectAddressList; // IPv6address should be replaced with DataAgent <cn,da>
    DirectAddressList directAddressList;

//    virtual void processLowerLayerMessage(cMessage *msg);
//    virtual void processUpperLayerMessage(cMessage *msg);

    void createCAInitialization();
    void sendCAInitialization(cMessage *msg); // send initialization message to CA
    void sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr = IPv6Address::UNSPECIFIED_ADDRESS, int interfaceId = -1, simtime_t sendTime = 0); // resend after timer expired
    void processCAMessages(ControlAgentHeader *agentHdr, IPv6ControlInfo *ipCtrlInfo);
    void processMAMessages(MobileAgentHeader *agentHdr, IPv6ControlInfo *ipCtrlInfo);

//============================================= Timer configuration ===========================
    class ExpiryTimer {
    public:
        cMessage *timer;
        virtual ~ExpiryTimer() {};
        IPv6Address dest;
        simtime_t ackTimeout;
        simtime_t nextScheduledTime;
        InterfaceEntry *ie; // store over which entry it should be send
    };
    class TimerKey {
    public:
        int type;
        int interfaceID;
        IPv6Address dest;
        TimerKey(IPv6Address _dest, int _interfaceID, int _type) {
            dest=_dest;
            interfaceID=_interfaceID;
            type=_type;
        }
        bool operator<(const TimerKey& b) const {
            if (type == b.type)
                return interfaceID == b.interfaceID ? dest < b.dest : interfaceID < b.interfaceID;
            else
                return type < b.type;
        }
    };
    typedef std::map<TimerKey,ExpiryTimer *> ExpiredTimerList;
    ExpiredTimerList expiredTimerList;

    // TimerType 50
    class InitMessageTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    // TimerType 51
    class SessionRequestMessageTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    // TimerType 52
    class SequenceUpdateMessageTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    // TimerType 53
    class LocationUpdateMessageTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    ExpiryTimer *getExpiryTimer(TimerKey& key, int timerType);
    bool pendingExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType);
    bool cancelExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType);
    void cancelExpiryTimers();
//============================================= Timer configuration ===========================

};

} //namespace

#endif
