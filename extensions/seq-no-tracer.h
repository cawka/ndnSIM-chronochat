/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef SEQ_NO_TRACER_H
#define SEQ_NO_TRACER_H

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <ns3/ptr.h>

namespace ns3
{

class Application;

class SeqNoTracer
{
public:
  SeqNoTracer (Ptr<Application> app, const std::string &id, boost::shared_ptr<std::ostream> os, const std::string &type, uint32_t run);

  void
  OnNewSeqNo (const std::string &prefix, uint32_t seqNo);

  static void
  PrintHeader (boost::shared_ptr<std::ostream> os);
  
private:
  // string m_id;
  boost::shared_ptr<std::ostream> m_of;
  std::string m_type;
  std::string m_id;
  uint32_t m_run;
};

}

#endif // SEQ_NO_TRACER_H
