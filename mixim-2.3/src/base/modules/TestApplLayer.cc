

#include "TestApplLayer.h"

#include "NetwControlInfo.h"
#include "SimpleAddress.h"
#include "ApplPkt_m.h"
#include "NetwPkt_m.h" //Mostafa added
#include "istream"
#include "iostream"
#include "ostream"
#include "fstream"
#include "stdio.h"
#include "cstdlib"
#include "stdlib.h"
#include "dos.h"
#include "math.h"

using namespace std ;
using std::endl;

using std::endl;

TestApplLayer::TestApplLayer()
: BaseApplLayer()
, delayTimer(NULL)
, coreDebug(false)
{}


int TestApplLayer::TimeLines[100][10000];
double TestApplLayer::CarrierTime[100][10000];
void TestApplLayer::initialize(int stage)
{

    number_of_jammed_slot=0;

    No_Actual_Jammed_nodes=0;
    Busytime_Timeseries_Actual_Jammed_nodes=Busytime_Timeseries_No_false_Positive_Jammed_nodes=0;
    PDR_Timeseries_Actual_Jammed_nodes=PDR_Timeseries_No_false_Positive_Jammed_nodes=0;
    Traditional_Busytime_Actual_Jammed_nodes=Traditional_Busytime_No_false_Positive_Jammed_nodes=0;
    Traditional_PDR_Actual_Jammed_nodes=Traditional_PDR_No_false_Positive_Jammed_nodes=0;
    NJS_Actual_ammed_nodes=NJS_No_false_Positive_Jammed_nodes=0;

    count=0;
    Total_Node=25;
    BaseApplLayer::initialize(stage);
    if(stage == 0) {
        hasPar("coreDebug") ? coreDebug = par("coreDebug").boolValue() : coreDebug = false;
        delayTimer = new cMessage( "delay-timer", SEND_BROADCAST_TIMER );
        StartFindingJammerProcess=new cMessage ("Start Process", Jammer_finding_process);


    }
    else if(stage==1) {
        if(myApplAddr()==0)
        {
            // number_of_slots=10;
            Read_Nodes_Positions();
            //  Start_finding_Jammer();

            ev<<" Mostafa number of jammed slot = "<<number_of_jammed_slot<<endl;


        }
        // if(myApplAddr()==1)
        scheduleAt(simTime()+ Gen_Rand(0,1.0), delayTimer); //uniform(0,0.05)
    }
}
double TestApplLayer::Gen_Rand(double a ,double b)
{
    double random = ((double) rand()) / (double) RAND_MAX;
    double diff = b - a;
    double r = random * diff;
    return a + r;
}

/**
 * There are two kinds of messages that can arrive at this module: The
 * first (kind = BROADCAST_MESSAGE) is a broadcast packet from a
 * neighbor node to which we have to send a reply. The second (kind =
 * BROADCAST_REPLY_MESSAGE) is a reply to a broadcast packet that we
 * have send and just causes some output before it is deleted
 **/
void TestApplLayer::handleLowerMsg( cMessage* msg )
{
    ApplPkt *m;
    switch( msg->getKind() ) {
    case BROADCAST_MESSAGE:
        m = static_cast<ApplPkt *>(msg);
        coreEV << "Received a broadcast packet from host["<<m->getSrcAddr()<<"] -> sending reply" << endl;
        delete msg;
        break;
    case BROADCAST_REPLY_MESSAGE:
        m = static_cast<ApplPkt *>(msg);
        coreEV << "Received reply from host["<<m->getSrcAddr()<<"]; delete msg" << endl;
        delete msg;
        break;
    default:
        EV <<"Error! got packet with unknown kind: " << msg->getKind()<<endl;
        delete msg;
        break;
    }
}

/**
 * A timer with kind = SEND_BROADCAST_TIMER indicates that a new
 * broadcast has to be send (@ref sendBroadcast).
 *
 * There are no other timer implemented for this module.
 *
 * @sa sendBroadcast
 **/

