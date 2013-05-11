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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/NDNabstraction-module.h"
#include "ns3/internet-module.h"
#include "ns3/assert.h"
#include "ns3/callback.h"

#include "sync-logic.h"
#include "sync-logic-helper.h"

#include <ns3/ipv4.h>
#include <ns3/node.h>
#include <ns3/channel-list.h>

#include "standard-muc.h"
#include "sync-muc.h"
#include <boost/tuple/tuple.hpp>

#include <ns3/netanim-module.h>
#include <boost/make_shared.hpp>

#include <map>
#include <vector>

using namespace ns3;
using namespace std;
using namespace boost;

NS_LOG_COMPONENT_DEFINE ("SyncEval");

#include "experiment.h"
#include "validator.h"
#include "heat-map.h"

void
PrintTime (Time next)
{
  cout << Simulator::Now ().ToDouble (Time::S) << "s" << endl;
  Simulator::Schedule (next, PrintTime, next);
}


class SeqNoTracer
{
public:
  SeqNoTracer (Ptr<Application> app, const string &id, ostream &os, const std::string &type, uint32_t loss, uint32_t run, double randomLoss)
    : m_of (os)
  {
    m_id = id;
    m_of << "#Run\tType\tLoss\tLoss2\tTime\tNode1\tNode2\tSeqNo\n";

    m_type = type;
    m_loss = loss;
    m_randomLoss = randomLoss;
    m_run = run;
    app->TraceConnectWithoutContext ("SeqNoTrace", MakeCallback (&SeqNoTracer::OnNewSeqNo, this)); 

    m_of.precision(10);
    m_of.setf(ios::fixed,ios::floatfield);
  }

  void
  OnNewSeqNo (const string &prefix, uint32_t seqNo)
  {
    m_of << m_run << "\t" << m_type << "\t" << m_loss << "\t" << m_randomLoss << "\t" << Simulator::Now ().ToDouble (Time::S) << "\t" << m_id << "\t" << prefix.substr (1) << "\t" << seqNo << "\n";
  }
  
private:
  // string m_id;
  ostream &m_of;
  string m_type;
  string m_id;
  uint32_t m_loss;
  uint32_t m_run;
  double m_randomLoss;
};


void
ToggleLinkStatusCcnx (const TopologyReader::Link &link)
{
  NS_LOG_DEBUG ("Toggling link from "
                << Names::FindName (link.GetFromNetDevice ()->GetNode ())
                << " to "
                << Names::FindName (link.GetToNetDevice ()->GetNode ()));
  
  Ptr<CcnxFace> face1 = link.GetFromNetDevice ()->GetNode ()->GetObject<Ccnx> ()->GetFaceByNetDevice (link.GetFromNetDevice ());
  Ptr<CcnxFace> face2 = link.GetToNetDevice ()  ->GetNode ()->GetObject<Ccnx> ()->GetFaceByNetDevice (link.GetToNetDevice ());

  NS_LOG_DEBUG (face1->IsUp () << ", " << face2->IsUp ());

  face1->SetUp (!face1->IsUp ());
  face2->SetUp (!face2->IsUp ());

  NS_LOG_DEBUG (face1->IsUp () << ", " << face2->IsUp ());
}

void
ToggleLinkStatusIpv4 (const TopologyReader::Link &link)
{
  NS_LOG_DEBUG ("Toggling link from "
                << Names::FindName (link.GetFromNetDevice ()->GetNode ())
                << " to "
                << Names::FindName (link.GetToNetDevice ()->GetNode ()));
  
  uint32_t face1 = link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->GetInterfaceForDevice (link.GetFromNetDevice ());
  uint32_t face2 = link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->GetInterfaceForDevice (link.GetToNetDevice ());

  
  NS_LOG_DEBUG (link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->IsUp (face1) << ", " <<
                link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->IsUp (face2));

  
  if (link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->IsUp (face1))
    {
      link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->SetDown (face1);
      link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->SetMetric (face1, numeric_limits<uint16_t>::max ());
    }
  else
    {
      link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->SetUp (face1);
      link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->SetMetric (face1, 1);
    }


  if (link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->IsUp (face2))
    {
      link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->SetDown (face2);
      link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->SetMetric (face2, numeric_limits<uint16_t>::max ());
    }
  else
    {
      link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->SetUp (face2);
      link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->SetMetric (face2, 1);
    }

  
  NS_LOG_DEBUG (link.GetFromNetDevice ()->GetNode ()->GetObject<Ipv4> ()->IsUp (face1) << ", " <<
                link.GetToNetDevice ()  ->GetNode ()->GetObject<Ipv4> ()->IsUp (face2));
}


