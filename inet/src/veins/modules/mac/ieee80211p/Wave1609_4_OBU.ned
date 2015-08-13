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

simple Wave1609_4_OBU extends BaseMacLayer
{
	parameters:
        @class(Wave1609_4);
        string rsuMacAddress; // MAC address of associate AP
        bool useServiceChannel = default(true); // cycle between an SCH and the CCH (or stay on CCH all the time)
        int serviceChannel = default(1); // the service channel this maclayer listens on
		int queue = default(0); // set the queue to be used
        
        bool debug = default(false); // debug switch
        headerLength @unit(bit) = default(0 bit); // length of MAC header, header is already added at 1609_4
        double bitrate @unit(bps) = default(18 Mbps); // bit rate 
        double syncOffset @unit(s) = default(0.0003s); // little asynchronization between cars
        double txPower @unit(mW); //tx power [mW]
        int queueSize = default(0);
        @signal[sigChannelBusy](type=bool); // signal informing interested application about channel busy state
        @signal[sigCollision](type=bool); // signal informing interested application about a collision
    
}
