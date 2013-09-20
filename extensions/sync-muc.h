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

#ifndef SYNC_MUC_H
#define SYNC_MUC_H

#include <ns3/ndn-app.h>
#include <ns3/traced-callback.h>
#include <ns3/type-id.h>
#include <fstream>
#include "sync-logic.h"

using namespace ns3;
using namespace Sync;

class SyncMuc : public ndn::App
{
public:
  static TypeId
  GetTypeId ();
  
  SyncMuc (const std::string &syncPrefix, const std::string &mucPrefix, const std::string &id, const std::string &input, bool requestData);
  ~SyncMuc ();
  
protected:
  virtual void
  StartApplication ();

  virtual void
  StopApplication ();
  
private:
  void
  Wrapper (const std::vector<MissingDataInfo> &v);

  void
  OnDataUpdate (const std::string &prefix, const SeqNo &newSeq, const SeqNo &/*oldSeq*/);

  void
  OnDataRemove (const std::string &prefix);

  void
  ScheduleNextMessage ();
  
  /**
   * @brief Generate and send message to the server
   */
  bool
  SendMessage (uint32_t msgLen); 
  
private:
  Ptr<SyncLogic> m_logic;
  std::string m_syncPrefix;
  std::string m_mucPrefix;
  ns3::UniformVariable m_rand; // nonce generator

  EventId m_retryEvent;
  std::ifstream m_input; // file from where data will be generated
  uint32_t m_inputLen;

  uint32_t m_curMsgId;
  bool     m_requestData;
  std::string m_id;

  TracedCallback< const std::string &/*mucPrefix*/, uint32_t/*seqNo*/ > m_seqNoPrefixTrace;  
  TracedCallback< const std::string &/*mucPrefix*/, uint32_t/*seqNo*/ > m_latestSeqNoTrace;  
};


#endif // SYNC_MUC_H
