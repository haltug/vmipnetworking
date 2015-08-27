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

package frankfurt_urban;

import inet.linklayer.ethernet.EtherHub;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.environment.common.PhysicalEnvironment;
import inet.networklayer.configurator.ipv6.FlatNetworkConfigurator6;
import inet.node.ipv6.*;
import inet.node.ipv6mev.*;
import inet.node.wireless.AccessPoint;

import inet.mobility.traci.TraCIScenarioManagerLaunchd;

channel ethernetline extends ned.DatarateChannel
{
    parameters:
        delay = 0.1us;
        datarate = 1000Mbps;
}

network frankfurt_urban
{
    parameters:
        int total_cn;
        int total_mn;
        int total_da;
        @display("bgb=6231,2104;bgi=background/frankfurtUrban,s");

    submodules:
        environment: PhysicalEnvironment {
            parameters:
                @display("p=50,180;is=l");
        }
        manager: TraCIScenarioManagerLaunchd {
            parameters:
                @display("p=50,260;is=l");
        }
        configurator: FlatNetworkConfigurator6 {
            parameters:
                @display("p=50,20;is=l");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=50,100;is=l");
        }
        R_1: Router6 {
            parameters:
                @display("p=1554,1086;is=l");
        }
        R_2: Router6 {
            parameters:
                @display("p=2960,295;is=l");
        }
        R_3: Router6 {
            parameters:
                @display("p=4963,346;is=l");
        }
        R_5: Router6 {
            parameters:
                @display("p=1258,101;is=l");
        }
        R_10: Router6 {
            parameters:
                @display("p=1948,101;is=l");
        }
        AP_1: AccessPoint {
            parameters:
                @display("p=1318,1486;is=vl");
        }
        AP_2: AccessPoint {
            parameters:
                @display("p=967,666;is=vl");
        }
        AP_3: AccessPoint {
            parameters:
                @display("p=1766,496;is=vl");
        }
        AP_4: AccessPoint {
            parameters:
                @display("p=2530,411;is=vl");
        }
        AP_5: AccessPoint {
            parameters:
                @display("p=3410,478;is=vl");
        }
        AP_6: AccessPoint {
            parameters:
                @display("p=4200,582;is=vl");
        }
        AP_7: AccessPoint {
            parameters:
                @display("p=4752,648;is=vl");
        }
        AP_8: AccessPoint {
            parameters:
                @display("p=5218,1026;is=vl");
        }
        AP_9: AccessPoint {
            parameters:
                @display("p=5742,709;is=vl");
        }
        CN[total_cn]: StandardHost6 {
            parameters:
                @display("p=2366,101;is=vl");
        }
        CA: ControlAgent {
            parameters:
                @display("p=420,101;is=vl");
        }
        DA[total_da]: DataAgent {
            parameters:
                @display("p=1948,201;is=vl");
        }
    //        MA[total_mn]: MobileAgent {
    //            parameters:
    //                @display("p=150,400;");
    //                numRadios = default(1);
    //        }
    connections allowunconnected:
        R_1.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_1.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_2.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_2.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_3.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_3.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        CA.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        CA.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_10.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_10.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
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
            CN[i].ethg$o++ --> ethernetline --> R_10.ethg$i++;
            CN[i].ethg$i++ <-- ethernetline <-- R_10.ethg$o++;
        }
		for i=0..total_da-1 {
            DA[i].ethg$o++ --> ethernetline --> R_10.ethg$i++;
            DA[i].ethg$i++ <-- ethernetline <-- R_10.ethg$o++;
        }
}
