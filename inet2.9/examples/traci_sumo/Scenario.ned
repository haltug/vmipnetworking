// Halis Altug - mail@halisaltug.de
// TU Darmstadt - Multimedia Kommunikation
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


// Adapted and updated TraCI module from Veins framework for INET 3.0. 
// Ping test in a highway-urban scenario in Frankfurt am Main. Map data from
// OpenStreetMap (OSM). 
package inet.examples.traci_sumo;

import inet.environment.common.PhysicalEnvironment;
import inet.linklayer.ethernet.EtherHub;
import inet.mobility.traci.TraCIScenarioManagerLaunchd;
import inet.networklayer.configurator.ipv6.FlatNetworkConfigurator6;
import inet.node.wireless.AccessPoint;
import inet.node.ipv6.*;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;

channel ethernetline extends ned.DatarateChannel
{
    parameters:
        delay = 0.1us;
        datarate = 1000Mbps;
}

network sumoExampleFrankfurt
{
    parameters:
        double total_mn;
        double total_cn;
        @display("bgb=6231,2104;bgi=background/map,s");

    submodules:
        environment: PhysicalEnvironment {
            parameters:
                @display("p=50,180");
        }
        manager: TraCIScenarioManagerLaunchd {
            parameters:
                @display("p=50,260");
        }
        configurator: FlatNetworkConfigurator6 {
            parameters:
                @display("p=50,20");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            parameters:
                @display("p=50,100");
        }
        R_1: Router6 {
            parameters:
                @display("p=182,902");
        }
        R_2: Router6 {
            parameters:
                @display("p=1444,248");
        }
        R_3: Router6 {
            parameters:
                @display("p=4711,49");
        }
        R_4: Router6 {
            parameters:
                @display("p=3298,168");
        }
        R_5: Router6 {
            parameters:
                @display("p=1168,40");
        }
        AP_1: AccessPoint {
            parameters:
                @display("p=1552,1716;i=device/accesspoint_s");
        }
        AP_2: AccessPoint {
            parameters:
                @display("p=1778,421;i=device/accesspoint_s");
        }
        AP_3: AccessPoint {
            parameters:
                @display("p=5464,955;i=device/accesspoint_s");
        }
        AP_4: AccessPoint {
            parameters:
                @display("p=3618,901;i=device/accesspoint_s");
        }
        AP_5: AccessPoint {
            parameters:
                @display("p=1244,1176;i=device/accesspoint_s");
        }
        AP_6: AccessPoint {
            parameters:
                @display("p=1137,439;i=device/accesspoint_s");
        }
        AP_7: AccessPoint {
            parameters:
                @display("p=5896,473;i=device/accesspoint_s");
        }
        AP_8: AccessPoint {
            parameters:
                @display("p=3001,684;i=device/accesspoint_s");
        }
        AP_9: AccessPoint {
            parameters:
                @display("p=952,792;i=device/accesspoint_s");
        }
        AP_10: AccessPoint {
            parameters:
                @display("p=2469,536;i=device/accesspoint_s");
        }
        AP_11: AccessPoint {
            parameters:
                @display("p=4742,1111;i=device/accesspoint_s");
        }
        AP_12: AccessPoint {
            parameters:
                @display("p=4181,1093;i=device/accesspoint_s");
        }
        CN[total_cn]: StandardHost6 {
            parameters:
                @display("p=215,30");
        }
    connections allowunconnected:
        R_1.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_1.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_2.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_2.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_3.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_3.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        R_4.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        R_4.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        AP_1.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        AP_1.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
        AP_2.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        AP_2.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        AP_3.ethg$o++ --> ethernetline --> R_3.ethg$i++;
        AP_3.ethg$i++ <-- ethernetline <-- R_3.ethg$o++;
        AP_4.ethg$o++ --> ethernetline --> R_4.ethg$i++;
        AP_4.ethg$i++ <-- ethernetline <-- R_4.ethg$o++;
        AP_5.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        AP_5.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
        AP_6.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        AP_6.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        AP_7.ethg$o++ --> ethernetline --> R_3.ethg$i++;
        AP_7.ethg$i++ <-- ethernetline <-- R_3.ethg$o++;
        AP_8.ethg$o++ --> ethernetline --> R_4.ethg$i++;
        AP_8.ethg$i++ <-- ethernetline <-- R_4.ethg$o++;
        AP_9.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        AP_9.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
        AP_10.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        AP_10.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        AP_11.ethg$o++ --> ethernetline --> R_3.ethg$i++;
        AP_11.ethg$i++ <-- ethernetline <-- R_3.ethg$o++;
        AP_12.ethg$o++ --> ethernetline --> R_4.ethg$i++;
        AP_12.ethg$i++ <-- ethernetline <-- R_4.ethg$o++;
        for i=0..total_cn-1 {
            CN[i].ethg$o++ --> ethernetline --> R_5.ethg$i++;
            CN[i].ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        }
}
