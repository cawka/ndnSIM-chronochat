/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
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
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include <ns3/core-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/ndnSIM-module.h>

#include <ChronoSync.ns3/sync-logic.h>
#include <ChronoSync.ns3/sync-logic-helper.h>

#include <ns3/ipv4.h>
#include <ns3/node.h>
#include <ns3/channel-list.h>

#include <boost/tuple/tuple.hpp>
#include <boost/make_shared.hpp>

#include <map>
#include <vector>

#include "standard-muc.h"
#include "seq-no-tracer.h"

using namespace ns3;
using namespace std;
using namespace boost;

NS_LOG_COMPONENT_DEFINE ("IrcService");

void
PrintTime (Time next)
{
  cout << Simulator::Now ().ToDouble (Time::S) << "s" << endl;
  Simulator::Schedule (next, PrintTime, next);
}

int Ns3Random (int i)
{
  static UniformVariable var;
  return var.GetInteger (0, i);
}

ApplicationContainer
SetupApps (AnnotatedTopologyReader &reader)
{
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
    
  NodeContainer clients; // random list of clients
  for (NodeList::Iterator node = NodeList::Begin (); node != NodeList::End (); node++)
    {
      if (Names::FindName (*node).substr (0, 6) == "client")
        {
          clients.Add (*node);
        }
    }
  std::random_shuffle (clients.begin (), clients.end (), Ns3Random);

  InternetStackHelper ipv4Helper;
  ipv4Helper.InstallAll ();
    
  reader.AssignIpv4Addresses (Ipv4Address ("10.0.0.0"));
  reader.ApplyOspfMetric ();

  NodeContainer serverNodes;
  serverNodes.Create (1);
    
  PointToPointHelper p2pH;
  NetDeviceContainer serverLink = p2pH.Install (serverNodes.Get (0), clients.Get (0));
  ipv4Helper.Install (serverNodes.Get (0));
    
  MobilityHelper m;
  m.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  m.Install (serverNodes.Get (0));
  Ipv4AddressHelper addressH (Ipv4Address ("5.5.5.0"), Ipv4Mask ("/24"));
  addressH.Assign (serverLink);
    
  Ipv4Address serverIp ("5.5.5.1");
      
  Ptr<Application> server = CreateObject<MucServer> ();
  server->SetStartTime (Seconds (0.0));
  serverNodes.Get (0)->AddApplication (server);

  Ipv4Address base ("1.0.0.0");
  Ipv4AddressHelper address (base, Ipv4Mask ("/24"));
        
  ApplicationContainer apps;
  for (NodeList::Iterator node = NodeList::Begin (); node != NodeList::End (); node++)
    {
      Ptr<Node> realNode = CreateObject<Node> ();
      m.Install (realNode);
      NetDeviceContainer nd = p2pH.Install (realNode, *node);
      ipv4Helper.Install (realNode);
      address.Assign (nd);
        
      Ptr<MucClient> client = CreateObject<MucClient> (serverIp, Names::FindName (*node).substr (7), m_inputFile);
      client->SetStartTime (Seconds (1.0)); // clients should start at the same time !
      realNode->AddApplication (client);
      Names::Add (":"+Names::FindName (*node).substr (7), client);

      apps.Add (client);

      base = Ipv4Address (base.Get () + 256);
      address.SetBase (base, Ipv4Mask ("/24"));        
    }
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // SetUpChannelTrace ();
  // SetUpDropTrace ();
  // // SetUpLosses ();
  return apps;
}


// void
// ToggleLinkStatusIpv4 (const TopologyReader::Link &link)
// {
//   NS_LOG_DEBUG ("Toggling link from "
//                 << Names::FindName (link.GetFromNetDevice ()->GetNode ())
//                 << " to "
//                 << Names::FindName (link.GetToNetDevice ()->GetNode ()));
  
//   uint32_t face1 = link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->GetInterfaceForDevice (link.GetFromNetDevice ());
//   uint32_t face2 = link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->GetInterfaceForDevice (link.GetToNetDevice ());

  
//   NS_LOG_DEBUG (link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->IsUp (face1) << ", " <<
//                 link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->IsUp (face2));

  
//   if (link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->IsUp (face1))
//     {
//       link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->SetDown (face1);
//       link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->SetMetric (face1, numeric_limits<uint16_t>::max ());
//     }
//   else
//     {
//       link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->SetUp (face1);
//       link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->SetMetric (face1, 1);
//     }


