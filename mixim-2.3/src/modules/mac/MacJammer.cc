/*
 * MacJammer.cc
 *
 *  Created on: Jun 1, 2020
 *      Author: mostafa
 */



#include "mac/MacJammer.h"

#include "PhyToMacControlInfo.h"
#include "FWMath.h"
#include "phy/Decider80211.h"
#include "DeciderResult80211.h"
#include "BaseConnectionManager.h"
#include "MacToPhyInterface.h"
#include "ChannelSenseRequest_m.h"
#include "NetwPkt_m.h" //Mostafa added
using namespace std;

Define_Module(MacJammer);

MacJammer::MacJammer()
: BaseMacLayer()
, timeout()
, nav(NULL)
, contention(NULL)
, endSifs(NULL)
, chSenseStart()
, state()
, defaultBitrate(0)
, txPower(0)
, centerFreq(0)
, bitrate(0)
, autoBitrate(false)
, snrThresholds()
, queueLength(0)
, nextIsBroadcast(false)
, fromUpperLayer()
, longRetryCounter(0)
, shortRetryCounter(0)
, remainingBackoff()
, currentIFS()
, rtsCtsThreshold(0)
, delta()
, neighborhoodCacheSize(0)
, neighborhoodCacheMaxAge()
, neighbors()
, switching(false)
, fsc(0)
{}

void MacJammer::initialize(int stage)
{
    BaseMacLayer::initialize(stage);

    if (stage == 0)
    {
        debugEV << "Initializing stage 0\n";

        switching = false;
        fsc = intrand(0x7FFFFFFF);
        if(fsc == 0) fsc = 1;
        debugEV << " fsc: " << fsc << "\n";

        queueLength = hasPar("queueLength") ? par("queueLength").longValue() : 10;
        ReactiveJammer=par("ReactiveJammer").boolValue();

        ev<<" mostafa   ReactiveJammer "<<ReactiveJammer<<endl;
        // timers
        timeout = new cMessage("timeout", TIMEOUT);
        nav = new cMessage("NAV", NAV);
        contention = new ChannelSenseRequest("contention", MacToPhyInterface::CHANNEL_SENSE_REQUEST);
        contention->setSenseMode(UNTIL_BUSY);
        endSifs = new ChannelSenseRequest("end SIFS", MacToPhyInterface::CHANNEL_SENSE_REQUEST);
        endSifs->setSenseMode(UNTIL_BUSY);
        endSifs->setSenseTimeout(SIFS);

        state = IDLE;
        longRetryCounter = 0;
        shortRetryCounter = 0;
        rtsCtsThreshold = hasPar("rtsCtsThreshold") ? par("rtsCtsThreshold").longValue() : 1;
        currentIFS = EIFS;

        autoBitrate = hasPar("autoBitrate") ? par("autoBitrate").boolValue() : false;

        txPower = hasPar("txPower") ? par("txPower").doubleValue() : 110.11;

        delta = 1E-9;

        debugEV << "SIFS: " << SIFS << " DIFS: " << DIFS << " EIFS: " << EIFS << endl;
    }
    else if(stage == 1) {
        BaseConnectionManager* cc = getConnectionManager();

        if(cc->hasPar("pMax") && txPower > cc->par("pMax").doubleValue())
            opp_error("TranmitterPower can't be bigger than pMax in ConnectionManager! "
                    "Please adjust your omnetpp.ini file accordingly.");

        int channel = phy->getCurrentRadioChannel();
        if(!(1<=channel && channel<=14)) {
            opp_error("MiximRadio set to invalid channel %d. Please make sure the"
                    " phy modules parameter \"initialRadioChannel\" is set to"
                    " a valid 802.11 channel (1 to 14)!", channel);
        }
        centerFreq = CENTER_FREQUENCIES[channel];

        bool found = false;
        bitrate = hasPar("bitrate") ? par("bitrate").doubleValue() : BITRATES_80211[0];
        for(int i = 0; i < 4; i++) {
            if(bitrate == BITRATES_80211[i]) {
                found = true;
                break;
            }
        }
        if(!found) bitrate = BITRATES_80211[0];
        defaultBitrate = bitrate;

        snrThresholds.push_back(hasPar("snr2Mbit") ? par("snr2Mbit").doubleValue() : 100);
        snrThresholds.push_back(hasPar("snr5Mbit") ? par("snr5Mbit").doubleValue() : 100);
        snrThresholds.push_back(hasPar("snr11Mbit") ? par("snr11Mbit").doubleValue() : 100);
        snrThresholds.push_back(111111111); // sentinel

        neighborhoodCacheSize = hasPar("neighborhoodCacheSize") ? par("neighborhoodCacheSize").longValue() : 0;
        neighborhoodCacheMaxAge = hasPar("neighborhoodCacheMaxAge") ? par("neighborhoodCacheMaxAge").longValue() : 10000;

        debugEV << " MAC Address: " << myMacAddr
                << " rtsCtsThreshold: " << rtsCtsThreshold
                << " bitrate: " << bitrate
                << " channel: " << channel
                << " autoBitrate: " << autoBitrate
                << " 2MBit: " << snrThresholds[0]
                                               << " 5.5MBit: " <<snrThresholds[1]
                                                                               << " 11MBit: " << snrThresholds[2]
                                                                                                               << " neighborhoodCacheSize " << neighborhoodCacheSize
                                                                                                               << " neighborhoodCacheMaxAge " << neighborhoodCacheMaxAge
                                                                                                               << endl;

        for(int i = 0; i < 3; i++) {
            snrThresholds[i] = FWMath::dBm2mW(snrThresholds[i]);
        }

        remainingBackoff = backoff();
        senseChannelWhileIdle(remainingBackoff + currentIFS);

        //Mostafa added
        // scheduleAt(simTime()+0.002 , ReactJammer);
    }
}

