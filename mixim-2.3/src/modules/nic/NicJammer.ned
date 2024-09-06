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

package org.mixim.modules.nic;


import org.mixim.modules.mac.MacJammer;
import org.mixim.modules.phy.PhyLayer;

//
// This NIC implements an 802.11 network interface card.
//
// @see Mac80211, Decider80211
// @author Marc Loebbers, Karl Wessel (port for MiXiM)
//
module NicJammer extends WirelessNic
{
    parameters:
    	@display("i=block/ifcard");
    	macType = "MacJammer";
    	
    	mac.queueLength = default(20);
        mac.bitrate     = default(2E+6bps);            
        mac.autoBitrate = default(false);
        // values if no fading is modelled, gives at most 1% packet error rate
        mac.snr2Mbit  = default(1.46dB); 
        mac.snr5Mbit  = default(2.6dB);
        mac.snr11Mbit = default(5.68dB);
        mac.neighborhoodCacheSize   = default(30);
        mac.neighborhoodCacheMaxAge = default(100s);
        mac.txPower = default(110.11mW);
    	
    	phy.headerLength        = 192bit;
        phy.sensitivity         = default(-119.5dBm);
        phy.decider             = default(xmldoc("Nic80211_Decider.xml"));
        phy.initialRadioChannel = default(1);
        phy.nbRadioChannels     = 15; //1 to 14, 0 is invalid
        //Mac80211 does not support switching times
        phy.timeRXToTX    = 0s;
        phy.timeRXToSleep = 0s;
        phy.timeTXToRX    = 0s;
        phy.timeTXToSleep = 0s;
        phy.timeSleepToRX = 0s;
        phy.timeSleepToTX = 0s;
}