void TestApplLayer::show()
{
    ev<<"mostafa Final node = "<<myApplAddr()<<endl;

    for(int i=0 ;i<Total_Node;i++)
    {
        for(int j=0;j<number_of_slots;j++)
            ev<<TimeLines[i][j]<<"    ";
        ev<<endl;
    }//end for

    for(int i=0 ;i<Total_Node;i++)
    {
        for(int j=0;j<number_of_slots;j++)
            ev<<CarrierTime[i][j]<<"    ";
        ev<<endl;
    }//end for


    Start_finding_Jammer();
    endSimulation();

}
void TestApplLayer::handleSelfMsg(cMessage *msg) {
    switch( msg->getKind() ) {

    case SEND_BROADCAST_TIMER:
        sendBroadcast();
        delete msg;
        delayTimer = NULL;
        break;

    case Jammer_finding_process:

        show();
        break;
    default:
        EV << "Unknown selfmessage! -> delete, kind: "<<msg->getKind() <<endl;
        delete msg;
        break;
    }
}

/**
 * This function creates a new broadcast message and sends it down to
 * the network layer
 **/
void TestApplLayer::sendBroadcast()
{
    ApplPkt *pkt = new ApplPkt("BROADCAST_MESSAGE", BROADCAST_MESSAGE);
    pkt->setDestAddr(LAddress::L3BROADCAST);
    // we use the host modules getIndex() as a appl address
    pkt->setSrcAddr( myApplAddr() );
    pkt->setBitLength(11504);

    // set the control info to tell the network layer the layer 3
    // address;
    NetwControlInfo::setControlInfo(pkt, LAddress::L3BROADCAST );

    coreEV << "Sending broadcast packet!" << endl;
    sendDown( pkt );
    count++;

    if(count<20000)
        scheduleAt(simTime()+  Gen_Rand(0,1.0) , delayTimer);//uniform(0,0.05)

}




void TestApplLayer::handleLowerControl(cMessage *msg) //Mostafa added
{
    ev<<"TestApplLayer::handleLowerControl "<<endl;


    NetwPkt *m;
    m = static_cast<NetwPkt *>(msg);

    number_of_slots=m->getCounter();

    for(int i=0;i<m->getCounter();i++)
    {
        TimeLines[myApplAddr()][i]=m->getTimeSlot(i);
        CarrierTime[myApplAddr()][i]=m->getCarrierTime(i);
        ev<<m->getTimeSlot(i)<<" ";
    }
    ev<<endl;

    if(myApplAddr()==0)
        scheduleAt(simTime()+0.1, StartFindingJammerProcess);

    delete msg;

}



