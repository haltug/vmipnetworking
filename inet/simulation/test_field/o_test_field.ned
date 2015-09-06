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

package test_field;

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
        datarate = 100Mbps;
}

network test_field
{
    parameters:
        double total_mn;
        double total_cn;
        //	    **.mgmt.numChannels = 5;
        double playgroundSizeX @unit(m) = default(1000m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m) = default(1000m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m) = default(2m); // z size of the area the nodes are in (in meters)
        @display("bgb=1200,900;bgi=background/streetmap,f");
//		@display("bgb=$playgroundSizeX,$playgroundSizeY;bgi=background/streetmap,f");
//        @display("bgb=1176,812;bgi=background/streetmap,s");

    submodules:
        environment: PhysicalEnvironment {
            parameters:
                @display("p=50,180");
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
                @display("p=140,230");
        }
        R_2: Router6 {
            parameters:
                @display("p=700,230");
        }
        R_3: Router6 {
            parameters:
                @display("p=230,570");
        }
        R_4: Router6 {
            parameters:
                @display("p=757,570");
        }
        R_5: Router6 {
            parameters:
                @display("p=430,30");
        }
        AP_1: AccessPoint {
            parameters:
                @display("p=140,342;i=device/accesspoint_s");
        }
        //        AP_2: AccessPoint {
        //            parameters:
        //                @display("p=630,300;i=device/accesspoint_s");
        //        }
        AP_3: AccessPoint {
            parameters:
                @display("p=312,464;i=device/accesspoint_s");
        }
        AP_4: AccessPoint {
            parameters:
                @display("p=757,464;i=device/accesspoint_s");
        }
        //        HUB_1: EtherHub {
        //            parameters:
        //                @display("p=730,30;i=device/hub_s");
        //        }
        CN1[total_cn]: StandardHost6 {
            parameters:
                @display("p=1030,30");
        }
        CN2[total_cn]: StandardHost6 {
            parameters:
                @display("p=1030,226");
        }
        CA: ControlAgent {
            parameters:
                @display("p=140,30");
        }
        DA1: DataAgent {
            parameters:
                @display("p=700,30");
        }
        DA2: DataAgent {
            parameters:
                @display("p=1030,400");
        }
        MA[total_mn]: MobileAgent {
            parameters:
                @display("p=150,400;");
                numRadios = default(1);
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
        CA.ethg$o++ --> ethernetline --> R_5.ethg$i++;
        CA.ethg$i++ <-- ethernetline <-- R_5.ethg$o++;
        DA1.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        DA1.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        DA2.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        DA2.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;        
        AP_1.ethg$o++ --> ethernetline --> R_1.ethg$i++;
        AP_1.ethg$i++ <-- ethernetline <-- R_1.ethg$o++;
        //        AP_2.ethg$o++ --> ethernetline --> R_2.ethg$i++;
        //        AP_2.ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        AP_3.ethg$o++ --> ethernetline --> R_3.ethg$i++;
        AP_3.ethg$i++ <-- ethernetline <-- R_3.ethg$o++;
        AP_4.ethg$o++ --> ethernetline --> R_4.ethg$i++;
        AP_4.ethg$i++ <-- ethernetline <-- R_4.ethg$o++;
        for i=0..total_cn-1 {
            CN1[i].ethg$o++ --> ethernetline --> R_2.ethg$i++;
            CN1[i].ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        }
        for i=0..total_cn-1 {
            CN2[i].ethg$o++ --> ethernetline --> R_2.ethg$i++;
            CN2[i].ethg$i++ <-- ethernetline <-- R_2.ethg$o++;
        }        
}
