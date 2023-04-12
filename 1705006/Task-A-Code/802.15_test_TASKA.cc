/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Universita' di Firenze
 *
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

// Network topology
//
//       n0    n1
//       |     |
//       =================
//        WSN (802.15.4)
//
// - ICMPv6 echo request flows from n0 to n1 and back with ICMPv6 echo reply
// - DropTail queues 
// - Tracing of queues and packet receptions to file "wsn-ping6.tr"
//
// This example is based on the "ping6.cc" example.

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/mobility-module.h"
#include <iostream>
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-stack-helper.h"
#include <ns3/ripng-helper.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Ping6WsnExample");
//-----------------------routing-----------------//
class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run (int nSinks, double txp, std::string CSVfileName);
  //static void SetMACParam (ns3::NetDeviceContainer & devices,
  //                                 int slotDistance);
  std::string CommandSetup (int argc, char **argv);

private:
  Ptr<Socket> SetupPacketReceive (Ipv6Address addr, Ptr<Node> node);
  void ReceivePacket (Ptr<Socket> socket);
  void CheckThroughput ();

  uint32_t port;
  uint32_t bytesTotal;
  uint32_t packetsReceived;

  std::string m_CSVfileName;
  int m_nSinks;
  int m_nodes;
  int m_nPacketsPerSecond;
  int m_NodeSpeed;
  std::string m_protocolName;
  double m_txp;
  bool m_traceMobility;
  // uint32_t m_protocol;
};

RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0),
    m_CSVfileName ("802.15.csv"),
    m_nSinks(20),
    m_nodes(80),
    m_nPacketsPerSecond(10),
    m_NodeSpeed(10),
    m_traceMobility (false)
{
}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (Inet6SocketAddress::IsMatchingType (senderAddress))
    {
      Inet6SocketAddress addr = Inet6SocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv6 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void
RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      bytesTotal += packet->GetSize ();
      packetsReceived += 1;
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}

void
RoutingExperiment::CheckThroughput ()
{
  double kbs = (bytesTotal * 8.0) / 1000;
  bytesTotal = 0;

  std::ofstream out (m_CSVfileName.c_str (), std::ios::app);

  out << (Simulator::Now ()).GetSeconds () << " "
      << kbs << " "
      << packetsReceived << ""
      << std::endl;

  out.close ();
  packetsReceived = 0;
  Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv6Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  Inet6SocketAddress local = Inet6SocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

  return sink;
}

std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd (__FILE__);
  cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("m_nSinks", "Number of FLows", m_nSinks);
  cmd.AddValue ("m_nodes", "Number of Nodes", m_nodes);
  cmd.AddValue ("m_nPacketsPerSecond", "Number of packets sent out every second", m_nPacketsPerSecond);
  cmd.AddValue ("m_NodeSpeed", "One side of the coverage area where the nodes are scattered", m_NodeSpeed);
  //cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
  cmd.Parse (argc, argv);
  return m_CSVfileName;
}
//-----------------end--------------------------//

