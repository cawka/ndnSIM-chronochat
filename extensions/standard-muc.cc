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

#include "standard-muc.h"

#include <ns3/tcp-socket.h>
#include <ns3/tcp-socket-factory.h>
#include <ns3/assert.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include <ns3/callback.h>
#include <ns3/inet-socket-address.h>
#include <ns3/packet.h>
#include <boost/lexical_cast.hpp>

NS_LOG_COMPONENT_DEFINE ("Muc");

using namespace std;

const uint16_t MucPort = 12345;

NS_OBJECT_ENSURE_REGISTERED (MucClient);

class ClientAndSeqNoHeader : public Header
{
public:
  ClientAndSeqNoHeader ()
    : m_clientId (-1)
    , m_seqNo (-1)
    , m_length (-1)
  {
  }
  
  ClientAndSeqNoHeader (uint32_t client, uint32_t seqno, uint32_t length)
    : m_clientId (client)
    , m_seqNo (seqno)
    , m_length (length)
  {
  }
  
  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ClientAndSeqNoHeader")
      .SetParent<Header> ()
      ;
    return tid;
  }
  
  virtual TypeId GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }
  
  virtual void Print (std::ostream &os) const
  {
    os << "Client: " << m_clientId << ", seq: " << m_seqNo;
  }
  
  virtual uint32_t GetSerializedSize (void) const
  {
    return 4 + 4 + 4;
  }
  
  virtual void Serialize (Buffer::Iterator start) const
  {
    start.WriteU32 (m_clientId);
    start.WriteU32 (m_seqNo);
    start.WriteU32 (m_length);
  }
  
  virtual uint32_t Deserialize (Buffer::Iterator start)
  {
    Buffer::Iterator oldstart = start;
    
    m_clientId = start.ReadU32 ();
    m_seqNo    = start.ReadU32 ();
    m_length   = start.ReadU32 ();

    return start.GetDistanceFrom (oldstart);
  }

  uint32_t
  GetClientId () const { return m_clientId; }

  uint32_t
  GetSeqNo () const { return m_seqNo; }

  uint32_t
  GetLength () const { return m_length; }
  
private:
  uint32_t m_clientId;
  uint32_t m_seqNo;
  uint32_t m_length;
};

TypeId
MucClient::GetTypeId ()
{
  static TypeId tid = TypeId ("MucClient")
    .SetParent<Application> ()
    .AddTraceSource ("SeqNoTrace",       "Sequence number trace",
                     MakeTraceSourceAccessor (&MucClient::m_seqNoPrefixTrace))
    .AddTraceSource ("LatestSeqNoTrace", "Latest sequence number trace (only fired from data generation)",
                     MakeTraceSourceAccessor (&MucClient::m_latestSeqNoTrace))
    ;
  return tid;
}


MucServer::MucServer ()
{
  // ?? nothing
}

void
MucServer::StartApplication ()
{
  m_socket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
  m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), MucPort));
  m_socket->Listen ();

  m_socket->SetAcceptCallback (MakeCallback (&MucServer::OnConnectionRequest, this),
                               MakeCallback (&MucServer::OnConnectionAccept, this));
}

void
MucServer::StopApplication ()
{
  for (ClientList::iterator client = m_clients.begin ();
       client != m_clients.end ();
       client ++)
    {
      (*client)->Close ();
    }  
}

bool
MucServer::OnConnectionRequest (Ptr< Socket > clientSocket, const Address &addr)
{
  NS_LOG_DEBUG ("Connect request");
  return true; //always accept incoming connection
}
  
void
MucServer::OnConnectionAccept (Ptr< Socket > clientSocket, const Address &addr)
{
  NS_LOG_DEBUG ("New connection from: " << InetSocketAddress::ConvertFrom (addr).GetIpv4 ());
  
  m_clients.push_back (clientSocket);
  clientSocket->SetRecvCallback (MakeCallback (&MucServer::RebroadcastIncomingMsg, this));

  m_socket->SetCloseCallbacks (MakeNullCallback< void, Ptr< Socket > > (),
                               MakeCallback (&MucServer::OnError, this));
}

void
MucServer::OnError (Ptr<Socket> socket)
{
  NS_LOG_DEBUG ("Some error happened with socket " << socket);
  m_clients.remove (socket);
}

