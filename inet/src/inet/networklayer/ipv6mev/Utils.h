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

#ifndef UTILS_H_
#define UTILS_H_

namespace inet {

#define KEY_SEQ_CHANGE_CA               0 // Sequence Update to CA
#define KEY_SEQ_CHANGE_DA               1 // Sequence Update to CA

class Utils
{
  public:
    Utils();
    virtual ~Utils();

    class RetransmitTimer
    {
      public:
        cMessage *timer;
        virtual ~RetransmitTimer() {};
        IPv6Address dest;    // the address (HA or CN(s) for which the message is sent
        simtime_t ackTimeout;    // timeout for the Ack
        simtime_t nextScheduledTime;    // time when the corrsponding message is supposed to be sent
        InterfaceEntry *ifEntry;    // interface from which the message will be transmitted

    };

    struct Key // TODO
    {
        int type;    // type of the message (BU, HoTI, CoTI) stored in the map, indexed by this key
        int interfaceID;    // ID of the interface over which the message is sent
        IPv6Address dest;    // the address of either the HA or the CN
        Key(IPv6Address _dest, int _interfaceID, int _type)
        {
            dest = _dest;
            interfaceID = _interfaceID;
            type = _type;
        }
        bool operator<(const Key& b) const
        {
            if (type == b.type)
                return interfaceID == b.interfaceID ? dest < b.dest : interfaceID < b.interfaceID;
            else
                return type < b.type;
        }
    };

    RetransmitTimer *getRetransmitTimer(Key& key);

    RetransmitTimer *lookupRetransmitTimer(IPv6Address& ip);

    RetransmitTimer *removeRetransmitTimer(IPv6Address& ip);



};
} /* namespace inet */

#endif /* UTILS_H_ */
