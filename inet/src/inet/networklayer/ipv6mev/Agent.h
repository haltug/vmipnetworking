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
#define TIMERKEY_UPDATE_ACK      9
//#define TIMERKEY_UDP_OUT_MSG     10
//#define TIMERKEY_TCP_OUT_MSG     11

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
#define TIMERTYPE_UPDATE_ACK    59
//#define TIMERTYPE_UDP_OUT_MSG   60
//#define TIMERTYPE_TCP_OUT_MSG   61

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
#define MSG_TCP_RETRANSMIT      115
#define MSG_INTERFACE_DELAY     116
#define MSG_UPDATE_ACK          117

//========== Retransmission time of messages ==========
#define TIMEOUT_SESSION_INIT    1 // retransmission time of ca init in sec
#define TIMEOUT_SEQNO_INIT      1
#define TIMEOUT_SEQ_UPDATE      1
#define TIMEOUT_LOC_UPDATE      1
#define TIMEDELAY_IF_DOWN       3   // delay of ip msg handler
#define TIMEDELAY_IF_UP         0
#define TIMEDELAY_FLOW_REQ      1 // unit is [s]
#define TIMEDELAY_PKT_PROCESS   0.5
#define TIMEDELAY_MA_INIT       1 // unit is [s]
#define MAX_PKT_LIFETIME        30 // specifies retransmission attempts until pkt is discarded for udp tcp
#define TIMEDELAY_IFACE_INIT    1 // delaying registration of interface when MA is in init stage.
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
class INET_API Agent : public cSimpleModule
{
  public:
    virtual ~Agent() {};

    enum AgentState {
        UNASSOCIATED = 0,
        INITIALIZING = 1,
        ASSOCIATED   = 2
    };
    AgentState sessionState; // state of MA at beginning
    AgentState seqnoState; // state of MA for seq init

    IPv6Address caAddress; // ip address of ca
    AddressManagement am;

    std::vector<uint64>  mobileIdList; // lists all id of mobile nodes
    std::vector<IPv6Address> nodeAddressList; // lists all data agents

    enum FlowState {
        UNREGISTERED = 0,
        REGISTERING  = 1,
        REGISTERED   = 2
    };

    struct FlowTuple {
        short protocol;
        int destPort;
        int sourcePort;
        IPv6Address destAddress;
        uint64 interfaceId;
        bool operator<(const FlowTuple& b) const {
            if(destAddress == b.destAddress) {
                if(destPort == b.destPort) {
                    if(sourcePort == b.sourcePort) {
                        if(interfaceId == b.interfaceId) {
                            return protocol < b.protocol;
                        } else
                            return interfaceId < b.interfaceId;
                    } else
                        return sourcePort < b.sourcePort;
                } else
                    return destPort < b.destPort;
            } else
                return destAddress < b.destAddress;
        }
    };

    struct FlowUnit {
        FlowState state;
        bool isFlowActive;
        bool isAddressCached; // specifiy if address should be cached
//        bool locationUpdate;
//        bool loadSharing;
        int  lifetime;

        uint64 id;
        IPv6Address dataAgent; // defines the address of data agent
        IPv6Address mobileAgent; // defines the current return address. if MA send over this path, DA will return over same one.
        IPv6Address nodeAddress; // defines the address of correspondent node
    };

    typedef std::map<FlowTuple, FlowUnit> FlowTable; // IPv6address should be replaced with DataAgent <cn,da>
    FlowTable flowTable;

    typedef std::map<IPv6Address,IPv6Address> AddressAssociation; // nodeAddress -> agentAddress
    AddressAssociation addressAssociation;

    FlowUnit *getFlowUnit(FlowTuple &tuple);
    IPv6Address *getAssociatedAddress(const IPv6Address &dest);
    bool isAddressAssociated(const IPv6Address &dest);
    /**
     * All fields are by default false. Type: MA=1, CA=2, DA=3
     */
    IdentificationHeader *getAgentHeader(short type, short protocol, uint seq, uint ack, uint64 id);
    // Interface Id of DA or local id of MA/CA
    uint64 agentId;

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
        uint seq;
        uint ack;
        IPv6Address dest;
//        IPv6Address flowRequest; // add later
        TimerKey(IPv6Address _dest, int _interfaceID, int _type) {
            dest=_dest;
            interfaceID=_interfaceID;
            type=_type;
            mobileId=0;
            seq=0;
        }
        TimerKey(IPv6Address _dest, int _interfaceID, int _type, uint64 _mobileId) {
            dest=_dest;
            interfaceID=_interfaceID;
            type=_type;
            mobileId=_mobileId;
            seq=0;
        }
        TimerKey(IPv6Address _dest, int _interfaceID, int _type, uint64 _mobileId, uint _seq) {
            dest=_dest;
            interfaceID=_interfaceID;
            type=_type;
            mobileId=_mobileId;
            seq=_seq;
        }
        TimerKey(int _interfaceID,int _type) {
            dest=dest.UNSPECIFIED_ADDRESS;
            interfaceID=_interfaceID;
            type=_type;
            mobileId=0;
            seq=0;
        }
        virtual ~TimerKey() {};
        bool operator<(const TimerKey& b) const {
            if(type == b.type) {
                if(interfaceID == b.interfaceID) {
                    if(dest == b.dest) {
                        if(mobileId == b.mobileId) {
                            return seq < b.seq;
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
        uint64 id;
    };
    // TimerType 51
    class SequenceInitTimer : public ExpiryTimer {
    public:
        uint64 id;
    };
    // TimerType 52
    class SequenceUpdateTimer : public ExpiryTimer {
    public:
        uint64 id;
        uint seq;
        uint ack;
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
        FlowTuple tuple;
        IPv6Address nodeAddress;
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
        simtime_t liftime;
        bool active = false;
    };
    class UpdateAckTimer : public ExpiryTimer {
    public:
        uint64 id;
        uint seq;
    };
    // TODO should it be declared as virtual?
    ExpiryTimer *getExpiryTimer(TimerKey& key, int timerType);
    bool pendingExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType, uint64 id = 0, uint seq = 0);
    bool cancelExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType, uint64 id = 0, uint seq = 0, uint ack = 0);
    bool cancelAndDeleteExpiryTimer(const IPv6Address& dest, int interfaceId, int timerType, uint64 id = 0, uint seq = 0, uint ack = 0);
//============================================= Timer configuration ===========================

};

} //namespace

#endif