void MacJammer::senseChannelWhileIdle(simtime_t_cref duration) {
    if(contention->isScheduled()) {
        error("Cannot start a new channel sense request because already sensing the channel!");
    }

    chSenseStart = simTime();
    contention->setSenseTimeout(duration);

    sendControlDown(contention);
}

/**
 * This implementation does not support fragmentation, so it is tested
 * if the maximum length of the MPDU is exceeded.
 */
void MacJammer::handleUpperMsg(cMessage *msg)
{
    cPacket* jampk = static_cast<cPacket*>(msg);

    Mac80211Pkt* pkt = static_cast<Mac80211Pkt*>(encapsMsg(jampk));

    ev<<" MacJammer::handleUpperMsg(cMessage *msg) "<<endl;
    ev<<"pkt name = "<<pkt->getName()<<endl;
    ev<<"pkt name = "<<pkt->getBitLength()<<endl;


    double br = defaultBitrate;

    simtime_t duration = packetDuration(pkt->getBitLength(), br);
    setDownControlInfo(pkt, createSignal(simTime(), duration, txPower, br));

    phy->setRadioState(MiximRadio::TX);

    sendDown(pkt);

    // delete msg;

}

/**
 * Encapsulates the received network-layer packet into a MacPkt and set all needed
 * header fields.
 */
MacJammer::macpkt_ptr_t MacJammer::encapsMsg(cPacket* netw)
{

    Mac80211Pkt *pkt = new Mac80211Pkt(netw->getName());
    // headerLength, including final CRC-field AND the phy header length!
    pkt->setBitLength(MAC80211_HEADER_LENGTH);
    pkt->setRetry(false);                 // this is not a retry
    pkt->setSequenceControl(fsc++);       // add a unique fsc to it
    if(fsc <= 0) fsc = 1;

    pkt->setKind(Jammer);
    pkt->setDestAddr((LAddress::L2Type)1000);

    pkt->setSrcAddr(myMacAddr);

    //encapsulate the network packet
    pkt->encapsulate(netw);
    debugEV <<"pkt encapsulated, length: " << pkt->getBitLength() << "\n";

    return pkt;
}



/**
 *  Handle all messages from lower layer. Checks the destination MAC
 *  adress of the packet. Then calls one of the three functions :
 *  handleMsgNotForMe(), handleBroadcastMsg(), or handleMsgForMe().
 *  Called by handleMessage().
 */
