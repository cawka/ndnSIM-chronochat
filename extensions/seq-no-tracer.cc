/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "seq-no-tracer.h"

#include <ns3/core-module.h>
#include <ns3/network-module.h>

using namespace std;

namespace ns3
{

SeqNoTracer::SeqNoTracer (Ptr<Application> app, const string &id, boost::shared_ptr<std::ostream> os, const std::string &type, uint32_t run)
  : m_of (os)
{
  m_id = id;

  m_type = type;
  m_run = run;
  app->TraceConnectWithoutContext ("SeqNoTrace", MakeCallback (&SeqNoTracer::OnNewSeqNo, this)); 

  m_of->precision(10);
  m_of->setf(ios::fixed,ios::floatfield);
}

void
SeqNoTracer::OnNewSeqNo (const string &prefix, uint32_t seqNo)
{
  *m_of << Simulator::Now ().ToDouble (Time::S) << "\t"
        << m_run << "\t"
        << m_type << "\t"
        << m_id << "\t"
        << prefix.substr (1)
        << "\t" << seqNo << "\n";
}

// static
void
SeqNoTracer::PrintHeader (boost::shared_ptr<std::ostream> os)
{
  *os << "Time\tRun\tType\tNode1\tNode2\tSeqNo\n";
}

}