void TestApplLayer::Start_finding_Jammer()
{

    //********************* finding Actual jammed nodes **** START ***********

    for (int j=0;j<Total_Node;j++)
    {
        if(Calculate_distance(Position[j][0],Position[j][1], 200,200)<225)// position of nodes
        {
            Jammed_Nodes[j]=true;
            No_Actual_Jammed_nodes++;
            cout<<" Node "<<j<<" is a actual jammed node"<<endl;
        }
        else
        {
            Jammed_Nodes[j]=false;
            cout<<" Node "<<j<<" is NOT a actual jammed node"<<endl;

        }


    }//end for

    //********************* finding Actual jammed nodes **** END ***********


    for(int i=0;i<number_of_slots;i++)
    {
        for(int j=0;j<Total_Node;j++)
        {
            if(Check_Jammed_node_or_not(i,j)==true)
            {
                X_i[j]+=1;
                ev<<" i= "<<i<<" j= "<<j<<endl;

            }//end if
        }
    }//end for


    for(int j=0;j<Total_Node;j++)
    {
        ev<<j<<" Total X_i= "<<X_i[j]<<endl;
    }

    for(int i=0;i<number_of_slots;i++)
    {
        ev<<" Slot number = "<<i<<endl;
        for(int j=0;j<Total_Node;j++)
        {
            if(Check_Jammed_node_or_not(i,j)==true)
                ev<<"main jammed node = "<<j<<endl;

            if(Check_Jammed_node_or_not(i,j)==false && X_i[j]>0 && Is_this_a_Slot_jammed(i,j)==true && TimeLines[j][i]!=0)
            {
                ev<<" Y slot = "<<i<<" node= "<<j<<endl;
                Y_i[j]+=1;

            }//end if
        }
    }//end for

    ev<<"******** Final Result ********"<<endl;
    int Num_of_Jammed_nodes=0, Sum_NJS=0;


    fstream file2;
       file2.open("C:\\Users\\mostafa\\Desktop\\Result\\Proactive.txt");
       file2.seekp(0,ios::end);
       file2<<endl;


    fstream file;
    file.open("C:\\Users\\mostafa\\Desktop\\Result\\Reactive.txt");
    file.seekp(0,ios::end);
    file<<endl;


    NJS_Actual_ammed_nodes=NJS_No_false_Positive_Jammed_nodes=0;

    for(int j=0;j<Total_Node;j++)
    {
        if((X_i[j]+Y_i[j])>0)
        {
            Sum_NJS+=(X_i[j]+Y_i[j]);
            Num_of_Jammed_nodes++;
           // file<<j<<" "<<X_i[j]+Y_i[j]<<endl;
            if(Jammed_Nodes[j]==true)
                NJS_Actual_ammed_nodes++;
            else
                NJS_No_false_Positive_Jammed_nodes++;
        }
        ev<<j<<" X= "<<X_i[j]<<" Y= "<<Y_i[j]<<endl;
        cout<<j<<" X= "<<X_i[j]<<" Y= "<<Y_i[j]<<endl;
    }



    PDR_Timeseries();
    busytime_Timeseries();
    TraditionalBusyTime_and_PDR();
    cout<<" ##################################################"<<endl;

    cout<<"No_Actual_Jammed_nodes= "<<No_Actual_Jammed_nodes<<endl;
    cout<<"PDR_Timeseries_Actual_Jammed_nodes= "<<PDR_Timeseries_Actual_Jammed_nodes<<endl;
    cout<<"PDR_Timeseries_No_false_Positive_Jammed_nodes= "<<PDR_Timeseries_No_false_Positive_Jammed_nodes<<endl;
    cout<<"Busytime_Timeseries_Actual_Jammed_nodes= "<<Busytime_Timeseries_Actual_Jammed_nodes <<endl;
    cout<<"Busytime_Timeseries_No_false_Positive_Jammed_nodes= "<< Busytime_Timeseries_No_false_Positive_Jammed_nodes<<endl;
    cout<<" Traditional_Busytime_Actual_Jammed_nodes= "<<Traditional_Busytime_Actual_Jammed_nodes<<endl;
    cout<<"Traditional_Busytime_No_false_Positive_Jammed_nodes= "<<Traditional_Busytime_No_false_Positive_Jammed_nodes<<endl;
    cout<<"Traditional_PDR_Actual_Jammed_nodes= "<< Traditional_PDR_Actual_Jammed_nodes<<endl;
    cout<<"Traditional_PDR_No_false_Positive_Jammed_nodes=  "<<Traditional_PDR_No_false_Positive_Jammed_nodes<<endl;
    cout<<"NJS_Actual_ammed_nodes= "<<NJS_Actual_ammed_nodes<<endl;
    cout<<"NJS_No_false_Positive_Jammed_nodes= "<<NJS_No_false_Positive_Jammed_nodes<<endl;

file<<No_Actual_Jammed_nodes<<" "<<NJS_Actual_ammed_nodes<<" "<<PDR_Timeseries_Actual_Jammed_nodes<<" "<<Busytime_Timeseries_Actual_Jammed_nodes<<" "<<Traditional_Busytime_Actual_Jammed_nodes<<" "<<Traditional_PDR_Actual_Jammed_nodes;
file2<<NJS_No_false_Positive_Jammed_nodes<<" "<<PDR_Timeseries_No_false_Positive_Jammed_nodes<<" "<<Busytime_Timeseries_No_false_Positive_Jammed_nodes<<" "<<Traditional_Busytime_No_false_Positive_Jammed_nodes<<" "<<Traditional_PDR_No_false_Positive_Jammed_nodes;

    cout<<" ##################################################"<<endl;












}

