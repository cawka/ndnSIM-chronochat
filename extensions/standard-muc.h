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

#ifndef STANDARD_MUC_H
#define STANDARD_MUC_H

#include <ns3/application.h>
#include <list>
#include <map>
#include <ns3/socket.h>
#include <ns3/address.h>
#include <ns3/traced-callback.h>
#include <ns3/random-variable.h>
#include <fstream>

using namespace ns3;

class MucServer : public Application
{
public:
  MucServer ();

  void
  StartApplication ();

  void
  StopApplication ();

private:
  bool
  OnConnectionRequest (Ptr< Socket > clientSocket, const Address &addr);
  
  void
  OnConnectionAccept (Ptr< Socket > clientSocket, const Address &addr);

  void
  OnError (Ptr<Socket> socket);  
  
  void
  RebroadcastIncomingMsg (Ptr< Socket > clientSocket);
  
  void
  ReadBody (Ptr< Socket > clientSocket);

private:
  Ptr<Socket> m_socket;

  typedef std::list< Ptr<Socket> > ClientList;
  ClientList m_clients;
  std::map< Ptr<Socket>, uint32_t > m_bodyToRead;
};


class MucClient : public Application
{
public:
  static TypeId
  GetTypeId ();
  
  MucClient (Ipv4Address server, const std::string &id, const std::string &input);

private:
  void
  StartApplication ();

  void
  StopApplication ();
  
private:
  void
  TryConnect (Ptr<Socket> socket);
  
  void
  OnConnectSucceed (Ptr<Socket> socket);

  void
  OnConnectFailed (Ptr<Socket> socket);

  void
  OnShutdown (Ptr<Socket> socket);

  void
  OnError (Ptr<Socket> socket);

  void
  ScheduleNextMessage ();
  
  /**
   * @brief Generate and send message to the server
   */
  bool
  SendMessage (uint32_t msgLen); 

  void
  OnRecvMessage (Ptr< Socket > clientSocket);

  void
  ReadBody (Ptr< Socket > clientSocket);
  
private:
  Ptr<Socket> m_socket;
  Address m_serverAddress;
  EventId m_retryEvent;
  std::ifstream m_input; // file from where data will be generated
  uint32_t m_inputLen;

  uint32_t m_curMsgId;
  std::string m_id;

  UniformVariable m_jitter;
  uint32_t m_bodyToRead;
  
  TracedCallback< const std::string &/*mucPrefix*/, uint32_t/*seqNo*/ > m_seqNoPrefixTrace;  
  TracedCallback< const std::string &/*mucPrefix*/, uint32_t/*seqNo*/ > m_latestSeqNoTrace;  
};


#endif // STANDARD_MUC_H