void MacJammer::handleLowerMsg(cMessage *msg)
{
    ev<<"MacJammer::handleLowerMsg "<<endl;
    Mac80211Pkt *af = static_cast<Mac80211Pkt *>(msg);

    ev<<" Mostafa MacJammer::handleLowerMsg getName"<< msg->getName()<<endl;
    ev<<" Mostafa MacJammer::handleLowerMsg getKind"<< msg->getKind()<<endl;
    ev<<" Mostafa MacJammer::handleLowerMsg dup"<< msg->dup()<<endl;
    ev<<" Mostafa MacJammer::handleLowerMsg getNamePooling"<< msg->getNamePooling()<<endl;



    //std::string str1="wlan-rts";
   // std::string str1="BROADCAST_MESSAGE";
    //ev<<" Show result str1.compare(msg->getName()) "<<str1.compare(msg->getName())<<endl;

//    if((simTime()>=1.0 && simTime()<=2.0) || (simTime()>=3.0 && simTime()<=4.0) || (simTime()>=6.0 && simTime()<=7.0) || (simTime()>=8.0 && simTime()<=9.0) )
//    {
////        if(str1.compare(msg->getName())==0)
////        {
//            cPacket *Jampk = new cPacket("Jammer Packet");
//            Jampk->setBitLength(6000);
//            handleUpperMsg(Jampk);
////}
//   }//end if
    delete msg;

    currentIFS = DIFS;

}

void MacJammer::handleLowerControl(cMessage *msg)
{

    ev<<" Mostafa MacJammer::handleLowerControl getName"<< msg->getName()<<endl;
    ev<<" Mostafa MacJammer::handleLowerControl getKind"<< msg->getKind()<<endl;
    ev<<" Mostafa MacJammer::handleLowerControl dup"<< msg->dup()<<endl;
    ev<<" Mostafa MacJammer::handleLowerControl getNamePooling"<< msg->getNamePooling()<<endl;


    switch(msg->getKind()) {
    case MacToPhyInterface::CHANNEL_SENSE_REQUEST:
        if(msg == contention ) {
            if(!contention->getSenseMode()==2)//2==busy
            {
                ev<<"    if(!contention->getSenseMode()==2)"<<endl;
            }

        }
        else {
            error("Unknown ChannelSenseRequest returned!");
        }
        break;

    case MacToPhyInterface::JammerBusyChannel:


//                if(ReactiveJammer==true){
//
//                    if((simTime()>=1.0 && simTime()<=2.0) || (simTime()>=3.0 && simTime()<=4.0) || (simTime()>=6.0 && simTime()<=7.0) || (simTime()>=8.0 && simTime()<=9.0) )
//                    {
//                        cPacket *Jampk = new cPacket("Jammer Packet");
//                        Jampk->setBitLength(6000);
//                        handleUpperMsg(Jampk);
//                    }
//
//                }//end if(ReactiveJammer==true)
//


        ev<<"     case MacToPhyInterface::JammerBusyChannel:"<<endl;
        break;

    case MacToPhyInterface::TX_OVER:

        phy->setRadioState(MiximRadio::RX);

        break;

    }
    delete msg;

}

/**
 * handle timers
 */
void MacJammer::handleSelfMsg(cMessage * msg)
{
    debugEV << simTime() << " handleSelfMsg " << msg->getName() << "\n";
    switch (msg->getKind())
    {
    // the MAC was waiting for a CTS, a DATA, or an ACK packet but the timer has expired.
    case TIMEOUT:
        // handleTimeoutTimer();   // noch zu betrachten..
        break;


    default:
        error("unknown timer type");
        break;
    }
}


/**
 *  Handle a packet for the node. The result of this reception is
 *  a function of the type of the received message (RTS,CTS,DATA,
 *  or ACK), and of the current state of the MAC (WFDATA, CONTEND,
 *  IDLE, WFCTS, or WFACK). Called by handleLowerMsg()
 */
void MacJammer::handleMsgForMe(Mac80211Pkt *af)
{
    delete af;

}