void
MucServer::RebroadcastIncomingMsg (Ptr< Socket > clientSocket)
{
  ReadBody (clientSocket);

  ClientAndSeqNoHeader hdr;
  
  // cout << "# of clients: " << m_clients.size () << endl;
  while (clientSocket->GetRxAvailable () >= hdr.GetSerializedSize ())
    {
      Ptr<Packet> incomingPacket = clientSocket->Recv (hdr.GetSerializedSize (), 0);
      incomingPacket->RemoveHeader (hdr);
      
      m_bodyToRead[clientSocket] += hdr.GetLength ();

      ReadBody (clientSocket);

      Ptr<Packet> outPacket = Create<Packet> (hdr.GetLength ());
      outPacket->AddHeader (hdr);
      
      // NS_LOG_DEBUG ("RebroadcastedSize: " << incomingPacket->GetSize ());
      // NS_ASSERT (incomingPacket->PeekPacketTag<TypeTag> () != 0);
      // NS_LOG_DEBUG ("Tag: " << incomingPacket->PeekPacketTag<TypeTag> ());

      uint32_t i = 0;
      for (ClientList::iterator client = m_clients.begin ();
           client != m_clients.end ();
           client ++)
        {
          i++;
          if (*client == clientSocket)
            {
              // NS_LOG_DEBUG ("SAME LOGIC");
              continue; // don't send message back to the originator
            }

          // cout << "Send to client #" << i << endl;
          (*client)->Send (outPacket->Copy ());
        }
    }
}

void
MucServer::ReadBody (Ptr< Socket > clientSocket)
{
  while (clientSocket->GetRxAvailable () > 0 && m_bodyToRead[clientSocket] > 0)
    {
      Ptr<Packet> incomingPacket = clientSocket->Recv (m_bodyToRead[clientSocket], 0);
      if (incomingPacket == 0)
        return;
      
      m_bodyToRead[clientSocket] -= incomingPacket->GetSize ();
    }
}


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////


MucClient::MucClient (Ipv4Address server, const string &id, const string &input)
  : m_socket (0)
  , m_serverAddress (InetSocketAddress (server, MucPort))
  , m_input (input.c_str ())
  , m_curMsgId (0)
  , m_id (id)
  , m_jitter (0, 100)
  , m_bodyToRead (0)
{
  NS_ASSERT_MSG (m_input.good (), "Input file " << input << " cannot be open for reading");
  NS_LOG_DEBUG ("Client for server " << server);
  while (!m_input.eof ())
    {
      string tmp;
      getline (m_input, tmp);
      if (tmp == "") break;
    }
  m_input >> m_inputLen;
}


void
MucClient::StartApplication ()
{
  Ptr<Socket> socket = Socket::CreateSocket (GetNode (), TcpSocketFactory::GetTypeId ());
  TryConnect (socket);
}

void
MucClient::TryConnect (Ptr<Socket> socket)
{
  NS_LOG_DEBUG ("Attempting to make connection to " << InetSocketAddress::ConvertFrom (m_serverAddress).GetIpv4 ());
  
  socket->SetConnectCallback (MakeCallback (&MucClient::OnConnectSucceed, this),
                              MakeCallback (&MucClient::OnConnectFailed,  this));
  socket->Connect (m_serverAddress);
}

void
MucClient::StopApplication ()
{
  m_retryEvent.Cancel ();
  m_socket->Close ();
  m_socket = 0;
}

void
MucClient::OnConnectSucceed (Ptr<Socket> socket)
{
  NS_LOG_DEBUG ("Connected");
  m_socket = socket;

  m_socket->SetCloseCallbacks (MakeCallback (&MucClient::OnShutdown, this),
                               MakeCallback (&MucClient::OnError,    this));

  m_socket->SetRecvCallback (MakeCallback (&MucClient::OnRecvMessage, this));
  
  ScheduleNextMessage ();
}

void
MucClient::OnConnectFailed (Ptr<Socket> socket)
{
  NS_LOG_DEBUG ("Connect failed, retry in 30+jitter seconds " << socket);
  m_socket = 0;
  // try again
  m_retryEvent.Cancel ();
  m_retryEvent = Simulator::Schedule (Seconds (30.0) + MilliSeconds (m_jitter.GetValue ()),
                                      &MucClient::TryConnect, this, socket);
}

