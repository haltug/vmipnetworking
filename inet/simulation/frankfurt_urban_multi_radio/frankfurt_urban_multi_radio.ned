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

package frankfurt_urban_multi_radio;

import inet.linklayer.ethernet.EtherHub;
import inet.physicallayer.ieee80211.packetlevel.*;
import inet.environment.common.PhysicalEnvironment;
import inet.networklayer.configurator.ipv6.FlatNetworkConfigurator6;
import inet.node.ipv6.*;
import inet.node.ipv6mev.*;
import inet.node.wireless.AccessPoint;

import inet.mobility.traci.TraCIScenarioManagerLaunchd;

channel E1 extends ned.DatarateChannel
{
    parameters:
        delay = 75us;
        datarate = 1Gbps;
}

channel E2 extends ned.DatarateChannel
{
    parameters:
        delay = 250us;
        datarate = 10Gbps;
}



network frankfurt_urban_multi_radio
{
    parameters:
        int total_cn;
        int total_mn;
        int total_da;
        @display("bgb=6231,2104;bgi=background/frankfurtUrban,s;bgs=0.203,m");

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
                @display("p=1403,98;is=s");
        }
        R_2: Router6 {
            parameters:
                @display("p=2133,98;is=s");
        }
        R_3: Router6 {
            parameters:
                @display("p=3073,98;is=s");
        }
        R_4: Router6 {
            parameters:
                @display("p=4073,98;is=s");
        }
        R_5: Router6 {
            parameters:
                @display("p=4960,98;is=s");
        }
        R_6: Router6 {
            parameters:
                @display("p=2128,1211;is=n;i=,black");
        }
        R_7: Router6 {
            parameters:
                @display("p=2128,1655;is=n;i=,black");
        }
        R_8: Router6 {
            parameters:
                @display("p=4073,1211;is=n;i=,black");
        }
        R_9: Router6 {
            parameters:
                @display("p=4073,1655;is=n;i=,black");
        }
        AP_1: AccessPoint {
            parameters:
                @display("p=1369,1487;is=s");
        }
        AP_2: AccessPoint {
            parameters:
                @display("p=1137,733;is=s");
        }
        AP_3: AccessPoint {
            parameters:
                @display("p=1665,532;is=s");
        }
        AP_4: AccessPoint {
            parameters:
                @display("p=2472,532;is=s");
        }
        AP_5: AccessPoint {
            parameters:
                @display("p=3315,620;is=s");
        }
        AP_6: AccessPoint {
            parameters:
                @display("p=4128,709;is=s");
        }
        AP_7: AccessPoint {
            parameters:
                @display("p=4753,556;is=s");
        }
        AP_8: AccessPoint {
            parameters:
                @display("p=5226,940;is=s");
        }
        AP_9: AccessPoint {
            parameters:
                @display("p=6000,620;is=s");
        }
        CN1[total_cn]: StandardHost6 {
            parameters:
                @display("p=5339,1655;is=s");
        }

        
        CA: ControlAgent {
            parameters:
                @display("p=793,1206;is=s");
        }
        DA1[total_da]: DataAgent {
            parameters:
                @display("p=4073,1945;is=s");
        }

    connections allowunconnected:
        R_1.ethg$o++ --> E1 --> R_2.ethg$i++;
        R_1.ethg$i++ <-- E1 <-- R_2.ethg$o++;
        R_2.ethg$o++ --> E1 --> R_3.ethg$i++;
        R_2.ethg$i++ <-- E1 <-- R_3.ethg$o++;
        R_3.ethg$o++ --> E1 --> R_4.ethg$i++;
        R_3.ethg$i++ <-- E1 <-- R_4.ethg$o++;
        R_4.ethg$o++ --> E1 --> R_5.ethg$i++;
        R_4.ethg$i++ <-- E1 <-- R_5.ethg$o++;
        R_6.ethg$o++ --> E2 --> R_7.ethg$i++;
        R_6.ethg$i++ <-- E2 <-- R_7.ethg$o++;
        R_6.ethg$o++ --> E2 --> R_2.ethg$i++;
        R_6.ethg$i++ <-- E2 <-- R_2.ethg$o++;
        R_7.ethg$o++ --> E2 --> R_9.ethg$i++;
        R_7.ethg$i++ <-- E2 <-- R_9.ethg$o++;
        R_8.ethg$o++ --> E2 --> R_9.ethg$i++;
        R_8.ethg$i++ <-- E2 <-- R_9.ethg$o++;
        R_8.ethg$o++ --> E2 --> R_6.ethg$i++;
        R_8.ethg$i++ <-- E2 <-- R_6.ethg$o++;
        R_8.ethg$o++ --> E2 --> R_4.ethg$i++;
        R_8.ethg$i++ <-- E2 <-- R_4.ethg$o++;
        AP_1.ethg$o++ --> E1 --> R_1.ethg$i++;
        AP_1.ethg$i++ <-- E1 <-- R_1.ethg$o++;
        AP_2.ethg$o++ --> E1 --> R_1.ethg$i++;
        AP_2.ethg$i++ <-- E1 <-- R_1.ethg$o++;
        AP_3.ethg$o++ --> E1 --> R_1.ethg$i++;
        AP_3.ethg$i++ <-- E1 <-- R_1.ethg$o++;
        AP_4.ethg$o++ --> E1 --> R_3.ethg$i++;
        AP_4.ethg$i++ <-- E1 <-- R_3.ethg$o++;
        AP_5.ethg$o++ --> E1 --> R_3.ethg$i++;
        AP_5.ethg$i++ <-- E1 <-- R_3.ethg$o++;
        AP_6.ethg$o++ --> E1 --> R_3.ethg$i++;
        AP_6.ethg$i++ <-- E1 <-- R_3.ethg$o++;
        AP_7.ethg$o++ --> E1 --> R_5.ethg$i++;
        AP_7.ethg$i++ <-- E1 <-- R_5.ethg$o++;
        AP_8.ethg$o++ --> E1 --> R_5.ethg$i++;
        AP_8.ethg$i++ <-- E1 <-- R_5.ethg$o++;
        AP_9.ethg$o++ --> E1 --> R_5.ethg$i++;
        AP_9.ethg$i++ <-- E1 <-- R_5.ethg$o++;

        CA.ethg$o++ --> E2 --> R_6.ethg$i++;
        CA.ethg$i++ <-- E2 <-- R_6.ethg$o++;
        for i=0..total_cn-1 {
            CN1[i].ethg$o++ --> E2 --> R_9.ethg$i++;
            CN1[i].ethg$i++ <-- E2 <-- R_9.ethg$o++;
        }
        for i=0..total_da-1 {
            DA1[i].ethg$o++ --> E1 --> R_9.ethg$i++;
            DA1[i].ethg$i++ <-- E1 <-- R_9.ethg$o++;
        }
}