void MacJammer::handleBroadcastMsg(Mac80211Pkt *af)
{
    debugEV << "handle broadcast\n";
    if((state == BUSY) && (!switching)) {
        error("logic error: node is currently transmitting, can not receive "
                "(does the physical layer do its job correctly?)");
    }
    //sendUp(decapsMsg(af)); Mostafa Disable
    delete af;
    if (state == CONTEND) {
        assert(!contention->isScheduled());
        //suspendContention();

        beginNewCycle();
    }
}


/**
 *  Handle the end of transmission timer (end of the transmission
 *  of an ACK or a broadcast packet). Called by
 *  HandleTimer(cMessage* msg)
 */
void MacJammer::handleEndTransmission()
{
    debugEV << "transmission of packet is over\n";
    if(state == BUSY) {
        if(nextIsBroadcast) {
            shortRetryCounter = 0;
            longRetryCounter = 0;
            remainingBackoff = backoff();
        }
        beginNewCycle();
    }
    else if(state == WFDATA) {
        beginNewCycle();
    }
}

/**
 *  Send a DATA frame. Called by HandleEndSifsTimer() or
 *  handleEndContentionTimer()
 */
void MacJammer::sendDATAframe(Mac80211Pkt *af)
{
    Mac80211Pkt *frame = static_cast<Mac80211Pkt *>(fromUpperLayer.front()->dup());
    double br;

    if(af) {
        br = static_cast<const DeciderResult80211*>(PhyToMacControlInfo::getDeciderResult(af))->getBitrate();

        delete af->removeControlInfo();
    }
    else {
        br  = retrieveBitrate(frame->getDestAddr());

        if(shortRetryCounter) frame->setRetry(true);
    }
    simtime_t duration = packetDuration(frame->getBitLength(), br);
    setDownControlInfo(frame, createSignal(simTime(), duration, txPower, br));
    // build a copy of the frame in front of the queue'
    frame->setSrcAddr(myMacAddr);
    frame->setKind(DATA);
    frame->setDuration(SIFS + packetDuration(LENGTH_ACK, br));

    // schedule time out
    scheduleAt(simTime() + timeOut(DATA, br), timeout);
    debugEV << "sending DATA  to " << frame->getDestAddr() << " with bitrate " << br << endl;
    // send DATA frame
    sendDown(frame);

    // update state and display
    setState(WFACK);
}





/**
 *  Send a BROADCAST frame.Called by handleContentionTimer()
 */
void MacJammer::sendBROADCASTframe()
{
    // send a copy of the frame in front of the queue
    Mac80211Pkt *frame = static_cast<Mac80211Pkt *>(fromUpperLayer.front()->dup());

    double br = retrieveBitrate(frame->getDestAddr());

    simtime_t duration = packetDuration(frame->getBitLength(), br);
    setDownControlInfo(frame, createSignal(simTime(), duration, txPower, br));

    frame->setKind(BROADCAST);

    sendDown(frame);
    // update state and display
    setState(BUSY);
}

/**
 *  Start a new contention period if the channel is free and if
 *  there's a packet to send.  Called at the end of a deferring
 *  period, a busy period, or after a failure. Called by the
 *  HandleMsgForMe(), HandleTimer() HandleUpperMsg(), and, without
 *  RTS/CTS, by handleMsgNotForMe().
 */
