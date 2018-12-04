/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//
// Simple example of OLSR routing over some point-to-point links
//
// Network topology
//
//   n0
//     \ 5 Mb/s, 2ms
//      \          1.5Mb/s, 10ms
//       n2 -------------------------n3---------n4
//      /
//     / 5 Mb/s, 2ms
//   n1
//
// - all links are point-to-point links with indicated one-way BW/delay
// - CBR/UDP flows from n0 to n4, and from n3 to n1
// - UDP packet size of 210 bytes, with per-packet interval 0.00375 sec.
//   (i.e., DataRate of 448,000 bps)
// - DropTail queues
// - Tracing of queues and packet receptions to file "simple-point-to-point-olsr.tr"

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include "ns3/topology-read-module.h"
#include <list>
#include <ctime>
#include <sstream>
#include <time.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"

using namespace ns3;

NetDeviceContainer ndc[100];
NodeContainer nc[100];
static void CalculateDelay()
{
  int max = -1;
  int tmp;
  //int maxaddr=-1;
  for (int i = 0; i < 100; i++)
  {
    tmp = 0;
    int size = ndc[i].GetN();
    //std::cout<<"test1"<<i<<size<<std::endl;
    for (int j = 0; j < size; j++)
    {

      tmp = tmp + ndc[i].Get(j)->GetObject<PointToPointNetDevice>()->GetQueue()->GetNBytes();
      //std::cout<<"test2"<<j<<std::endl;
    }
    if (tmp > max)
    {
      max = tmp;
      //maxaddr=i;
    }
  }
  //std::cout << Simulator::Now ().GetSeconds ()<<" "<<max<<" "<<maxaddr<<std::endl;
  std::cout << max << std::endl;
}
NS_LOG_COMPONENT_DEFINE("SJTU_CNPROJECT");
std::vector<int> random_pair()
{

  std::vector<int> l;
  int end = 49;
  int begin = 0;
  int sum = (rand() % (25) + 1) * 2;
  while (l.size() < (unsigned)(sum))
  {
    l.push_back(rand() % (end - begin + 1) + begin);
    //l.unique();//
  }
  return l;
}
int main(int argc, char *argv[])
{
  srand((unsigned)time(NULL));
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
  // #if 0
  LogComponentEnable("SJTU_CNPROJECT", LOG_LEVEL_ALL);
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // #endif

  // Set up some default values for the simulation.  Use the

  // Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (210));
  // Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("448kb/s"));

  //DefaultValue::Bind ("DropTailQueue::m_maxPackets", 30);

  // Allow the user to override any of the defaults and the above
  // DefaultValue::Bind ()s at run-time, via command-line arguments

  std::string format("Inet");
  std::string input("topo.txt");

  // Set up command line parameters used to control the experiment.
  CommandLine cmd;
  cmd.AddValue("format", "Format to use for data input [Orbis|Inet|Rocketfuel].",
               format);
  cmd.AddValue("input", "Name of the input file.",
               input);
  cmd.Parse(argc, argv);

  // ------------------------------------------------------------
  // -- Read topology data.
  // --------------------------------------------

  // Pick a topology reader based in the requested format.
  TopologyReaderHelper topoHelp;
  topoHelp.SetFileName(input);
  topoHelp.SetFileType(format);
  Ptr<TopologyReader> inFile = topoHelp.GetTopologyReader();

  NodeContainer nodes;
  if (inFile != 0)
  {
    nodes = inFile->Read();
  }

  if (inFile->LinksSize() == 0)
  {
    NS_LOG_ERROR("Problems reading the topology file. Failing.");
    return -1;
  }
  NS_LOG_INFO("read the file");
  NodeContainer routers;
  NodeContainer terminals;
  std::cout << nodes.GetN() << ' ' << std::endl;
  for (int i = 0; i < 50; i++)
  {
    std::cout << i << std::endl;
    terminals.Add(nodes.Get(i));
  }
  for (int i = 50; i < 80; i++)
  {
    std::cout << i << std::endl;
    routers.Add(nodes.Get(i));
  }

  // ------------------------------------------------------------
  // -- Create nodes and network stacks
  // --------------------------------------------
  NS_LOG_INFO("creating internet stack");
  InternetStackHelper stack_router;

  // // Setup NixVector Routing
  // Ipv4NixVectorHelper nixRouting;
  // stack.SetRoutingHelper (nixRouting);  // has effect on the next Install ()
  // stack.Install (nodes);

  RipHelper ripRouting;

  // Rule of thumb:
  // Interfaces are added sequentially, starting from 0
  // However, interface 0 is always the loopback...
  std::vector<int> nodes_Interface(100, 0);
  int i = 0;
  for (TopologyReader::ConstLinksIterator iter = inFile->LinksBegin(); i < 50; iter++, i++)
  {
    ripRouting.ExcludeInterface(iter->GetToNode(), ++nodes_Interface[iter->GetToNode()->GetId()]);
  }

  Ipv4ListRoutingHelper listRH;
  listRH.Add(ripRouting, 0);
  //  Ipv4StaticRoutingHelper staticRh;
  //  listRH.Add (staticRh, 5);

  stack_router.SetIpv6StackInstall(false);
  stack_router.SetRoutingHelper(listRH);
  stack_router.Install(routers);

  InternetStackHelper stack_term;
  stack_term.SetIpv6StackInstall(false);
  stack_term.Install(terminals);

  NS_LOG_INFO("creating ip4 addresses");
  Ipv4AddressHelper address;
  address.SetBase("10.0.0.0", "255.255.255.0");

  int totlinks = inFile->LinksSize();

  NS_LOG_INFO("creating node containers");
  //   NodeContainer* nc = new NodeContainer[totlinks];
  TopologyReader::ConstLinksIterator iter;
  i = 0;
  for (TopologyReader::ConstLinksIterator iter = inFile->LinksBegin(); iter != inFile->LinksEnd(); iter++, i++)
  {
    nc[i] = NodeContainer(iter->GetFromNode(), iter->GetToNode());
  }

  NS_LOG_INFO("creating net device containers");
  //   NetDeviceContainer* ndc = new NetDeviceContainer[totlinks];
  PointToPointHelper p2p;
  // error model
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
  em->SetAttribute("ErrorRate", DoubleValue(0.0001));
  for (int i = 0; i < totlinks; i++)
  {
    // p2p.SetChannelAttribute ("Delay", TimeValue(MilliSeconds(weight[i])));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    ndc[i] = p2p.Install(nc[i]);
    ndc[i].Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    ndc[i].Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(em));
  }

  // it crates little subnets, one for each couple of nodes.
  NS_LOG_INFO("creating ipv4 interfaces");
  Ipv4InterfaceContainer *ipic = new Ipv4InterfaceContainer[totlinks];
  for (int i = 0; i < totlinks; i++)
  {
    ipic[i] = address.Assign(ndc[i]);
    address.NewNetwork();
  }

  Ptr<Ipv4StaticRouting> staticRouting;

  for (int i = 0; i < 50; i++)
  {
    staticRouting = Ipv4RoutingHelper::GetRouting<Ipv4StaticRouting>(terminals.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol());
    staticRouting->SetDefaultRoute(ipic[i].GetAddress(1), 1);
  }

  /////////////////////////////////////////////////////////////////////////////
  NS_LOG_INFO("creating sending");
  ApplicationContainer clientContainer;
  UdpEchoClientHelper client(ipic[0].GetAddress(0), 1);
  client.SetAttribute("MaxPackets", UintegerValue(10));
  client.SetAttribute("Interval", TimeValue(Seconds(0.00375)));
  client.SetAttribute("PacketSize", UintegerValue(210));

  double current_time;
  std::vector<int> host_pair;
  int pair_num;
  int current_pair;
  double total_time = 120.0;
  for (current_time = 0.0; current_time < total_time; current_time += 0.1)
  {
    host_pair = random_pair();
    //NS_LOG_INFO ();
    std::cout << host_pair.size() << std::endl;
    pair_num = host_pair.size() / 2;
    for (current_pair = 0; current_pair < pair_num; ++current_pair)
    {
      client.SetAttribute("RemoteAddress", AddressValue(ipic[host_pair[2 * current_pair + 1]].GetAddress(0)));
      clientContainer = client.Install(terminals.Get(host_pair[2 * current_pair]));
      clientContainer.Start(Seconds(current_time));
      clientContainer.Stop(Seconds(current_time + 0.1));
    }
  }

  int port = 1;
  PacketSinkHelper sink("ns3::UdpSocketFactory",
                        InetSocketAddress(Ipv4Address::GetAny(), port));
  ApplicationContainer sinkApps = sink.Install(terminals);
  sinkApps.Start(Seconds(0.0));
  sinkApps.Stop(Seconds(120.0));
  /*// Create the OnOff application to send UDP datagrams of size
  // 210 bytes at a rate of 448 Kb/s from n0 to n4
  NS_LOG_INFO ("Create Applications.");
  uint16_t port = 9;   // Discard port (RFC 863)

  OnOffHelper onoff1 ("ns3::UdpSocketFactory",
                     InetSocketAddress (i34.GetAddress(1) (1), port));
  onoff1.SetConstantRate (DataRate ("448kb/s"));

  ApplicationContainer onOffApp1 = onoff1.Install (c.Get (0));
  onOffApp1.Start (Seconds (10.0));
  onOffApp1.Stop (Seconds (20.0));

  // Create a similar flow from n3 to n1, starting at time 1.1 seconds
  OnOffHelper onoff2 ("ns3::UdpSocketFactory",
                     InetSocketAddress (i12.GetAddress (0), port));
  onoff2.SetConstantRate (DataRate ("448kb/s"));

  ApplicationContainer onOffApp2 = onoff2.Install (c.Get (3));
  onOffApp2.Start (Seconds (10.1));
  onOffApp2.Stop (Seconds (20.0));

  // Create packet sinks to receive these packets
  PacketSinkHelper sink ("ns3::UdpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  NodeContainer sinks = NodeContainer (c.Get (4), c.Get (1));
  ApplicationContainer sinkApps = sink.Install (sinks);
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (21.0));

  AsciiTraceHelper ascii;
  p2p.EnableAsciiAll (ascii.CreateFileStream ("simple-point-to-point-olsr.tr"));
  p2p.EnablePcapAll ("simple-point-to-point-olsr");*/

  Simulator::Stop(Seconds(30));
  NS_LOG_INFO("set avg queue collector");
  for (int i = 0; i <= 1200; i++)
  {

    Simulator::Schedule(MilliSeconds(100 + 100 * i), CalculateDelay);
  }
  NS_LOG_INFO("Run Simulation.");
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_INFO("Done.");

  return 0;
}
