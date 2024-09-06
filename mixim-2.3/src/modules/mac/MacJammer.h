/*
 * MacJammer.h
 *
 *  Created on: Jun 1, 2020
 *      Author: mostafa
 */

#ifndef MACJAMMER_H_
#define MACJAMMER_H_


#include <list>

#include "MiXiMDefs.h"
#include "BaseMacLayer.h"
#include "Consts80211.h"
#include "Mac80211Pkt_m.h"

class ChannelSenseRequest;

/**
 * @brief An implementation of the 802.11b MAC.
 *
 * For more info, see the NED file.
 *
 * @ingroup macLayer
 * @ingroup ieee80211
 * @author David Raguin, Karl Wessel (port for MiXiM)
 */
class MIXIM_API MacJammer : public BaseMacLayer
{
private:
    /** @brief Copy constructor is not allowed.
     */
    MacJammer(const MacJammer&);
    /** @brief Assignment operator is not allowed.
     */
    MacJammer& operator=(const MacJammer&);

public:




    /** @brief frame kinds */
    enum MacJammerMessageKinds {
      //between MAC layers of two nodes
      RTS = LAST_BASE_MAC_MESSAGE_KIND, // request to send
      Jammer,
       CTS,                 // clear to send
      ACK,                 // acknowledgement
      DATA,
      BROADCAST,
      LAST_MAC_80211_MESSAGE_KIND
    };

    enum BaseNetwMessageKinds {
        /** @brief Stores the id on which classes extending BaseNetw should
         * continue their own message kinds.*/
        LAST_BASE_NETW_MESSAGE_KIND = 24000,
        ReactJammerToNew,
    };


protected:
    /** @brief Type for a queue of Mac80211Pkts.*/
    typedef std::list<Mac80211Pkt*> MacPktList;

    /** Definition of the timer types */
    enum timerType {
      TIMEOUT,
      NAV
    };

    /** Definition of the states*/
    enum State {
      WFDATA = 0, // waiting for data packet
      QUIET = 1,  // waiting for the communication between two other nodes to end
      IDLE = 2,   // no packet to send, no packet receiving
      CONTEND = 3,// contention state (battle for the channel)
      WFCTS = 4,  // RTS sent, waiting for CTS
      WFACK = 5,  // DATA packet sent, waiting for ACK
      BUSY = 6    // during transmission of an ACK or a BROADCAST packet
    };





    /** @brief Data about a neighbor host.*/
    struct NeighborEntry {
        /** @brief The neighbors address.*/
        LAddress::L2Type address;
        int              fsc;
        simtime_t        age;
        double           bitrate;

        NeighborEntry() : address(), fsc(0), age(), bitrate(0) {}
    };

    /** @brief Type for a list of NeighborEntries.*/
    typedef std::list<NeighborEntry> NeighborList;

  public:
    MacJammer();
    virtual ~MacJammer();

    virtual void initialize(int);
    virtual void finish();

 protected:

    /** @brief Handle self messages such as timer... */
    virtual void handleSelfMsg(cMessage*);

    /** @brief Handle messages from upper layer */
  //  virtual void handleUpperMsg(cMessage* msg); Mostafa --? disable

    /** @brief Handle messages from lower layer */
    virtual void handleLowerMsg(cMessage*);

    /** @brief Handle messages from lower layer */
    virtual void handleLowerControl(cMessage*);

    /** @brief handle a message that was meant for me*/
    void handleMsgForMe(Mac80211Pkt*);
    // ** @brief handle a Broadcast message*/
    void handleBroadcastMsg(Mac80211Pkt*);

    /** @brief handle the end of a transmission...*/
    void handleEndTransmission();

    /** @brief Handle messages from upper layer */
        virtual void handleUpperMsg(cMessage* msg);




    /** @brief send data frame */
    virtual void sendDATAframe(Mac80211Pkt*);



    /** @brief send broadcast frame */
    void sendBROADCASTframe();

    /** @brief encapsulate packet */
    virtual macpkt_ptr_t encapsMsg(cPacket *netw);


    /** @brief start a new contention period */
    virtual void beginNewCycle();

    /** @brief Compute a backoff value */
    simtime_t backoff(bool rtscts = true);



    /** @brief return a timeOut value for a certain type of frame*/
    simtime_t timeOut(MacJammerMessageKinds type, double br);

    /** @brief computes the duration of a transmission over the physical channel, given a certain bitrate */
    simtime_t packetDuration(double bits, double br);

    /** @brief Produce a readable name of the given state */
    const char *stateName(State state);

    /** @brief Sets the state, and produces a log message in between */
    void setState(State state);