void MacJammer::beginNewCycle()
{
    // before trying to send one more time a packet, test if the
    // maximum retry limit is reached. If it is the case, then
    // delete the packet and send the next packet.

    if (nav->isScheduled()) {
        debugEV << "cannot beginNewCycle until NAV expires at t " << nav->getArrivalTime() << endl;
        return;
    }

    /*
    if(timeout->isScheduled()) {
        cancelEvent(timeout);
    }
     */

    if (!fromUpperLayer.empty()) {

        // look if the next packet is unicast or broadcast
        nextIsBroadcast = LAddress::isL2Broadcast(fromUpperLayer.front()->getDestAddr());

        setState(CONTEND);
        if(!contention->isScheduled()) {
            ChannelState channel = phy->getChannelState();
            debugEV << simTime() << " do contention: medium = " << channel.info() << ", backoff = "
                    <<  remainingBackoff << endl;

            if(channel.isIdle()) {
                senseChannelWhileIdle(currentIFS + remainingBackoff);
                //scheduleAt(simTime() + currentIFS + remainingBackoff, contention);
            }
            else {
                /*
                Mac80211 in MiXiM uses the same mechanism for backoff and post-backoff, a senseChannelWhileIdle which
                schedules a timer for a duration of IFS + remainingBackoff (i.e. The inter-frame spacing and the present
                state of the backoff counter).

                If Host A were doing post-backoff when the frame from Host B arrived, the remainingBackoff would have
                been > 0 and backoff would have resumed after the frame from Host B finishes.

                However, Host A has already completed its post-backoff (remainingBackoff was 0) so it essentially was
                IDLE when the beacon was generated (actually, it was receiving Host B’s frame). So what happens now is
                that all nodes which have an arrival during Host B’s frame AND have completed their post-backoff, will
                wait one IFS and then transmit, resulting in synchronised collisions one IFS after a transmission (or
                an EIFS after a collision).

                The correct behaviour here is for Host A’s MAC to note that post-backoff has completed (remainingBackoff
                has reached 0) and the medium is busy, so the MAC must draw a new backoff from the contention window.

                http://www.freeminded.org/?p=801
                 */
                //channel is busy
                if(remainingBackoff==0) {
                    remainingBackoff = backoff();
                }
            }
        }
    }
    else {
        // post-xmit backoff (minor nit: if random backoff=0, we punt)
        if(remainingBackoff > 0 && !contention->isScheduled()) {
            ChannelState channel = phy->getChannelState();
            debugEV << simTime() << " do contention: medium = " << channel.info() << ", backoff = "
                    <<  remainingBackoff << endl;

            if(channel.isIdle()) {
                senseChannelWhileIdle(currentIFS + remainingBackoff);
                //scheduleAt(simTime() + currentIFS + remainingBackoff, contention);
            }
        }
        setState(IDLE);
    }
}

/**
 * Compute the backoff value.
 */
simtime_t MacJammer::backoff(bool rtscts) {
    unsigned rc = (rtscts) ?  longRetryCounter : shortRetryCounter;
    unsigned cw = ((CW_MIN + 1) << rc) - 1;
    if(cw > CW_MAX) cw = CW_MAX;

    simtime_t value = ((double) intrand(cw + 1)) * ST;
    debugEV << simTime() << " random backoff = " << value << endl;

    return value;
}


Signal* MacJammer::createSignal( simtime_t_cref start, simtime_t_cref length,
        double power, double bitrate)
{
    simtime_t end = start + length;
    //create signal with start at current simtime and passed length
    Signal* s = new Signal(start, length);

    //create and set tx power mapping
    ConstMapping* txPowerMapping
    = createSingleFrequencyMapping( start, end,
            centerFreq, 11.0e6,
            power);
    s->setTransmissionPower(txPowerMapping);

    //create and set bitrate mapping

    //create mapping over time
    Mapping* bitrateMapping
    = MappingUtils::createMapping(DimensionSet::timeDomain,
            Mapping::STEPS);

    Argument pos(start);
    bitrateMapping->setValue(pos, BITRATE_HEADER);

    pos.setTime(PHY_HEADER_LENGTH / BITRATE_HEADER);
    bitrateMapping->setValue(pos, bitrate);

    s->setBitrate(bitrateMapping);

    return s;
}

/**
 *  Return a time-out value for a type of frame. Called by
 *  SendRTSframe, sendCTSframe, etc.
 */
simtime_t MacJammer::timeOut(MacJammerMessageKinds type, double br)
{
    simtime_t time_out = 0;

    switch (type)
    {
    case RTS:
        time_out = SIFS + packetDuration(LENGTH_RTS, br) + ST + packetDuration(LENGTH_CTS, br) + delta;
        debugEV << " Mac80211::timeOut RTS " << time_out << "\n";
        break;
    case DATA:
        time_out = SIFS + packetDuration(fromUpperLayer.front()->getBitLength(), br) + ST + packetDuration(LENGTH_ACK, br) + delta;
        debugEV << " Mac80211::timeOut DATA " << time_out << "\n";
        break;
    default:
        EV << "Unused frame type was given when calling timeOut(), this should not happen!\n";
        break;
    }
    return time_out;
}

