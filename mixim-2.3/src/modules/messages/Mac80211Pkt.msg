//**************************************************************************
// * file:        Mac80211Pkt.msg
// *
// * author:      Marc L�bbers
// * copyright:   (c) by 'the Glimmer Twins'
// *              Telecommunication Networks Group
// *              TU-Berlin
// * email:       loebbers@tkn.tu-berlin.de
// **************************************************************************
// * part of:     framework implementation developed by tkn
// *************************************************************************/

cplusplus {{
#include "MiXiMMacPkt.h"
}}


class MacPkt;

//
// Defines all fields of an 802.11 MAC frame
//
packet Mac80211Pkt extends MacPkt
{
    int address3;
    int address4;
    int multicast_MAC[50];
    int Multicast_Nodes;
    bool Multicast_or_Not;
    int fragmentation; //part of the Frame Control field
    int informationDS; //part of the Frame Control field
    int sequenceControl;
    bool retry;
    simtime_t duration; 	//the expected remaining duration the current transaction 
						//(RTS->CTS->DATA->ACK) between two macs will occupy the channel 
}
