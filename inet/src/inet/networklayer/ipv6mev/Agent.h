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
#include "inet/networklayer/ipv6mev/AddressManagement.h"
#include "inet/networklayer/common/InterfaceEntry.h"

namespace inet {

//========== Timer key value ==========
#define TIMERKEY_SESSION_INIT    0 // ca init key for timer module
#define TIMERKEY_SEQNO_INIT      1
#define TIMERKEY_SEQ_UPDATE      2
#define TIMERKEY_LOC_UPDATE      3
#define TIMERKEY_IF_DOWN         4
#define TIMERKEY_IF_UP           5
#define TIMERKEY_FLOW_REQ        6
#define TIMERKEY_MA_INIT         7
#define TIMERKEY_SEQ_UPDATE_NOT  8

//========== Timer type
#define TIMERTYPE_SESSION_INIT  50
#define TIMERTYPE_SEQNO_INIT    51
#define TIMERTYPE_SEQ_UPDATE    52
#define TIMERTYPE_LOC_UPDATE    53
#define TIMERTYPE_IF_DOWN       54
#define TIMERTYPE_IF_UP         55
#define TIMERTYPE_FLOW_REQ      56
#define TIMERTYPE_MA_INIT       57
#define TIMERTYPE_SEQ_UPDATE_NOT 58

//========== Message type in handleMessage() ==========
#define MSG_START_TIME          100
#define MSG_SESSION_INIT        101 // ca init msg type for handling
#define MSG_SEQNO_INIT          102
#define MSG_SEQ_UPDATE          103
#define MSG_SEQ_UPDATE_DELAYED  104
#define MSG_IF_DOWN             105
#define MSG_IF_UP               106
#define MSG_LOC_UPDATE          107
#define MSG_LOC_UPDATE_DELAYED  108
#define MSG_FLOW_REQ            109
#define MSG_UDP_RETRANSMIT      110
#define MSG_MA_INIT             111
#define MSG_MA_INIT_DELAY       112
#define MSG_AGENT_UPDATE        113
#define MSG_SEQ_UPDATE_NOTIFY   114

//========== Retransmission time of messages ==========
#define TIMEOUT_SESSION_INIT    1 // retransmission time of ca init in sec
#define TIMEOUT_SEQNO_INIT      1
#define TIMEOUT_SEQ_UPDATE      1
#define TIMEOUT_LOC_UPDATE      1
#define TIMEDELAY_IF_DOWN       3   // delay of ip msg handler
#define TIMEDELAY_IF_UP         0
#define TIMEDELAY_FLOW_REQ      1 // unit is [s]
#define TIMEDELAY_MA_INIT       1 // unit is [s]
#define MAX_PKT_LIFETIME        30 // specifies retransmission attempts until pkt is discarded for udp tcp
//#define TIMEOUT_FLOW_REQ        1

//========== Header SIZE ===========
#define SIZE_AGENT_HEADER        16
#define SIZE_ADDING_ADDR_TO_HDR  16
#define SIZE_REDIRECT_ADDR_REQU  16
#define SIZE_REDIRECT_ADDR_RESP  32
#define SIZE_LOCATION_UPDATE     32
#define USER_ID_SIZE             16 // Mobile ID length in char, not used

class IInterfaceTable;
class IPv6ControlInfo;
class IPv6Datagram;
class AddressManagement;
/**
 * TODO - Generated class
 */
class INET_API Agent : public cSimpleModule, public cListener
{
  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    simtime_t startTime;
    bool isMA;
    bool isCA;
    bool isDA;
  public:
    Agent();
    virtual ~Agent();

    enum AgentState {
        UNASSOCIATED = 0,
        INITIALIZING = 1,
        ASSOCIATED   = 2
    };
    AgentState sessionState; // state of MA at beginning
    AgentState seqnoState; // state of MA for seq init

    IInterfaceTable *ift = nullptr; // for recognizing changes etc
    cModule *interfaceNotifier = nullptr; // listens for changes in interfacetable
    IPv6Address CA_Address; // ip address of ca
    AddressManagement am;
    uint64 mobileId = 0;
    std::vector<uint64>  mobileIdList; // lists all id of mobile nodes
    std::vector<IPv6Address> nodeAddressList; // lists all agents

