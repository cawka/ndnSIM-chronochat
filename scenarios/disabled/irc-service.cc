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
#include "ns3/internet-module.h"
#include "ns3/assert.h"
#include "ns3/callback.h"

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

  uint32_t run = 0;
  uint32_t loss = 0;
  bool seqnos = false;
  bool manualLoss = false;
  double randomLoss = 0.0;
  Time simTime = Seconds (6000);

  CommandLine cmd;
  cmd.AddValue ("run",  "Simulation run", run);
  cmd.AddValue ("loss", "Loss probability (in percents)", loss);
  cmd.AddValue ("manualLoss", "Hard-coded link failure pattern", manualLoss);
  cmd.AddValue ("randomLoss", "Random loss in links (double)", randomLoss);
  cmd.AddValue ("seqnos", "Enable sequence number logging", seqnos);
  cmd.AddValue ("simTime", "Total simulation time (default 6000 seconds)", simTime);
  cmd.Parse (argc, argv);

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

  list< shared_ptr< Validator > > validators;
  list< shared_ptr< SeqNoTracer > > seqNoTracer;

  ApplicationContainer apps;
  apps = exp.Ipv4Apps ();

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
          string id = Names::FindName (*app).substr (1);
          seqNoTracer.push_back (make_shared<SeqNoTracer> (*app, id, boost::ref (seqNoTracerFile), isIpv4?"IP":"NDN", loss, run, randomLoss));
        }
    }

  Time failureTime = Seconds (0.25 * simTime.ToDouble (Time::S));
  Time recoveryTime = Seconds (0.75 * simTime.ToDouble (Time::S));

  cout << simTime.ToDouble (Time::S)  << "s " << failureTime.ToDouble (Time::S) << "s " << recoveryTime.ToDouble (Time::S) << "s" << endl;

  Simulator::Schedule (Seconds (1000.0), &Experiment::PrintTraces, &exp, Seconds (1000.0));  
  Simulator::Schedule (Seconds (500.0), PrintTime, Seconds (500.0));


  Simulator::Stop (simTime);
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