void
PrintPIT ()
{
  for (NodeList::Iterator i = NodeList::Begin ();
       i != NodeList::End ();
       i ++)
    {
      cerr << Names::FindName (*i) << endl;
      cerr << *(*i)->GetObject<Ccnx> ()->GetPit () << endl;
    }
}

int 
main (int argc, char *argv[])
{
  Config::SetDefault ("ns3::PointToPointNetDevice::DataRate", StringValue ("100Mbps"));
  Config::SetDefault ("ns3::PointToPointChannel::Delay", StringValue ("1ns")); //actual delays are defined in topology file
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", StringValue ("2000"));

  Config::SetDefault ("ns3::PointToPointNetDevice::Mtu", StringValue ("65535")); // small cheating against mtu size
  Config::SetDefault ("ns3::TcpSocket::DelAckTimeout", StringValue ("1ms")); // effectively disable delayed ACKs to make TCP behave better
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", StringValue ("4294967295"));
  Config::SetDefault ("ns3::TcpSocket::ConnCount", StringValue ("4294967295"));
  Config::SetDefault ("ns3::TcpSocket::ConnTimeout", TimeValue (Seconds (0.1)));
  Config::SetDefault ("ns3::RttMeanDeviation::Gain", DoubleValue (0.125));
  Config::SetDefault ("ns3::RttMeanDeviation::Gain2", DoubleValue (0.25));
  
  // Set maximum number of packets that will be cached (default 100)
  Config::SetDefault ("ns3::CcnxContentStore::Size", StringValue ("10000")); // 10k messages will be cached
  Config::SetDefault ("ns3::CcnxL3Protocol::CacheUnsolicitedData", StringValue ("true")); // to "push" data from apps to ContentStore
  // Config::SetDefault ("ns3::CcnxL3Protocol::RandomDataDelaying", StringValue ("true")); // to reorder data and interests

  bool isCcnx = false;
  bool isIpv4 = false;
  bool isGenerate = false;
  uint32_t numClients = 10;
  bool isVis = false;
  bool ccnxData = true;
  uint32_t run = 0;
  uint32_t loss = 0;
  bool seqnos = false;
  bool manualLoss = false;
  double randomLoss = 0.0;
  Time simTime = Seconds (6000);

  CommandLine cmd;
  cmd.AddValue ("ccnx", "Enable ccnx", isCcnx);
  cmd.AddValue ("ipv4", "Enable ipv4", isIpv4);
  cmd.AddValue ("generate", "Generate input set (topology and message pattern)", isGenerate);
  cmd.AddValue ("numClients", "Number of clients (make sense only when --generate=1)", numClients);
  cmd.AddValue ("ccnxData",   "Request data in ccnx mode (otherwise only state messages will be exchanged", ccnxData);
  cmd.AddValue ("run",  "Simulation run", run);
  cmd.AddValue ("vis", "Enable visualizer", isVis);
  cmd.AddValue ("loss", "Loss probability (in percents)", loss);
  cmd.AddValue ("manualLoss", "Hard-coded link failure pattern", manualLoss);
  cmd.AddValue ("randomLoss", "Random loss in links (double)", randomLoss);
  cmd.AddValue ("seqnos", "Enable sequence number logging", seqnos);
  cmd.AddValue ("simTime", "Total simulation time (default 6000 seconds)", simTime);
  cmd.Parse (argc, argv);

  if (isVis)
    {
      GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::VisualSimulatorImpl"));
    }

  Config::SetGlobal ("RngRun", IntegerValue (run));
  ostringstream input;
  input << "evaluation/run-input/"
        << "run-" << run
        << "-numClients-" << numClients
        << ".txt";

  ostringstream inputLosses;
  inputLosses << "evaluation/run-input/"
              << "run-" << run
              << "-numClients-" << numClients
              << "-losses-" << loss
              << ".txt";
  
  Experiment exp (run, numClients, loss, randomLoss);
  exp.m_inputFile = input.str ();
  if (isGenerate)
    {
      ofstream os (input.str ().c_str ());

      UniformVariable interMessageDelay (1.0, 10.0); // seconds
      UniformVariable messageSize (20, 200); // bytes
      
      cout << "Randomizing clients" << endl;
      exp.RandomizeClients ();
      for (vector< uint32_t >::iterator i = exp.clientIds.begin ();
           i != exp.clientIds.end ();
           i++)
        {
          os << *i << endl;
        }
      
      os << endl; // separator

      cout << "Generating input data set for clients" << endl;
      uint32_t totalNumberOfMessages = 1000; //exp.clientIds.size () * 100;
      os << totalNumberOfMessages << endl;
      os.precision(5);
      os.setf (ios::fixed, ios::floatfield);
      double absTime = 0;
      for (uint32_t msg = 0; msg < totalNumberOfMessages; msg++)
        {
          UniformVariable randClient (0, exp.clientIds.size ());
          uint32_t clientId = randClient.GetValue ();
          uint32_t client = exp.clientIds[clientId];

          absTime += interMessageDelay.GetValue ();
          os << client << "\t" << absTime << "\t" << (uint32_t)messageSize.GetValue () << endl;
        }

      os.close ();

      cout << "Generating loss data" << endl;
      os.open (inputLosses.str ().c_str ());
      os << exp.reader->GetLinks ().size () << endl;

      UniformVariable randomDice;
      for (uint32_t i = 0; i < exp.reader->GetLinks ().size (); i++ )
        {
          double x = randomDice.GetValue ();
          // cout << x << endl;
          if (x <= (1.0*loss)/100)
            {
              os << i << "\t" << true << endl;
            }
          else
            {
              os << i << "\t" << false << endl;
            }
        }
      os.close ();
      
      cout << "Done generating data" << endl;
      return 0;
    }
  else
    {
      cout << "Reading generated data from [" << input.str () << "]" << endl;
      ifstream is (input.str ().c_str (), ios_base::in);
      if (is.eof () || is.bad () || !is.good ())
        {
          cerr << "Cannot open input file" << endl;
          return 1;
        }
      for (uint32_t i = 0; i < numClients; i++)
        {
          uint32_t id;
          is >> id;
          exp.clientIds.push_back (id);
        }
    }

  if (!(isCcnx ^ isIpv4))
    {
      cerr << "Either --ipv4=1 or --ccnx=1 should be specified, but not both" << endl;
      return 1;
    }

  list< shared_ptr< Validator > > validators;
  list< shared_ptr< SeqNoTracer > > seqNoTracer;

  ApplicationContainer apps;
  if (isCcnx)
    {
      exp.m_type = "ccnx";
      exp.m_ccnxData = ccnxData;
      apps = exp.CcnxApps ();
    }
  if (isIpv4)
    {
      exp.m_type = "ipv4";
      apps = exp.Ipv4Apps ();
    }

  for (ApplicationContainer::Iterator app = apps.Begin ();
       app != apps.End ();
       app ++)
    {
      validators.push_back (make_shared<Validator> (*app));
      AppKnowledge[ "/" + Names::FindName (*app).substr (1) ] = make_shared<CurrentAppKnowledge> (*app);
    }

  GlobalKnowledge = make_shared<CurrentGlobalKnowledge> ();

  ofstream seqNoTracerFile;
  if (seqnos)
    {
      ostringstream seqNoMapFileName;
      seqNoMapFileName << "evaluation/run-output/seqnos"
                       << "-numClients-" << numClients
                       << ".txt";

      seqNoTracerFile.open (seqNoMapFileName.str ().c_str (), ios::app);

      for (ApplicationContainer::Iterator app = apps.Begin ();
           app != apps.End ();
           app ++)
        {
          // string id = Names::FindName (apps.Get (0)).substr (1);
          // seqNoTracer.push_back (make_shared<SeqNoTracer> (apps.Get (0), id, boost::ref (seqNoTracerFile), isIpv4?"IP":"NDN", loss, run, randomLoss));
          string id = Names::FindName (*app).substr (1);
          seqNoTracer.push_back (make_shared<SeqNoTracer> (*app, id, boost::ref (seqNoTracerFile), isIpv4?"IP":"NDN", loss, run, randomLoss));
        }
    }

  Time failureTime = Seconds (0.25 * simTime.ToDouble (Time::S));
  Time recoveryTime = Seconds (0.75 * simTime.ToDouble (Time::S));

  cout << simTime.ToDouble (Time::S)  << "s " << failureTime.ToDouble (Time::S) << "s " << recoveryTime.ToDouble (Time::S) << "s" << endl;
  
  Simulator::Stop (simTime);

  HitMap hitmap;
  Simulator::Schedule (Seconds (1.0), &HitMap::Periodic, &hitmap, Seconds (1.0));

  Simulator::Schedule (Seconds (1000.0), &Experiment::PrintTraces, &exp, Seconds (1000.0));  
  Simulator::Schedule (Seconds (3.0), &Experiment::SetUpLosses, &exp);
  
  if (loss > 0)
    {
      ifstream is (inputLosses.str ().c_str ());
      uint32_t N;
      is >> N;
      
      std::list<TopologyReader::Link>::const_iterator linkI = exp.reader->GetLinks ().begin ();

      if (manualLoss)
        {
          Simulator::Stop (Seconds (1200));
          if (isIpv4)
            {
              // Simulator::Schedule (Seconds (200), ToggleLinkStatusIpv4, *linkI);
              // Simulator::Schedule (Seconds (800), ToggleLinkStatusIpv4, *linkI);
              // Simulator::Schedule (failureTime,  ToggleLinkStatusIpv4, *linkI);
              // Simulator::Schedule (recoveryTime, ToggleLinkStatusIpv4, *linkI);

              // Simulator::Schedule (failureTime + Seconds (30),  Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
              // Simulator::Schedule (recoveryTime + Seconds (30), Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
            }
          else
            {
              Simulator::Schedule (Seconds (200), ToggleLinkStatusCcnx, *linkI);
              Simulator::Schedule (Seconds (800), ToggleLinkStatusCcnx, *linkI);

              linkI ++;
              linkI ++;
              
              Simulator::Schedule (Seconds (400), ToggleLinkStatusCcnx, *linkI);
              Simulator::Schedule (Seconds (1000), ToggleLinkStatusCcnx, *linkI);
              // Simulator::Schedule (failureTime,  ToggleLinkStatusCcnx, *linkI);
              // Simulator::Schedule (recoveryTime, ToggleLinkStatusCcnx, *linkI);
            }
        }
      else
        {
          for (uint32_t linkId = 0; linkId < N; linkId ++)
            {
              uint32_t link;
              bool     failed;

              is >> link >> failed;

              if (failed)
                {
                  if (isIpv4)
                    {
                      Simulator::Schedule (failureTime,  ToggleLinkStatusIpv4, *linkI);
                      Simulator::Schedule (recoveryTime, ToggleLinkStatusIpv4, *linkI);
                    }
                  else
                    {
                      Simulator::Schedule (failureTime,  ToggleLinkStatusCcnx, *linkI);
                      Simulator::Schedule (recoveryTime, ToggleLinkStatusCcnx, *linkI);
                    }

                  if (isIpv4 && recoveryTime - failureTime > Seconds (30))
                    {
                      Simulator::Schedule (failureTime + Seconds (30), Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
                      if (simTime - recoveryTime > Seconds (30))
                        {
                          Simulator::Schedule (recoveryTime + Seconds (30), Ipv4GlobalRoutingHelper::RecomputeRoutingTables);
                        }
                    }
                }
          
              linkI ++;
            }
        }
    }

  // Simulator::Schedule (Seconds (7), PrintPIT);

  // Ptr<OutputStreamWrapper> routingStream =
  //   Create<OutputStreamWrapper> ("node-routes",
  //                                std::ios::out);
  // Ipv4GlobalRoutingHelper ipv4;
  // ipv4.PrintRoutingTableAllEvery (Seconds (5), routingStream);

  // InternetStackHelper h;
  // NodeList::Iterator x = NodeList::Begin ();
  // x++; x++; x++;
  // h.EnablePcapIpv4 ("node3", *x);
  
  // AnimationInterface anim ("animation.xml");
  // anim.EnablePacketMetadata (true);
  
  // Packet::EnablePrinting ();
  // Packet::EnableChecking ();
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Schedule (Seconds (500.0), PrintTime, Seconds (500.0));
  // Simulator::Schedule (Seconds (1.0), PrintTime, Seconds (1.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done!");

  ostringstream heatMapFileName;
  heatMapFileName << "evaluation/run-output/heatmap-"
                  << "numClients-" << numClients
                  << ".txt";

  ofstream ofHeatMap (heatMapFileName.str ().c_str (), ios::out | ios::app);
  hitmap.Print (ofHeatMap, run, loss, isIpv4?"IP" : "NDN", randomLoss);

  return 0;
}