int main (int argc, char **argv)
{
  RoutingExperiment experiment;
  std::string CSVfileName = experiment.CommandSetup (argc,argv);
  //blank out the last output file and write the column headers
  std::ofstream out (CSVfileName.c_str ());
  out << "SimulationSecond " <<
  "ReceiveRate " <<
  "PacketsReceived " <<
  std::endl;
  out.close ();

  int nSinks = 12;
  double txp = 7.5;
  //run
  experiment.Run (nSinks, txp, CSVfileName);
}
//-----------------run-------------------//
void
RoutingExperiment::Run (int nSinks, double txp, std::string CSVfileName)
{
  Packet::EnablePrinting ();
  //m_nSinks = nSinks;
  m_txp = txp;
  m_CSVfileName = CSVfileName;

  int nWifis = m_nodes;

  double TotalTime = 200.0;
  //added
  int datarate = m_nPacketsPerSecond*64*8; //?
  std::string rate (std::to_string(datarate));
  std::string phyMode ("DsssRate11Mbps");
  std::string tr_name ("802.15");
  int nodeSpeed = m_NodeSpeed; //in m/s
  int nodePause = 0; //in s
  //m_protocolName = "protocol";

  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));

  //Set Non-unicastMode rate to unicast mode
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));

  NodeContainer adhocNodes;
  adhocNodes.Create (nWifis);
  //set seed
  // Set seed for random numbers
  SeedManager::SetSeed (167);


  //---------------mobility------------------//
  MobilityHelper mobilityAdhoc;
  int64_t streamIndex = 0; // used to get consistent mobility across scenarios

  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=300.0]"));
  pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1500.0]"));

  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  streamIndex += taPositionAlloc->AssignStreams (streamIndex);
  //bitrate = 8*number of packets per second*packetsize
  std::stringstream ssSpeed;
  ssSpeed << "ns3::UniformRandomVariable[Min=0.0|Max=" << nodeSpeed << "]";
  std::stringstream ssPause;
  ssPause << "ns3::ConstantRandomVariable[Constant=" << nodePause << "]";
  mobilityAdhoc.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
                                  "Speed", StringValue (ssSpeed.str ()),
                                  "Pause", StringValue (ssPause.str ()),
                                  "PositionAllocator", PointerValue (taPositionAlloc));
  mobilityAdhoc.SetPositionAllocator (taPositionAlloc);
  mobilityAdhoc.Install (adhocNodes);
  streamIndex += mobilityAdhoc.AssignStreams (adhocNodes, streamIndex);
  NS_UNUSED (streamIndex); // From this point, streamIndex is unused
  //--------------end of mobility-----------------//

  //-----------------for 802.15---------------//
  NS_LOG_INFO ("Create channels.");
  LrWpanHelper lrWpanHelper;
  // Add and install the LrWpanNetDevice for each node
  // lrWpanHelper.EnableLogComponents();
  NetDeviceContainer devContainer = lrWpanHelper.Install(adhocNodes);
  lrWpanHelper.AssociateToPan (devContainer, 10);
  std::cout << "Created " << devContainer.GetN() << " devices" << std::endl;
  std::cout << "There are " << adhocNodes.GetN() << " nodes" << std::endl;
  //--------------for 802.15---------------//

  //set routing 
  RipNgHelper rip;
  Ipv6ListRoutingHelper list;
  list.Add(rip,100);

  /* Install IPv4/IPv6 stack */
  NS_LOG_INFO ("Install Internet stack.");
  InternetStackHelper internetv6;
  internetv6.SetIpv4StackInstall (false);
  //internetv6.SetRoutingHelper(list);

  internetv6.Install (adhocNodes);
  // Install 6LowPan layer
  NS_LOG_INFO ("Install 6LoWPAN.");
  SixLowPanHelper sixlowpan;
  NetDeviceContainer six1 = sixlowpan.Install (devContainer);
  
  
  InternetStackHelper internet;


  //NETWORK LAYER
  NS_LOG_INFO ("Assign addresses.");
  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer adhocInterfaces = ipv6.Assign (six1);
  
  //APPLICATION LAYER
  OnOffHelper onoff1 ("ns3::UdpSocketFactory",Address ());
  onoff1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
  

  //send packets
  for (int i = 0; i < m_nSinks; i++)
    {
      //std::cout << "i=>" << i<< std::endl;
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i,1), adhocNodes.Get (i));

      AddressValue remoteAddress (Inet6SocketAddress (adhocInterfaces.GetAddress (i,1), port));
      onoff1.SetAttribute ("Remote", remoteAddress);

      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      ApplicationContainer temp = onoff1.Install (adhocNodes.Get (i + m_nSinks));
      // //drop packet
      // std::list<uint32_t> sampleList;
      // // Lose first two SYNs
      // sampleList.push_back (0);
      // sampleList.push_back (1);
      // // This time, we'll explicitly create the error model we want
      // Ptr<ReceiveListErrorModel> pem = CreateObject<ReceiveListErrorModel> ();
      // pem->SetList (sampleList);
      // devContainer.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (pem));
      temp.Start (Seconds (var->GetValue (100.0,101.0)));
      temp.Stop (Seconds (TotalTime));
    }

  std::stringstream ss;
  ss << nWifis;
  std::string nodes = ss.str ();

  std::stringstream ss2;
  ss2 << nodeSpeed;
  std::string sNodeSpeed = ss2.str ();

  std::stringstream ss3;
  ss3 << nodePause;
  std::string sNodePause = ss3.str ();

  std::stringstream ss4;
  ss4 << rate;
  std::string sRate = ss4.str ();

  //NS_LOG_INFO ("Configure Tracing.");
  //tr_name = tr_name + "_" + m_protocolName +"_" + nodes + "nodes_" + sNodeSpeed + "speed_" + sNodePause + "pause_" + sRate + "rate";

  //AsciiTraceHelper ascii;
  //Ptr<OutputStreamWrapper> osw = ascii.CreateFileStream ( (tr_name + ".tr").c_str());
  //wifiPhy.EnableAsciiAll (osw);
  AsciiTraceHelper ascii;
  MobilityHelper::EnableAsciiAll (ascii.CreateFileStream (tr_name + ".mob"));

  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();


  NS_LOG_INFO ("Run Simulation.");

  CheckThroughput ();

  Simulator::Stop (Seconds (TotalTime));
  Simulator::Run ();

  flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), false, false);

  Simulator::Destroy ();
}


