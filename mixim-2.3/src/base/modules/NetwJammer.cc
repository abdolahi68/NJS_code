/*
 * NetwJammer.cc
 *
 *  Created on: Jun 2, 2020
 *      Author: mostafa
 */



#include "NetwJammer.h"

#include <cassert>

#include "NetwControlInfo.h"
#include "BaseMacLayer.h"
#include "AddressingInterface.h"
#include "SimpleAddress.h"
#include "FindModule.h"
#include "NetwPkt_m.h"
#include "ArpInterface.h"
#include "NetwToMacControlInfo.h"

Define_Module(NetwJammer);


NetwJammer::NetwJammer()
: BaseLayer()
, headerLength(0)
, Timer(NULL)
, arp(NULL)
, myNetwAddr()
, coreDebug(false)
{}

void NetwJammer::initialize(int stage)
{
    ev<<"- - - - - NetwJammer::initialize(int stage) - - - - - -"<<endl;

    BaseLayer::initialize(stage);

    if(stage==0){

        ProactiveJammer=par("ProactiveJammer").boolValue();
        ev<<" mostafa   ProactiveJammer "<<ProactiveJammer<<endl;

        coreDebug = par("coreDebug").boolValue();
        headerLength= par("headerLength");
        arp = FindModule<ArpInterface*>::findSubModule(findHost());
    }
    else if(stage == 1) {
        // see if there is an addressing module available
        // otherwise use module id as network address
        AddressingInterface* addrScheme = FindModule<AddressingInterface*>::findSubModule(findHost());
        if(addrScheme) {
            myNetwAddr = addrScheme->myNetwAddr(this);
        } else {
            myNetwAddr = LAddress::L3Type( getId() );
        }
        ev<< " myNetwAddr " << myNetwAddr << std::endl;
        ev<<"my index"<<getParentModule()->getIndex()<<endl;


        Timer = new cMessage( "timer", Time_Jammer );
        if(ProactiveJammer==true  && getParentModule()->getIndex()==0){
            scheduleAt(simTime()+1.0 , Timer); //Mostafa Disable temporary
        }
        else if(ProactiveJammer==true  && getParentModule()->getIndex()==1){
            scheduleAt(simTime()+3.0 , Timer); //Mostafa Disable temporary
        }

    }


}

void NetwJammer::handleSelfMsg(cMessage* msg)
{
    switch( msg->getKind() ) {
    case Time_Jammer:
        ev<<" Mostafa  - - - Time_Jammer - - - - ! ! !"<<endl;
        if(((simTime()>=1.0 && simTime()<=2.0) || (simTime()>=3.0 && simTime()<=4.0) || (simTime()>=5.0 && simTime()<=6.0) || (simTime()>=7.0 && simTime()<=8.0) ) && getParentModule()->getIndex()==0)
        {
            SendJammer();
           if( ((simTime()+0.004) >2.0 && (simTime()+0.004) <3.0) || ((simTime()+0.004) >4.0 && (simTime()+0.004) <5.0)  || ((simTime()+0.004) >6.0 && (simTime()+0.004) <7.0) || ((simTime()+0.004) >8.0 && (simTime()+0.004) <9.0) )
            scheduleAt(simTime()+uniform(0.004,0.005)+1.0 , Timer); //Mostafa Disable temporary
           else
               scheduleAt(simTime()+uniform(0.004,0.005) , Timer); //Mostafa Disable temporary
        }
        else if(simTime()<4 && getParentModule()->getIndex()==1)
        {
            SendJammer();
            scheduleAt(simTime()+uniform(0.007,0.008) , Timer); //Mostafa Disable temporary
        }
        else
            delete msg;
        break;
    default:
        EV << "Invalid Kind .... !!! "<<msg->getKind() <<endl;
        delete msg;
        break;
    }

}

/**
 * Decapsulates the packet from the received Network packet
 **/
cMessage* NetwJammer::decapsMsg(netwpkt_ptr_t msg)
{
    cMessage *m = msg->decapsulate();
    setUpControlInfo(m, msg->getSrcAddr());
    // delete the netw packet
    delete msg;
    return m;
}


void NetwJammer::SendJammer()
{
    cPacket *Jampk = new cPacket("Jammer Packet");
    Jampk->setBitLength(6000);

    sendDown(Jampk);
}