void TestApplLayer::TraditionalBusyTime_and_PDR()
{

    //        fstream file1;
    //         file1.open("C:\\Users\\mostafa\\Desktop\\Result\\Proactive.txt");
    //         file1.seekp(0,ios::end);
    //         file1<<endl;


    cout<<" ********************* TraditionalBusyTime_and_PDR() ***************************"<<endl;

    for(int j=0;j<Total_Node;j++)
    {
        int temp_BPR=0, temp_correctPKT=0 , temp_send=0, Mycounter=0;

        for(int i=0;i<number_of_slots;i++)
        {
            if(TimeLines[j][i]==2)
                temp_send++;
            else if(TimeLines[j][i]==1)
                temp_correctPKT++;
            else if(TimeLines[j][i]==3)
                temp_BPR++;

            if(i%142==0 && i>0)
            {
                Busy_Time[j][Mycounter]=(double)(temp_correctPKT+temp_BPR)/142;
                PDR[j][Mycounter]=(double)temp_correctPKT/(temp_correctPKT+temp_BPR);

                cout<<" temp_correctPKT= "<<temp_correctPKT<<" temp_BPR="<<temp_BPR<<endl;
                cout<<"Busy_Time["<<j<<"]["<<Mycounter<<"]="<<Busy_Time[j][Mycounter]<<endl;
                cout<<"PDR["<<j<<"]["<<Mycounter<<"]="<<PDR[j][Mycounter]<<endl;



                if(Mycounter>0 && (Busy_Time[j][Mycounter] > (Busy_Time[j][Mycounter-1] *1.1))) //10% difference
                {
                    if(Jammed_Nodes[j]==true)
                    {
                        Traditional_Busytime_Actual_Jammed_nodes++;
                        cout<<" BusyTime node "<<j<<" is a actual jammed node "<<endl;
                        cout<<" Busy_Time[j][Mycounter] = "<<Busy_Time[j][Mycounter]<<" Busy_Time[j][Mycounter-1]= "<<Busy_Time[j][Mycounter-1]<<endl;
                    }
                    else
                    {
                        Traditional_Busytime_No_false_Positive_Jammed_nodes++;
                        cout<<" BusyTime node "<<j<<" is NOT a actual jammed node "<<endl;

                    }
                    i=number_of_slots;
                }

                if(Mycounter>0 && (PDR[j][Mycounter] < (PDR[j][Mycounter-1] *0.9))) //10% difference
                {
                    if(Jammed_Nodes[j]==true)
                    {
                        Traditional_PDR_Actual_Jammed_nodes++;
                        cout<<" PDR node "<<j<<" is a actual jammed node "<<endl;

                    }
                    else
                    {
                        Traditional_PDR_No_false_Positive_Jammed_nodes++;
                        cout<<" PDR node "<<j<<" is NOT a actual jammed node "<<endl;

                    }
                    i=number_of_slots;
                }

                Mycounter++;
                temp_send=temp_correctPKT=temp_BPR=0;
            }// end if
        }//end for

    }//end for


}

void TestApplLayer::busytime_Timeseries()
{
    //*****************************  Time series method (Busy Time)***************START **********
    for(int j=0;j<Total_Node;j++)
    {
        for(int i=0;i<number_of_slots;i++)
        {
            if(TimeLines[j][i]==1 || TimeLines[j][i]==3)
                Timeseries[j][i]=1;
            else
                Timeseries[j][i]=0;

        }//end for
    }//end for


    int startpoint=0, m=10;

    for(int j=0;j<Total_Node;j++)
    {
        double D_i[100];
        for(int k=0;k<100;k++)
            D_i[k]=0;
        startpoint=0;
        cout<<" *************************** "<<j<<" *************************************"<<endl;
        while((2*m+startpoint)<number_of_slots)
        {
            double Sum_X1m=0 , Sum_Xm2m=0;
            double Var =0, Mean=0, Thr_i=0, z=0.2; //error E= 0.93 --> Z=1.49


            for(int i=0+startpoint;i<2*m+startpoint; i++)
            {
                cout<<Timeseries[j][i]<<"  ";
            }


            for(int i=0+startpoint;i<m+startpoint;i++)
            {
                Sum_X1m+=Timeseries[j][i];
            }

            for(int i=0+m+startpoint;i<2*m+startpoint;i++)
            {
                Sum_Xm2m+=Timeseries[j][i];
            }

            D_i[startpoint]= pow((Sum_X1m-Sum_Xm2m),2);

            if(startpoint>0)
            {
                for(int i=0; i<=startpoint ; i++)
                {
                    Mean+=D_i[i];
                }

                Mean=Mean/(startpoint+1); //calculate average

                for(int i=0; i<=startpoint ; i++)
                {
                    Var+=pow(D_i[i]-Mean,2);
                }

                Var=Var/(startpoint+1); //calculate Variance
                Thr_i= z * sqrt(Var)*sqrt(2*m); //calculate threshold

                if(startpoint>5)
                {
                    if(D_i[startpoint]>=Thr_i)
                    {
                        startpoint=20000; // to break the inside for

                        if(Jammed_Nodes[j]==true)
                        {
                            Busytime_Timeseries_Actual_Jammed_nodes++;
                        }
                        else
                            Busytime_Timeseries_No_false_Positive_Jammed_nodes++;
                        cout<<"\n node "<<j<<" is jammed node"<<endl;
                    }
                }

                cout<<"\n Sum_X1m= "<< Sum_X1m<<" Sum_Xm2m= "<< Sum_Xm2m <<"  D_i="<<D_i[startpoint]<< " Mean="<<Mean<<"  Var="<<Var<<" Thr_i="<<Thr_i<<endl;
                cout<<"\n\n\n\n";
            }//end if
            startpoint++;
        }//end while
    }//end for

    //******************************** Time series Method (Busy Time)************** END ******************
}




