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

package veins.nodes;

import veins.base.modules.*;
import veins.modules.nic.Nic80211p;
import veins.modules.nic.Ip80211p;
import inet.common.lifecycle.NodeStatus;
import inet.linklayer.contract.IMACRelayUnit;
import inet.linklayer.contract.IMACAddressTable;
import inet.linklayer.contract.IWiredNic;
import inet.linklayer.contract.IWirelessNic;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;

module BasicRSU
{
    parameters:
        @node();
        @labels(node,ethernet-node,wireless-node);
        @display("i=device/accesspoint");
        bool hasStatus = default(false);
        string applType; //type of the application layer
        string nicType = default("Ip80211p"); // type of network interface card
        string relayUnitType = default(firstAvailable("MACRelayUnit"));
        string macTableType = default("MACAddressTable"); // type of the ~IMACAddressTable
        eth.encapType = "EtherEncapDummy";
        *.interfaceTableModule = default(absPath(".interfaceTable"));
    gates:
        input veinsradioIn; // gate for sendDirect
        inout ethg @labels(EtherFrame-conn);
    submodules:
        status: NodeStatus if hasStatus {
            @display("p=30,30");
        }

        interfaceTable: InterfaceTable {
            @display("p=30,90;is=s");
        }
        relayUnit: <relayUnitType> like IMACRelayUnit {
            parameters:
                @display("p=200,150");
        }
        appl: <applType> like veins.base.modules.IBaseApplLayer {
            parameters:
                @display("p=100,100");
        }
        nic: <nicType> like veins.modules.nic.INic80211p {
            parameters:
                @display("p=100,300");
        }
        macTable: <macTableType> like IMACAddressTable {
            @display("p=30,150;is=s");
        }
        mobility: BaseMobility {
            parameters:
                @display("p=300,38;i=block/cogwheel");
        }
        eth: <default("EthernetInterface")> like IWiredNic {
            parameters:
                mac.promiscuous = true;
                @display("p=300,300,row;q=txQueue");
        }
    connections:
        eth.phys <--> ethg;
        eth.upperLayerIn <-- relayUnit.ifOut++;
        eth.upperLayerOut --> relayUnit.ifIn++;

        nic.upperLayerOut --> relayUnit.ifIn++;
        nic.upperLayerIn <-- relayUnit.ifOut++;

        nic.upperControlOut --> appl.lowerControlIn;
        nic.upperControlIn <-- appl.lowerControlOut;

        veinsradioIn --> nic.radioIn;
}