/**
 * Encapsulates the received ApplPkt into a NetwPkt and set all needed
 * header fields.
 **/
NetwJammer::netwpkt_ptr_t NetwJammer::encapsMsg(cPacket *appPkt) {
    LAddress::L2Type macAddr;
    LAddress::L3Type netwAddr;

    coreEV <<"in encaps...\n";

    netwpkt_ptr_t pkt = new NetwPkt(appPkt->getName(), appPkt->getKind());
    pkt->setBitLength(headerLength);

    cObject* cInfo = appPkt->removeControlInfo();

    if(cInfo == NULL){
        EV << "warning: Application layer did not specifiy a destination L3 address\n"
                << "\tusing broadcast address instead\n";
        netwAddr = LAddress::L3BROADCAST;
    } else {
        coreEV <<"CInfo removed, netw addr="<< NetwControlInfo::getAddressFromControlInfo( cInfo ) << std::endl;
        netwAddr = NetwControlInfo::getAddressFromControlInfo( cInfo );
        delete cInfo;
    }

    pkt->setSrcAddr(myNetwAddr);
    pkt->setDestAddr(netwAddr);
    coreEV << " netw "<< myNetwAddr << " sending packet" <<std::endl;
    if(LAddress::isL3Broadcast( netwAddr )) {
        coreEV << "sendDown: nHop=L3BROADCAST -> message has to be broadcasted"
                << " -> set destMac=L2BROADCAST\n";
        macAddr = LAddress::L2BROADCAST;
    }
    else{
        coreEV <<"sendDown: get the MAC address\n";
        macAddr = arp->getMacAddr(netwAddr);
    }

    setDownControlInfo(pkt, macAddr);

    //encapsulate the application packet
    pkt->encapsulate(appPkt);
    coreEV <<" pkt encapsulated\n";
    return pkt;
}

/**
 * Redefine this function if you want to process messages from lower
 * layers before they are forwarded to upper layers
 *
 *
 * If you want to forward the message to upper layers please use
 * @ref sendUp which will take care of decapsulation and thelike
 **/
void NetwJammer::handleLowerMsg(cMessage* msg)
{
    delete msg;
}

/**
 * Redefine this function if you want to process messages from upper
 * layers before they are send to lower layers.
 *
 * For the BaseNetwLayer we just use the destAddr of the network
 * message as a nextHop
 *
 * To forward the message to lower layers after processing it please
 * use @ref sendDown. It will take care of anything needed
 **/
void NetwJammer::handleUpperMsg(cMessage* msg)
{
    assert(dynamic_cast<cPacket*>(msg));
    sendDown(encapsMsg(static_cast<cPacket*>(msg)));
}

/**
 * Redefine this function if you want to process control messages
 * from lower layers.
 *
 * This function currently handles one messagetype: TRANSMISSION_OVER.
 * If such a message is received in the network layer it is deleted.
 * This is done as this type of messages is passed on by the BaseMacLayer.
 *
 * It may be used by network protocols to determine when the lower layers
 * are finished sending a message.
 **/
void NetwJammer::handleLowerControl(cMessage* msg)
{
    switch (msg->getKind())
    {
    case BaseMacLayer::TX_OVER:
        delete msg;
        break;

    case ReactJammerToNew:
        ev<<" Mostafa     case BaseMacLayer::ReactJammer:"<<endl;
        delete msg;
        break;

    default:
        EV << "BaseNetwLayer does not handle control messages called "
        << msg->getName() << std::endl;
        ev<<" Mostafa "<<msg->getKind()<<endl;
        if(msg->getKind()==ReactJammerToNew)
            ev<<"   if(msg->getKind()==ReactJammerToNew)"<<endl;
        delete msg;
        break;
    }
}

/**
 * Attaches a "control info" structure (object) to the down message pMsg.
 */
cObject* NetwJammer::setDownControlInfo(cMessage *const pMsg, const LAddress::L2Type& pDestAddr)
{
    return NetwToMacControlInfo::setControlInfo(pMsg, pDestAddr);
}

/**
 * Attaches a "control info" structure (object) to the up message pMsg.
 */
cObject* NetwJammer::setUpControlInfo(cMessage *const pMsg, const LAddress::L3Type& pSrcAddr)
{
    return NetwControlInfo::setControlInfo(pMsg, pSrcAddr);
}