void TestApplLayer::PDR_Timeseries()
{
    //********************************* Time Series Method (PDR) ****************** Start ******************

    int mystartpoint=0, numberofPDRs=0;

    for(int j=0;j<Total_Node;j++)
    {
        mystartpoint=0;
        while(mystartpoint+20<number_of_slots)
        {
            TimeSeries_PDR[j][mystartpoint]=Calculate_TimeSeries_PDR(j,mystartpoint);
            mystartpoint++;

            if(mystartpoint>numberofPDRs)
                numberofPDRs=mystartpoint;
        }
    }//end for

    for(int j=0;j<Total_Node;j++)
    {
        cout<<"  ************ "<<j<<" ************ "<<endl;
        for( int i=0;i<numberofPDRs;i++)
        {
            cout<<TimeSeries_PDR[j][i]<<" ";
        }
        cout<<endl;
    }//end for

    int PDRstartpoint=0;
    int  m=12;

    for(int j=0;j<Total_Node;j++)
    {
        double D_i[100];
        for(int k=0;k<100;k++)
            D_i[k]=0;
        PDRstartpoint=0;
        cout<<" *************************** "<<j<<" *************************************"<<endl;
        while((2*m+PDRstartpoint)<numberofPDRs)
        {
            double Sum_X1m=0 , Sum_Xm2m=0;
            double Var =0, Mean=0, Thr_i=0, z=0.2; //error E= 0.93 --> Z=1.49


            for(int i=0+PDRstartpoint;i<2*m+PDRstartpoint; i++)
            {
                cout<<TimeSeries_PDR[j][i]<<"  ";
            }


            for(int i=0+PDRstartpoint;i<m+PDRstartpoint;i++)
            {
                Sum_X1m+=TimeSeries_PDR[j][i];
            }

            for(int i=0+m+PDRstartpoint;i<2*m+PDRstartpoint;i++)
            {
                Sum_Xm2m+=TimeSeries_PDR[j][i];
            }

            D_i[PDRstartpoint]= pow((Sum_X1m-Sum_Xm2m),2);

            if(PDRstartpoint>0)
            {
                for(int i=0; i<=PDRstartpoint ; i++)
                {
                    Mean+=D_i[i];
                }

                Mean=Mean/(PDRstartpoint+1); //calculate average

                for(int i=0; i<=PDRstartpoint ; i++)
                {
                    Var+=pow(D_i[i]-Mean,2);
                }

                Var=Var/(PDRstartpoint+1); //calculate Variance
                Thr_i= z * sqrt(Var)*sqrt(2*m); //calculate threshold

                if(PDRstartpoint>5)
                {
                    if(D_i[PDRstartpoint]>=Thr_i)
                    {
                        PDRstartpoint=20000; // to break the inside for
                        cout<<"\n node "<<j<<" is jammed node"<<endl;
                        if(Jammed_Nodes[j]==true)
                        {
                            PDR_Timeseries_Actual_Jammed_nodes++;
                        }
                        else
                            PDR_Timeseries_No_false_Positive_Jammed_nodes++;
                    }
                }
                cout<<"\n Sum_X1m= "<< Sum_X1m<<" Sum_Xm2m= "<< Sum_Xm2m <<"  D_i="<<D_i[PDRstartpoint]<< " Mean="<<Mean<<"  Var="<<Var<<" Thr_i="<<Thr_i<<endl;
                cout<<"\n\n\n\n";
            }//end if
            PDRstartpoint++;
        }//end while
    }//end for


    //********************************* Time Series Method (PDR) ****************** END ******************

}


