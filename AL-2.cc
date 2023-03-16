#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include <stdlib.h>
#include <cmath>
#include <iostream>
#include <list>

using namespace ns3;


int count = 0;
int human = 150;
double time_simu = 200.0;
bool risk = false;
std::list<int> myList;

void
CalculateDistance (Ptr<Node> infected, Ptr<Node> normal, double time, int i, int human)
{
  double posX1 = infected->GetObject<MobilityModel> ()->GetPosition ().x;
  double posY1 = infected->GetObject<MobilityModel> ()->GetPosition ().y;
  double posX2 = normal->GetObject<MobilityModel> ()->GetPosition ().x;
  double posY2 = normal->GetObject<MobilityModel> ()->GetPosition ().y;
  double distance = sqrt(abs((pow((posX1 - posX2),2.0)+pow((posY1 - posY2),2.0))));
  // std::cout << "Distance between nodes at time " << time << ": " << distance/10 << " m " << "Normal node is " << i << std::endl;
  if((distance < 20) & (i%2 == 0)){ //with mask
    int gotcha = rand() % 10+1;
    if(gotcha == 1){ // 10 per 100 to infected
        myList.push_back(i);
        count++;
    }
  }
  else if((distance < 20) & (i%2 != 0)){ // without mask
    myList.push_back(i);
    count++;
  }
  if((i == human-1) & (time == time_simu)){
    myList.sort();
    myList.unique();
    std::cout << "Node Risk is : ";
    for (int& value : myList) {
        std::cout << value << " ";
    }
    std::cout << std::endl;
    double size = myList.size();
    std::cout << "Have : " << size << " nodes" <<std::endl;

    double rate = ( size / human )  * 100;
    std::cout << "Infected rate : " << rate << " %" <<std::endl;
  }
}

int
main(int argc, char* argv[])
{
    NodeContainer node;
    node.Create(human);

    NetDeviceContainer devices;

    InternetStackHelper stack;
    stack.Install (node);

    Ipv4AddressHelper address;

    Ipv4InterfaceContainer interface;

    ApplicationContainer app;

    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560)));
    devices = csma.Install(node);

    MobilityHelper mobility;

    ObjectFactory pos;
    pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
    pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=500.0]"));
    pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));
    Ptr<PositionAllocator> posAlloc = pos.Create()->GetObject<PositionAllocator>();

    mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel", 
                            "Speed", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"),
                            "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"),
                            "PositionAllocator", PointerValue(posAlloc));


    mobility.SetPositionAllocator(posAlloc);
    mobility.Install(node);

    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

    
    address.SetBase("10.1.1.0", "255.255.255.0");
    interface = address.Assign (devices);

    //Sent
    Ptr<Socket> source = Socket::CreateSocket (node.Get (0), tid);
    InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
    source->SetAllowBroadcast (true);
    source->Connect (remote);
    OnOffHelper onoff ("ns3::UdpSocketFactory",
                       Address (InetSocketAddress (Ipv4Address ("255.255.255.255"), 80)));
    onoff.SetConstantRate (DataRate ("4.15kb/s"));
    app.Add (onoff.Install (node.Get (0)));
    app.Start (Seconds (1.0));
    app.Stop (Seconds (time_simu));

    // Recive
    Ptr<Socket> recvSink = Socket::CreateSocket (node.Get(1), tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
    recvSink->Bind (local);

    // Schedule the distance calculation every second
    
    for (int i = 1; i <= human-1; i++){

        for (double time = 0; time <= time_simu; time += 1) {
          Simulator::Schedule (Seconds (time), &CalculateDistance, node.Get (0), node.Get (i), time, i, human);
        }

    }

    AnimationInterface anim ("test.xml");
    for (int i = 1; i <= human-1; i++){
        if(i%2 == 0){
            anim.UpdateNodeColor(node.Get(i), 0,255,0);
        }
        else{
            anim.UpdateNodeColor(node.Get(i), 255,255,0);
        }
    }
    anim.UpdateNodeColor(node.Get(0), 255,0,0);
    Simulator::Stop(Seconds(time_simu));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}