/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * License http://www.gnu.org/licenses/gpl-3.0.txt
 */

#ifndef P2P_CHANNEL_TRACER_H
#define P2P_CHANNEL_TRACER_H

namespace ns3
{

class P2pChannelTracer
{
public:
  
  ~P2pChannelTracer ()
  {
    m_trace.close ();
    PrintTraces (Seconds(10000000));
  }
  
  void SetUpChannelTrace ()
  {
    ostringstream traceFile;
    traceFile << "evaluation/run-output/packets"
              << "-numClients-" << numClients;
    traceFile << ".txt";

    cout << "Writing output into [" << traceFile.str () << "]" << endl; 

    m_trace.open (traceFile.str ().c_str (), ios_base::app);
    m_trace.precision (10);
    m_trace.setf(ios::fixed,ios::floatfield);

    m_trace //<< "Time" << "\t"
      << "# Time\tType\tRun\tLoss\tRandomLoss\tPkt\t"
      << "ChannelID" << "\t"
      << "NodeFrom" << "\t"
      << "NodeTo" << "\t"
      << "CountData" << "\t"
      << "CountInterests" << "\n";
    // << "Type" << "\n";

    Config::Connect ("/ChannelList/*/TxRxPointToPoint",
                     MakeCallback (&Experiment::P2pChannelTrace, this));
  }

  void SetUpDropTrace ()
  {
    // ostringstream traceFile;
    // traceFile << "evaluation/run-output/drops"
    //           << "-numClients-" << numClients;
    // traceFile << ".txt";

    // cout << "Writing output into [" << traceFile.str () << "]" << endl; 

    // m_trace2.open (traceFile.str ().c_str (), ios_base::app);
    // m_trace2.precision (10);
    // m_trace2.setf(ios::fixed,ios::floatfield);

    // m_trace2 //<< "Time" << "\t"
    //   << "# Time\tType\tRun\tLoss\tRandomLoss\t"
    //   << "ChannelID" << "\t"
    //   << "NodeFrom" << "\t"
    //   << "NodeTo" << "\t"
    //   << "CountData" << "\t"
    //   << "CountInterests" << "\n";
    // // << "Type" << "\n";

    Config::Connect ("/ChannelList/*/DropPointToPoint",
                     MakeCallback (&Experiment::DropChannelTrace, this));
  }

private:
  void
  Connect ();
  

  void
  P2pChannelTrace (std::string,
                   uint32_t channel,
		   Ptr<const Packet> p,
                   Ptr<NetDevice> tx,
                   Ptr<NetDevice> rx,
                   Time txTime,
                   Time rxTime)
  {
    PacketsInChannel::mapped_type &tuple = m_packetsInChannels [channel];

    tuple.get<0> () = ChannelList::GetChannel (channel)->GetDevice (0)->GetNode ()->GetId ();
    tuple.get<1> () = ChannelList::GetChannel (channel)->GetDevice (1)->GetNode ()->GetId ();      
      
    tuple.get<2> () ++;
  }

  void
  DropChannelTrace (std::string,
                   uint32_t channel,
		   Ptr<const Packet> p,
                   Ptr<NetDevice> tx,
                   Ptr<NetDevice> rx,
                   Time txTime,
                   Time rxTime)
  {
    NS_LOG_DEBUG ("From " << Names::FindName(tx->GetNode ()) << " to " << Names::FindName(rx->GetNode ()));
    PacketsInChannel::mapped_type &tuple = m_dropsInChannels [channel];

    tuple.get<0> () = ChannelList::GetChannel (channel)->GetDevice (0)->GetNode ()->GetId ();
    tuple.get<1> () = ChannelList::GetChannel (channel)->GetDevice (1)->GetNode ()->GetId ();      
      
    tuple.get<2> () ++;
  }



  void
  PrintTraces (Time period)
  {
    for (PacketsInChannel::iterator i = m_packetsInChannels.begin ();
         i != m_packetsInChannels.end ();
         i ++)
      {
        m_trace << Simulator::Now ().ToDouble (Time::S) << "\t"
                << ((m_type=="ccnx")?"NDN":"IP") << "\t"
                << run << "\t"
                << loss << "\t"
                << randomLoss << "\t"
                << "OK\t"
                << i->first << "\t"
                << i->second.get<0> () << "\t"
                << i->second.get<1> () << "\t"
                << i->second.get<2> () << "\t"
                << i->second.get<3> () << "\n";
      }
    m_packetsInChannels.clear ();

    for (PacketsInChannel::iterator i = m_dropsInChannels.begin ();
         i != m_dropsInChannels.end ();
         i ++)
      {
        m_trace << Simulator::Now ().ToDouble (Time::S) << "\t"
                << ((m_type=="ccnx")?"NDN":"IP") << "\t"
                << run << "\t"
                << loss << "\t"
                << randomLoss << "\t"
                << "Drop\t"
                << i->first << "\t"
                << i->second.get<0> () << "\t"
                << i->second.get<1> () << "\t"
                << i->second.get<2> () << "\t"
                << i->second.get<3> () << "\n";
      }
    m_dropsInChannels.clear ();

    Simulator::Schedule (period, &Experiment::PrintTraces, this, period);
  }
  
  
private:
  typedef map<uint32_t, tuple <uint64_t, uint64_t, uint64_t, uint64_t> > PacketsInChannel;
  PacketsInChannel m_packetsInChannels;
  PacketsInChannel m_dropsInChannels;
};

}

#endif // P2P_CHANNEL_TRACER_H