//   if (link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->IsUp (face2))
//     {
//       link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->SetDown (face2);
//       link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->SetMetric (face2, numeric_limits<uint16_t>::max ());
//     }
//   else
//     {
//       link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->SetUp (face2);
//       link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->SetMetric (face2, 1);
//     }

  
//   NS_LOG_DEBUG (link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->IsUp (face1) << ", " <<
//                 link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->IsUp (face2));
// }


// void
// PrintPIT ()
// {
//   for (NodeList::Iterator i = NodeList::Begin ();
//        i != NodeList::End ();
//        i ++)
//     {
//       cerr << Names::FindName (*i) << endl;
//       cerr << *(*i)->GetObject<Ccnx> ()->GetPit () << endl;
//     }
// }

int 
main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::PointToPointNetDevice::Mtu", StringValue ("65535")); // small cheating against mtu size
  Config::SetDefault ("ns3::TcpSocket::DelAckTimeout", StringValue ("1ms")); // effectively disable delayed ACKs to make TCP behave better
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", StringValue ("4294967295"));
  Config::SetDefault ("ns3::TcpSocket::ConnCount", StringValue ("4294967295"));
  Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (Seconds (0.1)));
  Config::SetDefault ("ns3::RttMeanDeviation::Gain", DoubleValue (0.125));
  Config::SetDefault ("ns3::RttMeanDeviation::Gain2", DoubleValue (0.25));

  uint32_t run = 0;
  string topology;
  // uint32_t loss = 0;
  // bool seqnos = false;
  // double randomLoss = 0.0;
  Time simTime = Seconds (6000);

  CommandLine cmd;
  cmd.AddValue ("run",  "Simulation run", run);
  cmd.AddValue ("topology", "Topology", topology);
  // cmd.AddValue ("loss", "Loss probability (in percents)", loss);
  // cmd.AddValue ("manualLoss", "Hard-coded link failure pattern", manualLoss);
  // cmd.AddValue ("randomLoss", "Random loss in links (double)", randomLoss);
  // cmd.AddValue ("seqnos", "Enable sequence number logging", seqnos);
  cmd.AddValue ("simTime", "Total simulation time (default 6000 seconds)", simTime);
  cmd.Parse (argc, argv);

  if (topology.empty ())
    {
      cerr << cmd;
      return 1;
    }
  
  Config::SetGlobal ("RngRun", IntegerValue (run));

  ostringstream resultFile;
  resultFile << "results/"
             << topology << "/irc-basic"
             << "seqnos-run-" << run
             << ".txt";

  AnnotatedTopologyReader reader;
  reader.SetFileName ("topologies/" + topology + ".txt");
  reader.Read ();
  
  ApplicationContainer apps = SetupApps (reader);
  
  list< shared_ptr< SeqNoTracer > > seqNoTracer;
  
  boost::shared_ptr<std::ostream> seqNoTracerFile = boost::make_shared<std::ofstream> (resultFile.str ().c_str (), ios::trunc);
  SeqNoTracer::PrintHeader (seqNoTracerFile);

  for (ApplicationContainer::Iterator app = apps.Begin ();
       app != apps.End ();
       app ++)
    {
      string id = Names::FindName (*app).substr (1);
      seqNoTracer.push_back (make_shared<SeqNoTracer> (*app, id, seqNoTracerFile, "IP", run));
    }

  // Time failureTime = Seconds (0.25 * simTime.ToDouble (Time::S));
  // Time recoveryTime = Seconds (0.75 * simTime.ToDouble (Time::S));

  // cout << simTime.ToDouble (Time::S)  << "s " << failureTime.ToDouble (Time::S) << "s " << recoveryTime.ToDouble (Time::S) << "s" << endl;

  // Simulator::Schedule (Seconds (1000.0), &Experiment::PrintTraces, &exp, Seconds (1000.0));  
  Simulator::Schedule (Seconds (500.0), PrintTime, Seconds (500.0));


  Simulator::Stop (simTime);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
