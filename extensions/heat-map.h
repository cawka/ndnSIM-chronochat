/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

class CurrentKnowldege : public std::map< string, uint32_t >
{
public:
  virtual void
  OnNewSeqNo (const string &prefix, uint32_t seqNo)
  {
    (*this)[prefix] = seqNo+1; 
  }
};

class CurrentGlobalKnowledge : public CurrentKnowldege
{
public:
  CurrentGlobalKnowledge ()
  {
    Config::ConnectWithoutContext ("/NodeList/*/ApplicationList/*/LatestSeqNoTrace",
                                   MakeCallback (&CurrentGlobalKnowledge::OnNewSeqNo, this));
  }

  virtual void
  OnNewSeqNo (const string &prefix, uint32_t seqNo)
  {
    // NS_LOG_DEBUG (prefix << ", " << seqNo);
    (*this)[prefix] = seqNo+1; 
  }
};

class CurrentAppKnowledge : public CurrentKnowldege
{
public:
  CurrentAppKnowledge (Ptr<Application> app)
  {
    app->TraceConnectWithoutContext ("SeqNoTrace", MakeCallback (&CurrentAppKnowledge::OnNewSeqNo, this)); 
  }
  
  virtual void
  OnNewSeqNo (const string &prefix, uint32_t seqNo)
  {
    // NS_LOG_DEBUG (prefix << ", " << seqNo);
    (*this)[prefix] = seqNo+1; 
  }
};

shared_ptr<CurrentGlobalKnowledge> GlobalKnowledge;
map< string, shared_ptr <CurrentAppKnowledge> > AppKnowledge;

class HitMap
{
public:
  void
  Periodic (const Time &period)
  {
    for (map< string, shared_ptr <CurrentAppKnowledge> >::iterator node1 = AppKnowledge.begin ();
         node1 != AppKnowledge.end ();
         node1 ++)
      {
        for (map< string, shared_ptr <CurrentAppKnowledge> >::iterator node2 = AppKnowledge.begin ();
             node2 != AppKnowledge.end ();
             node2 ++)
          {
            // NS_LOG_DEBUG ("Checking if "
            //               << node1->first << " (" << (*node1->second)[node2->first]
            //               << ") knows current state of "
            //               << node2->first << " (" << (*GlobalKnowledge)[node2->first]
            //               << ")");
            // Checking if node1 knows current state of node2

            uint32_t check = (*GlobalKnowledge)[node2->first];
            if (check > 0) check--;
            
            if ((*node1->second)[node2->first] >= check)
              {
                m_hitMap[node1->first][node2->first] ++;
              }
            else
              {
                // NS_LOG_DEBUG ("Checking if "
                //               << node1->first << " (" << (*node1->second)[node2->first]
                //               << ") knows current state of "
                //               << node2->first << " (" << (*GlobalKnowledge)[node2->first]
                //               << ")");
              }
          }
      }
    
    
    Simulator::Schedule (period, &HitMap::Periodic, this, period);
  }

  void
  Print (ostream &os, uint32_t run, uint32_t loss, const string &type, double randomLoss)
  {
    os.precision (10);
    os.setf(ios::fixed,ios::floatfield);
    
    os << "#Type\tRun\tRandomLoss\tLoss\tNode1\tNode2\tCount\n";
    for (std::map< std::string, std::map <std::string, uint32_t> >::iterator node1 = m_hitMap.begin ();
         node1 != m_hitMap.end ();
         node1 ++)
      {
        for (std::map <std::string, uint32_t>::iterator node2 = node1->second.begin ();
             node2 != node1->second.end ();
             node2 ++)
          {
            os << type << "\t" << run << "\t" << loss << "\t" << randomLoss << "\t"
               << node1->first.substr (1) << "\t" << node2->first.substr (1) << "\t" << node2->second << "\n";
          }
      }
  }
  
private:
  std::map< std::string, std::map <std::string, uint32_t> > m_hitMap;
};
