#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <algorithm>
#include <vector>
#include <random>
#include <ctime>
#include <list>
#include <sstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/topology-read-module.h"

using namespace ns3;

std::vector<int> random_pair()
{
    srand(time(NULL));
    int num_of_pair = (rand() % 25)+ 1;
    std::vector<int> tmp;
    for(int i = 0 ; i < 50 ; ++i)
    {
        tmp.push_back(i);
    }
    random_shuffle(tmp.begin(),tmp.end());

    std::vector<int> res;
    for(int i = 0 ; i < num_of_pair*2 ; ++i)
    {
        res.push_back(tmp[i]);
    }
    return res;
}

/*
static void CalculateDelay (Ptr<const Packet>p,const Address &address)
{
    //static float k = 0;
    //k++;
    //static float m = -1;
    //static float n = 0;
    //n += (p->GetUid() - m)/2-1;
    DelayJitterEstimation delayJitter;
    delayJitter.RecordRx(p);
    Time t = delayJitter.GetLastDelay();
    std::cout << Simulator::Now ().GetSeconds () << "\t" << t.GetMilliSeconds() << std::endl;
    //m = p->GetUid();
}
*/


NS_LOG_COMPONENT_DEFINE ("Topology Simulation.");

int main (int argc, char *argv[])
{
    std::string format("Inet");
    std::string input("/home/chenshenwei/Desktop/topo_inet.txt");

    CommandLine cmd;
    cmd.Parse (argc, argv);
	LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);

	//LogComponentEnable ("Topology Simulation.", LOG_LEVEL_INFO);

    // read topology data
    NS_LOG_INFO ("Read topology data.");
    TopologyReaderHelper topoHelp;
    topoHelp.SetFileName (input);
    topoHelp.SetFileType (format);
    Ptr<TopologyReader> inFile = topoHelp.GetTopologyReader ();

    NodeContainer nodes;

    if (inFile != 0)
    {
      nodes = inFile->Read ();
    }
    if (inFile->LinksSize () == 0)
    {
      NS_LOG_ERROR ("Problems reading the topology file. Failing.");
      return -1;
    }

    // Create nodes and network stacks
    NS_LOG_INFO ("Installing internet stack.");
    InternetStackHelper internet;

    // Setup NixVector Routing
    Ipv4NixVectorHelper nixRouting;
    internet.SetRoutingHelper(nixRouting);
    internet.Install(nodes);

    int totlinks = inFile->LinksSize ();

    NS_LOG_INFO ("Creating node containers");
    NodeContainer* nc = new NodeContainer[totlinks];
    TopologyReader::ConstLinksIterator iter;
    int i = 0;
    for ( iter = inFile->LinksBegin (); iter != inFile->LinksEnd (); iter++, i++ )
    {
        nc[i] = NodeContainer (iter->GetFromNode (), iter->GetToNode ());
    }

	//NodeContainer* nc_host = new NodeContainer[50];
	//for ( int j = 0 ; j < 50 ; ++j)
	//{
		//nc_host[j] = nc[j];
	//}

    NS_LOG_INFO ("creating net device containers");
    NetDeviceContainer* ndc = new NetDeviceContainer[totlinks];
    PointToPointHelper p2p;
    for (int i = 0; i < totlinks; i++)
    {
        p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
        p2p.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
        ndc[i] = p2p.Install (nc[i]);
    }

    NS_LOG_INFO ("Assign IP Addresses.");
    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer* ipic = new Ipv4InterfaceContainer[totlinks];
    for (int i = 0; i < totlinks; i++)
    {
        ipic[i] = address.Assign (ndc[i]);
        address.NewNetwork ();
    }

	NodeContainer hostNodes;
	for ( unsigned int i = 0; i < 50; i++ )
    {
		Ptr<Node> hostNode = nodes.Get(i);
		hostNodes.Add (hostNode);
    }
	//install applications onto the devices
    NS_LOG_INFO ("Create applications.");
    UdpServerHelper server(9);
    ApplicationContainer serverContainer = server.Install(hostNodes);
    serverContainer.Start(Seconds(0.0));
    serverContainer.Stop(Seconds(30.0));

    uint32_t MaxPacketSize = 210;
    Time interPacketInterval = Seconds (0.00375);
    //uint32_t maxPacketCount = 26;
    UdpClientHelper client (ipic[0].GetAddress(0), 9);
    //client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (MaxPacketSize));
    ApplicationContainer clientContainer;



    //std::cout<<1;

    double current_time;
    std::vector<int> host_pair;
    int pair_num;
    int current_pair;
    for(current_time = 0.0 ; current_time < 30 ; current_time+=0.1)
    {
        host_pair = random_pair();
        pair_num = host_pair.size()/2;
        for(current_pair = 0 ; current_pair < pair_num ; ++current_pair)
        {
			client.SetAttribute("RemoteAddress", AddressValue(ipic[host_pair[2*current_pair+1]].GetAddress(0)));
			clientContainer = client.Install(hostNodes.Get(host_pair[2*current_pair]));
			clientContainer.Start(Seconds(current_time));
			clientContainer.Stop(Seconds(current_time+0.1));
        }
    }

    //for(int i = 0 ; i < 50 ; ++i){
        //serverContainer.Get(i)->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&CalculateDelay));
    //}

	//AsciiTraceHelper ascii;
   	//p2p.EnableAsciiAll (ascii.CreateFileStream ("topo-simulation.tr"));
   	//p2p.EnablePcapAll ("topo-simulation");

	NS_LOG_INFO ("Run Simulation.");
	Simulator::Run ();
	Simulator::Destroy ();

	delete[] ipic;
	delete[] ndc;
	delete[] nc;

	NS_LOG_INFO ("Done.");

	return 0;
}
