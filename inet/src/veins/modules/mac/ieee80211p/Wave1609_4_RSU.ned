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

package veins.modules.mac.ieee80211p;

import veins.base.modules.BaseMacLayer;

simple Wave1609_4_RSU extends BaseMacLayer
{
    parameters:
        @class(Wave1609_4);

        string ssid = default("RSU_AP");
		double beaconInterval @unit("s") = default(100ms);
		string macModule;               // The path to the MAC module
		
        // cycle between an SCH and the CCH (or stay on CCH all the time)
        bool useServiceChannel = default(true);

        // the service channel this maclayer listens on
        int serviceChannel = default(1);
		
		// set the queue to be used
		int queue = default(0);
		
        // debug switch
        bool debug = default(false);

        // length of MAC header, header is already added at 1609_4
        headerLength @unit(bit) = default(0 bit);

        // bit rate 
        double bitrate @unit(bps) = default(18 Mbps);

        // little asynchronization between cars
        double syncOffset @unit(s) = default(0.0003s);

        //tx power [mW]
        double txPower @unit(mW);

        int queueSize = default(0);

        // signal informing interested application about channel busy state
        @signal[sigChannelBusy](type=bool);
        // signal informing interested application about a collision
        @signal[sigCollision](type=bool);
        
//        @signal[NF_L2_AP_ASSOCIATED](type="Ieee80211MgmtAP::NotificationInfoSta");
//        @signal[NF_L2_AP_DISASSOCIATED](type="Ieee80211MgmtAP::NotificationInfoSta");
}
