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

#include <omnetpp.h>
#include <vector>
#include <map>
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/networklayer/contract/ipv6/IPv6Address.h"
#include "inet/networklayer/contract/ipv6/IPv6ControlInfo.h"
#include "inet/networklayer/ipv6/IPv6Datagram.h"
#include "inet/networklayer/ipv6mev/AddressManagement.h"
#include "inet/networklayer/ipv6mev/IdentificationHeader.h"

namespace inet {

#define KEY_CA_INIT 0
#define MSG_CA_INIT 100
#define TIM_CA_INIT 1 // sec
class InterfaceEntry;
/**
 * TODO - Generated class
 */
class Agent : public cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

  public:
    Agent();
    virtual ~Agent();

    enum AgentState {
        UNASSOCIATED = 0,
        INITIALIZE   = 1,
        REGISTERED   = 2
    };
    AgentState state;

    IInterfaceTable *ift; // for recognizing changes etc
    AddressManagement *am; // for sliding address mechanism
    IPv6Address CA_Address; // ip address of ca
    bool isMA;
    bool isCA;
    bool isDA;

    typedef std::map<int, IPv6Address> InterfaceToIPv6AddressList; // not sure if type is set correct but it's stores association of dest addr of cn to data agent addr
    InterfaceToIPv6AddressList interfaceToIPv6AddressList;
    typedef std::map<IPv6Address, IPv6Address> DirectAddressList; // IPv6address should be replaced with DataAgent <cn,da>
    DirectAddressList directAddressList;

    virtual void processLowerLayerMessage(cMessage *msg);
    virtual void processUpperLayerMessage(cMessage *msg);

    void createCAInitialization();
    void sendCAInitialization(); // send initialization message to CA
    void resendCAInitialization(cMessage *msg); // resend after timer expired
    void processCAMessages(ControlAgentHeader *ctrlAgentHdr, IPv6ControlInfo *ipCtrlInfo);
//============================================= Timer configuration ===========================
    class ExpiryTimer {
    public:
        cMessage *timer;
        virtual ~ExpiryTimer();
        IPv6Address dest;
        simtime_t ackTimeout;
        simtime_t nextScheduledTime;
        InterfaceEntry *ie; // store over which entry it should be send
    };
    class TimerKey {
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
    typedef std::map<TimerKey,ExpiryTimer> ExpiredTimerList;
    ExpiredTimerList expiredTimerList;
    class InitMessageTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    class SessionRequestMessageTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    class SequenceUpdateMessageTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    class LocationUpdateMessageTimer : public ExpiryTimer {
    public:
        uint lifetime;
    };
    ExpiryTimer *getExpiryTimer(TimerKey& key, int timerType);

    bool cancelExpiryTimer(const IPv6Address& dest, int interfaceID, int msgType);
    void cancelExpiryTimers();
//============================================= Timer configuration ===========================

};

} //namespace

#endif
