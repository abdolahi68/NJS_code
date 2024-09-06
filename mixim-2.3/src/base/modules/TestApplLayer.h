/* -*- mode:c++ -*- ********************************************************
 * file:        TestApplLayer.h
 *
 * author:      Daniel Willkomm
 *
 * copyright:   (C) 2004 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************
 * part of:     framework implementation developed by tkn
 * description: application layer: test class for the application layer
 **************************************************************************/

#ifndef TEST_APPL_LAYER_H
#define TEST_APPL_LAYER_H

#include "MiXiMDefs.h"
#include "BaseApplLayer.h"

class ApplPkt;

/**
 * @brief Test class for the application layer
 *
 * In this implementation all nodes randomly send broadcast packets to
 * all connected neighbors. Every node who receives this packet will
 * send a reply to the originator node.
 *
 * @ingroup applLayer
 * @author Daniel Willkomm
 **/
class MIXIM_API TestApplLayer : public BaseApplLayer
{
public:
    TestApplLayer();
    virtual ~TestApplLayer();

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int);

    /** @brief Message kinds used by this layer.*/
    enum TestApplMessageKinds {
        SEND_BROADCAST_TIMER = LAST_BASE_APPL_MESSAGE_KIND,
        BROADCAST_MESSAGE,
        Jammer_finding_process,
        BROADCAST_REPLY_MESSAGE,
        LAST_TEST_APPL_MESSAGE_KIND
    };

    static int TimeLines[100][10000];
    static double CarrierTime[100][10000];
    int number_of_slots, number_of_jammed_slot;

    int count, Total_Node, Position[100][100], distance[100][100], NJS[100], Y_i[100], X_i[100];
    double PDR[100][100], PSR[100][100], BPR[100][100], Busy_Time[100][100], ECU[100][100], ECU_CU[100][100], Timeseries[100][10000], TimeSeries_PDR[100][10000];
     bool Jammed_Nodes[100];
     int No_Actual_Jammed_nodes, No_false_Positive_Jammed_nodes;
     int PDR_Timeseries_Actual_Jammed_nodes, PDR_Timeseries_No_false_Positive_Jammed_nodes;
     int Busytime_Timeseries_Actual_Jammed_nodes, Busytime_Timeseries_No_false_Positive_Jammed_nodes;
     int Traditional_Busytime_Actual_Jammed_nodes, Traditional_Busytime_No_false_Positive_Jammed_nodes;
     int Traditional_PDR_Actual_Jammed_nodes, Traditional_PDR_No_false_Positive_Jammed_nodes;
     int NJS_Actual_ammed_nodes, NJS_No_false_Positive_Jammed_nodes;


    void Start_finding_Jammer();
    void busytime_Timeseries();
    void PDR_Timeseries();
    void TraditionalBusyTime_and_PDR();


    bool Check_Jammed_node_or_not(int, int);
    bool Is_this_a_Slot_jammed(int,int);

    double Gen_Rand(double  ,double );

    struct jammerlist{
        int node_number;
        bool jammed_node;
        jammerlist *next ;

    }*jammerslot[100], *newitem , *first_item_of_list[100] ;

    double Calculate_distance(int , int , int , int);
    void Read_Nodes_Positions();
    double Calculate_TimeSeries_PDR(int /*node id*/ , int /* start timeslot*/ );
    void show();
private:
    /** @brief Copy constructor is not allowed.
     */
    TestApplLayer(const TestApplLayer&);
    /** @brief Assignment operator is not allowed.
     */
    TestApplLayer& operator=(const TestApplLayer&);

protected:
    /** @brief Timer message for scheduling next message.*/
    cMessage *delayTimer;

    cMessage *StartFindingJammerProcess;

    /** @brief Enables debugging of this module.*/
    bool coreDebug;

protected:
    /** @brief Handle self messages such as timer... */
    virtual void handleSelfMsg(cMessage*);

    /** @brief Handle messages from lower layer */
    virtual void handleLowerMsg(cMessage*);

    virtual void handleLowerControl(cMessage *); //Mostafa added


    /** @brief send a broadcast packet to all connected neighbors */
    void sendBroadcast();

    /** @brief send a reply to a broadcast message */
 };

#endif
