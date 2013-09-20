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
 *         Ilya Moiseenko <iliamo@ucla.edu>
 */

#include "sync-muc.h"

#include <ns3/assert.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/callback.h>
#include <ns3/packet.h>
#include <ns3/ndn-name.h>
#include <ns3/ndn-data.h>
#include <ns3/ndn-interest.h>
#include <ns3/ndn-face.h>
#include <ns3/ndn-fib.h>
#include <ns3/names.h>
#include <ns3/ndn-app.h>

#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("SyncMuc");

NS_OBJECT_ENSURE_REGISTERED (SyncMuc);

using namespace std;
using namespace Sync;

TypeId
SyncMuc::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::SyncMuc")
    .SetParent<ndn::App> ()
    .AddTraceSource ("SeqNoTrace",       "Sequence number trace",
                     MakeTraceSourceAccessor (&SyncMuc::m_seqNoPrefixTrace))
    .AddTraceSource ("LatestSeqNoTrace", "Latest sequence number trace (only fired from data generation)",
                     MakeTraceSourceAccessor (&SyncMuc::m_latestSeqNoTrace))
    ;
  return tid;
}

SyncMuc::SyncMuc (const string &syncPrefix, const string &mucPrefix, const string &id, const string &input, bool requestData)
  : m_logic (0)
  , m_syncPrefix (syncPrefix)
  , m_mucPrefix (mucPrefix)
  , m_rand (0, std::numeric_limits<uint32_t>::max ())
  , m_input (input.c_str ())
  , m_curMsgId (0)
  , m_requestData (requestData)
  , m_id (id)
{
  NS_ASSERT_MSG (m_input.good (), "Input file " << input << " cannot be open for reading");
  while (!m_input.eof ())
    {
      string tmp;
      getline (m_input, tmp);
      if (tmp == "") break;
    }
  m_input >> m_inputLen;
}

SyncMuc::~SyncMuc ()
{
  m_logic = 0;
}

void
SyncMuc::StartApplication ()
{
  NS_LOG_INFO ("Starting Muc");
  ndn::App::StartApplication ();

  // Set up FIB
  Ptr<ndn::Name> name = Create<ndn::Name> ();
  name->append(Names::FindName (GetNode ()));
  
  Ptr<ndn::Fib> fib = GetNode ()->GetObject<ndn::Fib> ();
  Ptr<ndn::fib::Entry> fibEntry = fib->Add (*name, m_face, 0);

  // end FIB
  
  // make face green, so it will be used primarily
  fibEntry->UpdateStatus (m_face,ndn::fib::FaceMetric::NDN_FIB_GREEN);
  
  m_logic = CreateObject<SyncLogic> (m_syncPrefix,
                                      boost::bind(&SyncMuc::Wrapper, this, _1),
                                      boost::bind(&SyncMuc::OnDataRemove, this, _1));
  
  m_logic->SetNode (GetNode ());
  m_logic->StartApplication ();

  ScheduleNextMessage ();
}

void
SyncMuc::StopApplication ()
{
  ndn::App::StopApplication ();

  // prefix is still in fib... whatever
  
  m_logic->StopApplication ();
}

void
SyncMuc::Wrapper (const vector<MissingDataInfo> &v)
{
  int n = v.size();
  for (int i = 0; i < n; i++)
  {
    OnDataUpdate (v[i].prefix, v[i].high, v[i].low);
  }
}

void
SyncMuc::OnDataUpdate (const std::string &prefix, const SeqNo &newSeq, const SeqNo &oldSeq)
{
  // NS_LOG_LOGIC (Simulator::Now ().ToDouble (Time::S) << ", prefix: " << prefix << " (we are: "<< m_mucPrefix << "), seqNo: " << newSeq);
  // NS_LOG_LOGIC (prefix << ", " << newSeq << ", " << oldSeq);
  NS_LOG_FUNCTION(this);
  
  if (m_mucPrefix == prefix)
    return; // don't request our own data
  
  uint32_t begin = 0;
  uint32_t end = newSeq.getSeq ();
  
  if (oldSeq.isValid ())
    begin = oldSeq.getSeq () + 1;

  for (uint32_t i = begin; i <= end; i++)
  {
    NS_LOG_DEBUG ("<< DATA " << prefix << " seqno: " << i);
    m_seqNoPrefixTrace (prefix, i);
      
    // NS_LOG_INFO ("> Interest for " << prefix << "/" << i);
      
    if (!m_requestData)
      continue; // just ignore
      
    Ptr<ndn::Name> name = Create<ndn::Name> ();
    name->append(prefix);
    name->append(boost::lexical_cast<std::string>(i));
    
    Ptr<ndn::Interest> interest = Create<ndn::Interest> ();
    interest->SetNonce            (m_rand.GetValue ());
    interest->SetName             (name);
    interest->SetInterestLifetime (Seconds (60.0));

    m_face->ReceiveInterest (interest);
    m_transmittedInterests (interest, this, m_face);
  }
}

void
SyncMuc::OnDataRemove (const std::string &prefix)
{
  // NS_LOG_LOGIC (Simulator::Now ().ToDouble (Time::S) <<"s\tNode: " << Simulator::GetContext () << ", prefix: "<< prefix);
  // not supported here
}


void
SyncMuc::ScheduleNextMessage ()
{
  if (m_inputLen > 0)
    {
      string id;
      double delay;
      uint32_t msgSize;
      do
        {
          m_inputLen --;
          m_input >> id >> delay >> msgSize;
        }
      while (m_inputLen > 0 && id != m_id);

      if (m_inputLen == 0 && id != m_id) return; //reached the end of input

      // cout << m_id << ": " << Seconds (delay).ToDouble (Time::S) << "s, " << Simulator::Now ().ToDouble (Time::S) << "s" << endl;
      NS_LOG_INFO("Schedule: " << Seconds (delay) - Simulator::Now ());
      Simulator::Schedule (Seconds (delay) - Simulator::Now (),
                           &SyncMuc::SendMessage, this, msgSize);
    }
}

bool
SyncMuc::SendMessage (uint32_t msgLen)
{  
  if (m_logic == 0)
    return false;
  
  NS_LOG_INFO ("> DATA: " << msgLen << " for " << m_mucPrefix << "/" << m_curMsgId);
  
  m_logic->addLocalNames (m_mucPrefix, 0, m_curMsgId);
  m_seqNoPrefixTrace ("/"+m_id, m_curMsgId);
  m_latestSeqNoTrace ("/"+m_id, m_curMsgId);

  if (m_requestData)
  {
    Ptr<ndn::Name> name = Create<ndn::Name> ();
    name->append(m_mucPrefix);
    name->append(boost::lexical_cast<std::string>(m_curMsgId));
    
    Ptr<ndn::Data> data = Create<ndn::Data> (Create<Packet> (msgLen));
    data->SetName (name);
      
    m_face->ReceiveData (data);
    m_transmittedDatas (data, this, m_face);
  }

  m_curMsgId ++;

  ScheduleNextMessage ();
  return true;
}