void
MucClient::OnShutdown (Ptr<Socket> socket)
{
  m_socket = 0;
  // do nothing more
}

void
MucClient::OnError (Ptr<Socket> socket)
{
  NS_LOG_DEBUG ("Something wrong happened with " << socket);
  m_socket->SetCloseCallbacks (MakeNullCallback< void, Ptr< Socket > > (),
                               MakeNullCallback< void, Ptr< Socket > > ());
  m_socket = 0;
  // try again
  m_retryEvent.Cancel ();
  m_retryEvent = Simulator::Schedule (Seconds (30.0) + MilliSeconds (m_jitter.GetValue ()),
                                      &MucClient::TryConnect, this, socket);
}

void
MucClient::ScheduleNextMessage ()
{
  if (m_inputLen > 0)
    {
      string id;
      double delay;
      uint32_t msgSize;
      do
        {
          m_input >> id >> delay >> msgSize;
          // NS_LOG_DEBUG (id << ", " << delay << ", " << msgSize << " (" << m_id << ", " << m_inputLen << ")");
          m_inputLen --;
        }
      while (m_inputLen > 0 && id != m_id);

      if (m_inputLen == 0 && id != m_id) return; //reached the end of input

      if (Simulator::Now () > Seconds (delay))
        Simulator::Schedule (Seconds (0),
                             &MucClient::SendMessage, this, msgSize);
      else
        Simulator::Schedule (Seconds (delay) - Simulator::Now (),
                             &MucClient::SendMessage, this, msgSize);
    }
}

bool
MucClient::SendMessage (uint32_t msgLen)
{
  if (m_socket == 0) // not connected to server
    return false;

  // NS_LOG_DEBUG ("Sending message len: " << msgLen);
  NS_LOG_DEBUG (">> From " << m_id << ", seqNo: " << m_curMsgId << ", size: " << msgLen);

  Ptr<Packet> pkt = Create<Packet> (msgLen);
  ClientAndSeqNoHeader hdr (boost::lexical_cast<uint32_t> (m_id), m_curMsgId, msgLen);
  pkt->AddHeader (hdr);

  int ret = m_socket->Send (pkt);
  // NS_LOG_DEBUG ("Send return: " << ret);

  m_seqNoPrefixTrace ("/"+m_id, m_curMsgId);
  m_latestSeqNoTrace ("/"+m_id, m_curMsgId);

  m_curMsgId ++;
  
  ScheduleNextMessage ();

  return true;
}

void
MucClient::ReadBody (Ptr< Socket > clientSocket)
{
  while (clientSocket->GetRxAvailable () > 0 && m_bodyToRead > 0)
    {
      Ptr<Packet> incomingPacket = clientSocket->Recv (m_bodyToRead, 0);
      if (incomingPacket == 0)
        return;
      
      m_bodyToRead -= incomingPacket->GetSize ();
    }
}

void
MucClient::OnRecvMessage (Ptr< Socket > clientSocket)
{
  ClientAndSeqNoHeader hdr;

  ReadBody (clientSocket);
  
  while (clientSocket->GetRxAvailable () >= hdr.GetSerializedSize ())
    {
      Ptr<Packet> incomingPacket = clientSocket->Recv (hdr.GetSerializedSize (), 0);
      if (incomingPacket == 0)
        return;

      NS_ASSERT (incomingPacket->GetSize () == hdr.GetSerializedSize ());
      
      uint32_t remLen = incomingPacket->RemoveHeader (hdr);
      // NS_LOG_DEBUG ("Removed: " << remLen);
      
      m_seqNoPrefixTrace ("/"+boost::lexical_cast<string> (hdr.GetClientId ()), hdr.GetSeqNo ());
      NS_LOG_DEBUG ("<< From " << hdr.GetClientId () << ", seqno: " << hdr.GetSeqNo () << ", size: " << hdr.GetLength ());

      m_bodyToRead += hdr.GetLength ();

      ReadBody (clientSocket);
      // NS_LOG_DEBUG ("Message size: " << hdr.GetLength () << ", read: " << incomingPacket->GetSize ());
    }
}