    class InterfaceUnit { // represents the entry of addressTable
    public:
        bool active;
        int priority;
        IPv6Address careOfAddress;
    };
    typedef std::map<int, InterfaceUnit *> AddressTable; // represents the address table
    AddressTable addressTable;

    struct FlowTuple {
        IPv6Address destAddress;
        int destPort;
        int sourcePort;
        int lifetime;
        short protocol;
        bool operator<(const FlowTuple& b) const {
            return (destAddress != b.destAddress) ? (destPort != b.destPort) : (sourcePort != b.sourcePort);
        }
    };

    enum FlowState {
        UNREGISTERED = 0,
        REGISTERING  = 1,
        REGISTERED   = 2
    };

    struct FlowUnit {
        FlowState state;
        bool active;
        bool cacheAddress; // specifiy if address should be cached
        bool cachingActive;   // presents if address has been cached by data agent
        bool locationUpdate;
        bool loadSharing;

        IPv6Address dataAgent; // defines the address of data agent
    };

    typedef std::map<FlowTuple, FlowUnit> FlowTable; // IPv6address should be replaced with DataAgent <cn,da>
    FlowTable flowTable;

    typedef std::map<IPv6Address,IPv6Address> AddressAssociation; // destinationAddress -> agentAddress
    AddressAssociation addressAssociation;

    FlowUnit *getFlowUnit(FlowTuple &tuple);
    FlowUnit *getFlowSetting(FlowTuple &tuple);
    bool isAddressAssociated(IPv6Address &dest);
    IPv6Address *getAssociatedAddress(IPv6Address &dest);

    void sendToLowerLayer(cMessage *msg, const IPv6Address& destAddr, const IPv6Address& srcAddr = IPv6Address::UNSPECIFIED_ADDRESS, int interfaceId = -1, simtime_t sendTime = 0); // resend after timer expired

    void createSessionInit();
    void sendSessionInit(cMessage *msg); // send initialization message to CA
    void createSequenceInit();
    void sendSequenceInit(cMessage *msg);
    void createSequenceUpdate();
    void sendSequenceUpdate(cMessage *msg);
    void createFlowRequest(FlowTuple &tuple);
    void sendFlowRequest(cMessage *msg);

    // CA function
    void createAgentInit(uint64 mobileId); // used by CA
    void sendAgentInit(cMessage *msg); // used by CA to init DA
    void createAgentUpdate(uint64 mobileId); // used by CA to update all its specific data agents
    void sendAgentUpdate(cMessage *msg);
    void sendSequenceUpdateAck(uint64 mobileId); // confirm to MA its new
    void sendSessionInitResponse(IPv6Address dest, IPv6Address source);
    void sendSequenceInitResponse(IPv6Address dest, IPv6Address source, uint64 mobileId);
    void sendSequenceUpdateResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId);
    void sendFlowRequestResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId, IPv6Address nodeAddr, IPv6Address agentAddr);

    // DA function
    void createSequenceUpdateNotificaiton(uint64 mobileId);
    void sendSequenceUpdateNotification(cMessage *msg); // used by DA to notify CA of changes
    void sendCorrespondentNodeMessage(cMessage *msg); // forwards message to CN
    void sendMobileAgentMessage(); // forwards messages to MA
    void sendAgentInitResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId);
    void sendAgentUpdateResponse(IPv6Address destAddr, IPv6Address sourceAddr, uint64 mobileId);

    // Header processing
    void processMobileAgentMessage(MobileAgentHeader *agentHdr, IPv6ControlInfo *ipCtrlInfo);
    void processControlAgentMessage(ControlAgentHeader *agentHdr, IPv6ControlInfo *ipCtrlInfo);
    void processDataAgentMessage(DataAgentHeader *agentHdr, IPv6ControlInfo *ipCtrlInfo);

    void processIncomingUDPPacket(cMessage *msg, IPv6ControlInfo *ipCtrlInfo);
//    void processIncomingTCPPacket(cMessage *msg);

    // functions for handling interface change
    void createInterfaceDownMessage(int id);
    void handleInterfaceDownMessage(cMessage *msg);
    void createInterfaceUpMessage(int id);
    void handleInterfaceUpMessage(cMessage *msg);
    void updateAddressTable(int id, InterfaceUnit *iu);
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj) override;
    InterfaceUnit* getInterfaceUnit(int id);

    InterfaceEntry *getInterface(IPv6Address destAddr, int destPort = -1, int sourcePort = -1, short protocol = -1); //const ,
    InterfaceEntry *getAnyInterface();
    // extension header processing
