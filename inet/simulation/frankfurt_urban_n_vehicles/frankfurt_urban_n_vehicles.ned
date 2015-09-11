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

package frankfurt_urban_n_vehicles;

import inet.linklayer.ethernet.EtherHub;
import inet.physicallayer.ieee80211.packetlevel.*;
import inet.environment.common.PhysicalEnvironment;
import inet.networklayer.configurator.ipv6.FlatNetworkConfigurator6;
import inet.node.ipv6.*;
import inet.node.ipv6mev.*;
import inet.node.wireless.AccessPoint;

import inet.mobility.traci.TraCIScenarioManagerLaunchd;

channel ethernetline extends ned.DatarateChannel
{
    parameters:
        delay = 4us;
        datarate = 10Gbps;
}

network frankfurt_urban_n_vehicles
{
    parameters:
        int total_cn;
        int total_mn;
        int total_da;
        @display("bgb=6231,2104;bgi=background/frankfurtUrban,s;bgs=0.203,km");

    submodules:
        environment: PhysicalEnvironment {
            parameters:
                @display("p=200,550;is=vs");
        }
        manager: TraCIScenarioManagerLaunchd {
            parameters:
                @display("p=200,400;is=vs");
        }
        configurator: FlatNetworkConfigurator6 {
            parameters:
                @display("p=200,100;is=vs");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=200,250;is=vs");
        }
        R_1: Router6 {
            parameters:
                @display("p=1554,1086;is=s");
        }
        R_2: Router6 {
            parameters:
                @display("p=3073.8916,98.52217;is=s");
        }
        R_3: Router6 {
            parameters:
                @display("p=4960.5913,98.52217;is=s");
        }
        R_5: Router6 {
            parameters:
                @display("p=2128.0789,1822.6602;is=s");
        }
        R_6: Router6 {
            parameters:
                @display("p=2133.005,98.52217;is=s");
        }
        R_7: Router6 {
            parameters:
                @display("p=4073.8918,98.52217;is=s");
        }
        R_10: Router6 {
            parameters:
                @display("p=4073.8918,1822.6602;is=s");
        }
        AP_1: AccessPoint {
            parameters:
                @display("p=1369.4581,1487.6848;is=s");
        }
        AP_2: AccessPoint {
            parameters:
                @display("p=967,666;is=s");
        }
        AP_3: AccessPoint {
            parameters:
                @display("p=1763.5469,339.9015;is=s");
        }
        AP_4: AccessPoint {
            parameters:
                @display("p=2561.5764,477.83252;is=s");
        }
        AP_5: AccessPoint {
            parameters:
                @display("p=3315.271,620.6897;is=s");
        }
        AP_6: AccessPoint {
            parameters:
                @display("p=4128.079,709.3596;is=s");
        }
        AP_7: AccessPoint {
            parameters:
                @display("p=4752,648;is=s");
        }
        AP_8: AccessPoint {
            parameters:
                @display("p=5226.601,940.8867;is=s");
        }
        AP_9: AccessPoint {
            parameters:
                @display("p=6000,620.6897;is=s");
        }
        CN1[total_cn]: StandardHost6 {
            parameters:
                @display("p=5226.601,1822.6602;is=s");
        }
        CA: ControlAgent {
            parameters:
                @display("p=418.71924,1822.6602;is=s");
        }
        DA1[total_da]: DataAgent {
            parameters:
                @display("p=4073.8918,1374.3843;is=s");
        }
    connections allowunconnected:
        R_1.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_1.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_2.ethg$o++ --> ethernetline --> R_6.ethg$i++;
        R_2.ethg$i++ <-- ethernetline <-- R_6.ethg$o++;
        R_7.ethg$o++ --> ethernetline --> R_6.ethg$i++;
        R_7.ethg$i++ <-- ethernetline <-- R_6.ethg$o++;
        R_3.ethg$o++ --> ethernetline --> R_7.ethg$i++;
        R_3.ethg$i++ <-- ethernetline <-- R_7.ethg$o++;
        R_6.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_6.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        CA.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        CA.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_10.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_10.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
		R_10.ethg$o++ --> ethernetline --> R_7.ethg$i++;
        R_10.ethg$i++ <-- ethernetline <-- R_7.ethg$o++;        
        AP_1.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        AP_1.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
        AP_2.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        AP_2.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
        AP_3.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        AP_3.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
        AP_4.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        AP_4.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        AP_5.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        AP_5.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        AP_6.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        AP_6.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        AP_7.ethg$o++ --> ethernetline --> R_3.ethg$i++;
        AP_7.ethg$i++ <-- ethernetline <-- R_3.ethg$o++;
        AP_8.ethg$o++ --> ethernetline --> R_3.ethg$i++;
        AP_8.ethg$i++ <-- ethernetline <-- R_3.ethg$o++;
        AP_9.ethg$o++ --> ethernetline --> R_3.ethg$i++;
        AP_9.ethg$i++ <-- ethernetline <-- R_3.ethg$o++;
        for i=0..total_cn-1 {
            CN1[i].ethg$o++ --> ethernetline --> R_10.ethg$i++;
            CN1[i].ethg$i++ <-- ethernetline <-- R_10.ethg$o++;
        }
        for i=0..total_da-1 {
            DA1[i].ethg$o++ --> ethernetline --> R_10.ethg$i++;
            DA1[i].ethg$i++ <-- ethernetline <-- R_10.ethg$o++;
        }
}
