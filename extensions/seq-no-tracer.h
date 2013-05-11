/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef SEQ_NO_TRACER_H
#define SEQ_NO_TRACER_H

#include <ns3/internet-module.h>

using namespace ns3;
using namespace std;

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

#endif // SEQ_NO_TRACER_H