/**
 * Computes the duration of the transmission of a frame over the
 * physical channel. 'bits' should be the total length of the
 * mac packet in bits excluding the phy header length.
 */
simtime_t MacJammer::packetDuration(double bits, double br)
{
    return bits / br + phyHeaderLength / BITRATE_HEADER;
}

const char *MacJammer::stateName(State state)
{
#define CASE(x) case x: s=#x; break
    const char *s = "???";
    switch (state)
    {
    CASE(WFDATA);
    CASE(QUIET);
    CASE(IDLE);
    CASE(CONTEND);
    CASE(WFCTS);
    CASE(WFACK);
    CASE(BUSY);
    }
    return s;
#undef CASE
}

void MacJammer::setState(State newState)
{
    if (state==newState)
        debugEV << "staying in state " << stateName(state) << "\n";
    else
        debugEV << "state " << stateName(state) << " --> " << stateName(newState) << "\n";
    state = newState;
}

void MacJammer::suspendContention()  {
    assert(!contention->isScheduled());
    // if there's a contention period

    //if(requestReturned || chSenseRequest->isScheduled()) {
    // update the backoff window in order to give higher priority in
    // the next battle

    simtime_t quietTime = simTime() - chSenseStart;

    debugEV << simTime() << " suspend contention: "
            << "began " << chSenseStart
            << ", ends " << chSenseStart + contention->getSenseTimeout()
            << ", ifs " << currentIFS
            << ", quiet time " << quietTime
            << endl;

    if(quietTime < currentIFS) {
        debugEV << "suspended during D/EIFS (no backoff)" << endl;
    }
    else {
        double remainingSlots;
        remainingSlots = SIMTIME_DBL(contention->getSenseTimeout() - quietTime)/ST;

        // Distinguish between (if) case where contention is
        // suspended after an integer number of slots and we
        // _round_ to integer to avoid fp error, and (else) case
        // where contention is suspended mid-slot (e.g. hidden
        // terminal xmits) and we _ceil_ to repeat the partial
        // slot.  Arbitrary value 0.0001 * ST is used to
        // distinguish the two cases, which may a problem if clock
        // skew between nic's is ever implemented.

        if (fabs(ceil(remainingSlots) - remainingSlots) < 0.0001 * ST ||
                fabs(floor(remainingSlots) - remainingSlots) < 0.0001 * ST) {
            remainingBackoff = floor(remainingSlots + 0.5) * ST;
        }
        else {
            remainingBackoff = ceil(remainingSlots) * ST;
        }

        debugEV << "backoff was " << ((contention->getSenseTimeout() - currentIFS))/ST
                << " slots, now " << remainingSlots << " slots remain" << endl;
    }

    debugEV << "suspended backoff timer, remaining backoff time: "
            << remainingBackoff << endl;

    //}
}

double MacJammer::retrieveBitrate(const LAddress::L2Type& destAddress) {
    double bitrate = defaultBitrate;
    NeighborList::iterator it;
    if(autoBitrate && !LAddress::isL2Broadcast(destAddress) &&
            (longRetryCounter == 0) && (shortRetryCounter == 0)) {
        it = findNeighbor(destAddress);
        if((it != neighbors.end()) && (it->age > (simTime() - neighborhoodCacheMaxAge))) {
            bitrate = it->bitrate;
        }
    }
    return bitrate;
}



MacJammer::~MacJammer() {
    cancelAndDelete(timeout);
    cancelAndDelete(nav);
    if(contention && !contention->isScheduled())
        delete contention;
    if(endSifs && !endSifs->isScheduled())
        delete endSifs;

    MacPktList::iterator it;
    for(it = fromUpperLayer.begin(); it != fromUpperLayer.end(); ++it) {
        delete (*it);
    }
    fromUpperLayer.clear();
}

void MacJammer::finish() {
    BaseMacLayer::finish();
}