    /** @brief Check whether the next packet should be send with RTS/CTS */
    bool rtsCts(Mac80211Pkt* m) {
        return m->getBitLength() - MAC80211_HEADER_LENGTH > rtsCtsThreshold;
    }

    /** @brief suspend an ongoing contention, pick it up again when the channel becomes idle */
    void suspendContention();

    /** @brief figure out at which bitrate to send to this particular destination */
    double retrieveBitrate(const LAddress::L2Type& destAddress);

    /** @brief find a neighbor based on his address */
    NeighborList::iterator findNeighbor(const LAddress::L2Type& address)  {
        NeighborList::iterator it;
        for(it = neighbors.begin(); it != neighbors.end(); ++it) {
            if(it->address == address) break;
        }
        return it;
    }

    /** @brief find the oldest neighbor -- usually in order to overwrite this entry */
    NeighborList::iterator findOldestNeighbor() {
        NeighborList::iterator it = neighbors.begin();
        NeighborList::iterator oldIt = neighbors.begin();
        simtime_t age = it->age;
        for(; it != neighbors.end(); ++it) {
            if(it->age < age) {
                age = it->age;
                oldIt = it;
            }
        }
        return oldIt;
    }


    /**
     * @brief Starts a channel sense request which sense the channel for the
     * passed duration or until the channel is busy.
     *
     * Used during contend state to check if the channel is free.
     */
    void senseChannelWhileIdle(simtime_t_cref duration);

    /**
     * @brief Creates the signal to be used for a packet to be sent.
     */
    Signal* createSignal(simtime_t_cref start, simtime_t_cref length, double power, double bitrate);

protected:

    bool ReactiveJammer;

    // TIMERS:

    /** @brief Timer used for time-outs after the transmission of a RTS,
       a CTS, or a DATA packet*/
    cMessage* timeout;


    /** @brief Timer used for the defer time of a node. Also called NAV :
       networks allocation vector*/
    cMessage* nav;

    /** @brief Used to sense if the channel is idle for contention periods*/
    ChannelSenseRequest* contention;

    /** @brief Timer used to indicate the end of a SIFS*/
    ChannelSenseRequest* endSifs;

    /** @brief Stores the the time a channel sensing started.
     * Used to calculate the quiet-time of the channel if the sensing was
     * aborted. */
    simtime_t chSenseStart;

    /** @brief Current state of the MAC*/
    State state;

    /** @brief Default bitrate
     *
     * The default bitrate must be set in the omnetpp.ini. It is used
     * whenever an auto bitrate is not appropriate, like broadcasts.
     */
    double defaultBitrate;

    /** @brief The power at which data is transmitted */
    double txPower;

    /** @brief Stores the center frequency the Mac uses. */
    double centerFreq;

    /** @brief Current bit rate at which data is transmitted */
    double bitrate;
    /** @brief Auto bit rate adaptation -- switch */
    bool autoBitrate;
    /** @brief Hold RSSI thresholds at which to change the bitrates */
    std::vector<double> snrThresholds;

    /** @brief Maximal number of packets in the queue; should be set in
       the omnetpp.ini*/
    unsigned queueLength;

    /** @brief Boolean used to know if the next packet is a broadcast packet.*/
    bool nextIsBroadcast;

    /** @brief Buffering of messages from upper layer*/
    MacPktList fromUpperLayer;

    /** @brief Number of frame transmission attempt
     *
     *  Incremented when the SHORT_RETRY_LIMIT is hit, or when an ACK
     *  or CTS is missing.
     */
    unsigned longRetryCounter;

    /** @brief Number of frame transmission attempt*/
    unsigned shortRetryCounter;

    /** @brief remaining backoff time.
     * If the backoff timer is interrupted,
     * this variable holds the remaining backoff time. */
    simtime_t remainingBackoff;

    /** @brief current IFS value (DIFS or EIFS)
     * If an error has been detected, the next backoff requires EIFS,
     * once a valid frame has been received, resets to DIFS. */
    simtime_t currentIFS;

    /** @brief Number of bits in a packet before RTS/CTS is used */
    int rtsCtsThreshold;

    /** @brief Very small value used in timer scheduling in order to avoid
       multiple changements of state in the same simulation time.*/
    simtime_t delta;

    /** @brief Keep information for this many neighbors */
    unsigned neighborhoodCacheSize;
    /** @brief Consider information in cache outdate if it is older than this */
    simtime_t neighborhoodCacheMaxAge;

    /** @brief A list of this hosts neighbors.*/
    NeighborList neighbors;

    /** take care of switchover times */
    bool switching;

    /** sequence control -- to detect duplicates*/
    int fsc;
};



#endif /* MACJAMMER_H_ */