double TestApplLayer::Calculate_TimeSeries_PDR(int node_ID, int startpoint)
{

    //    for( int i =0;i<number_of_slots;i++)
    //    {
    //        cout<<TimeLines[0][i]<<" ";
    //    }
    //    cout<<endl;

    int NoRXpkts=0 , NoCorrectpkts=0;
    double localPDR=0;
    for(int i=startpoint;i<startpoint+20 ;i++)
    {
        if(TimeLines[node_ID][i]==1)
        {
            NoCorrectpkts++;
            NoRXpkts++;
        }
        else if(TimeLines[node_ID][i]==3)
            NoRXpkts++;


    }//end if


    if(NoRXpkts>0)
    {
        localPDR=(double)NoCorrectpkts/NoRXpkts;
    }
    else
        localPDR=0;


    //cout << "node "<<node_ID<<" NoCorrectpkts= "<<NoCorrectpkts<<" NoRXpkts= "<<NoRXpkts<<"  localPDR= "<<localPDR<<endl;

    return localPDR;
}

bool TestApplLayer::Is_this_a_Slot_jammed(int Slot_Num, int Node_num)
{

    bool result=false;


    if(TimeLines[Node_num][Slot_Num]!=0) //send or received
    {
        for(int i=0;i<Total_Node;i++)
        {

            if(i!=Node_num && distance[i][Node_num]<400 && Check_Jammed_node_or_not(Slot_Num,i)==true)
            {
                result=true;
                ev<<" sloted node is = "<<i<<endl;
                break;
            }
        }
    }

    return result;
}

bool TestApplLayer::Check_Jammed_node_or_not(int Slot_Num, int Node_num)
{
    bool result;

    if(TimeLines[Node_num][Slot_Num]==2 || TimeLines[Node_num][Slot_Num]==0)
    {
        result=false;
    }
    else
    {
        bool find_sender=false;
        for(int i=0;i<Total_Node;i++)
        {
            if(i!=Node_num && distance[i][Node_num]<230 && TimeLines[i][Slot_Num]==2)
                find_sender=true;
        }
        if(find_sender==true)
            result=false;
        else
            result=true;
    }

    if(result==true)
        ev<<" slot# = "<<Slot_Num<<" node# = "<<Node_num<< " result= "<<result<<endl;

    return result;
}


double TestApplLayer::Calculate_distance(int x1, int y1, int x2, int y2)
{
    double temp=pow(double(x1-x2),2)+pow(double(y1-y2),2);

    double result=sqrt(temp);

    return result;
}

void TestApplLayer::Read_Nodes_Positions()
{
    ifstream file;
    file.open("C:\\Users\\mostafa\\Desktop\\Positions.txt");

    char ss[100] , posx[6] , posy[6];
    for(int k=0;k<Total_Node;k++)
    {
        file.getline(posx,6,' ');
        file.getline(posy,6,'\n');
        int x=atoi(posx);
        int y=atoi(posy);
        Position[k][0]=x;
        Position[k][1]=y;

        ev<<x<<"   "<<y<<endl;
    }//end for
    for(int i=Total_Node;i<100;i++)
    {
        Position[i][0]=Position[i][1]=10000;
        NJS[i]=Y_i[i]=X_i[i]=0;

    }

    for(int i=0;i<Total_Node;i++)
        for(int j=0;j<100;j++)
        {
            ECU_CU[i][j]=ECU[i][j]=Busy_Time[i][j]=PDR[i][j]=PSR[i][j]=BPR[i][j]=0;
        }



    for(int k=0;k<Total_Node;k++)
        ev<<k<< "  "<<Position[k][0]<<" "<<Position[k][1]<<endl;

    for(int i=0;i<Total_Node;i++)
    {
        for(int j=0;j<Total_Node;j++)
        {
            if(i==j)
                distance[i][j]=0;
            else{

                if(Calculate_distance(Position[i][0],Position[i][1],Position[j][0],Position[j][1]) >440)
                    distance[i][j]=1000;
                else
                    distance[i][j]=Calculate_distance(Position[i][0],Position[i][1],Position[j][0],Position[j][1]);
            }
            ev<<distance[i][j]<<" ";
        }//end for
        ev<<endl;
    }//end for



}


TestApplLayer::~TestApplLayer()
{
    cancelAndDelete(delayTimer);
}