//    void sendToLowerLayer(cObject *obj, const IPv6Address& destAddr, const IPv6Address& srcAddr = IPv6Address::UNSPECIFIED_ADDRESS, int interfaceId = -1, simtime_t sendTime = 0); // resend after timer expired
//    void messageProcessingUnitMA(MobileAgentOptionHeader *optHeader, IPv6Datagram *datagram, IPv6ControlInfo *controlInfo);
//    void messageProcessingUnitCA(ControlAgentOptionHeader *optHeader, IPv6Datagram *datagram, IPv6ControlInfo *controlInfo);

//============================================= Timer configuration ===========================
    class ExpiryTimer {
    public:
        virtual ~ExpiryTimer() {}; // TODO should delete pointers
        cMessage *timer;
        IPv6Address dest;
        simtime_t ackTimeout;
        simtime_t nextScheduledTime;
        InterfaceEntry *ie; // store over which entry it should be send
    };
    class TimerKey {
    public:
        int type;
        int interfaceID;
        uint64 mobileId;
        uint seqNo;
        IPv6Address dest;
        TimerKey(IPv6Address _dest, int _interfaceID, int _type) {
            dest=_dest;
            interfaceID=_interfaceID;
            type=_type;
            mobileId=0;
            seqNo=0;
        }
        TimerKey(IPv6Address _dest, int _interfaceID, int _type, uint64 _mobileId) {
            dest=_dest;
            interfaceID=_interfaceID;
            type=_type;
            mobileId=_mobileId;
            seqNo=0;
        }
        TimerKey(IPv6Address _dest, int _interfaceID, int _type, uint64 _mobileId, uint _seqNo) {
            dest=_dest;
            interfaceID=_interfaceID;
            type=_type;
            mobileId=_mobileId;
            seqNo=_seqNo;
        }
        TimerKey(int _interfaceID,int _type) {
            dest=dest.UNSPECIFIED_ADDRESS;
            interfaceID=_interfaceID;
            type=_type;
            mobileId=0;
            seqNo=0;
        }
        virtual ~TimerKey() {};
        bool operator<(const TimerKey& b) const {
            if(type == b.type) {
                if(interfaceID == b.interfaceID) {
                    if(dest == b.dest) {
                        if(mobileId == b.mobileId) {
                            return seqNo < b.seqNo;
                        } else
                            return mobileId < b.mobileId;
                    } else
                        return dest < b.dest;
                } else
                    return interfaceID < b.interfaceID;
            } else
                return type < b.type;
        }
    };
    typedef std::map<TimerKey,ExpiryTimer *> ExpiredTimerList;
    ExpiredTimerList expiredTimerList;

    // TimerType 50
    class SessionInitTimer : public ExpiryTimer {
    public:
        uint lifetime;
        uint64 id;
    };
    // TimerType 51
    class SequenceInitTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    // TimerType 52
    class SequenceUpdateTimer : public ExpiryTimer {
    public:
        uint64 id;
    };
    // TimerType 53
    class LocationUpdateTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    // TimerType 54
    class InterfaceDownTimer : public ExpiryTimer {
    public:
        bool active = false;
    };
    class InterfaceUpTimer : public ExpiryTimer {
    public:
        bool active = false;
    };
    class FlowRequestTimer : public ExpiryTimer {
    public:
        FlowTuple *tuple;
    };
    class MobileInitTimer : public ExpiryTimer {
    public:
        uint64 id;
    };
    class UpdateNotifierTimer : public ExpiryTimer {
    public:
        uint64 id;
        uint seq;
        uint ack;
    };
    ExpiryTimer *getExpiryTimer(TimerKey& key, int timerType);
    bool pendingExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType, uint64 id = 0);
    bool cancelExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType, uint64 id = 0);
    bool cancelAndDeleteExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType, uint64 id = 0);
    void cancelExpiryTimers();
//============================================= Timer configuration ===========================

};

} //namespace

#endif
