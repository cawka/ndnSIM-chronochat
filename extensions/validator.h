/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

class Validator
{
public:
  Validator (Ptr<Application> app)
  {
    m_id = Names::FindName (app);
    app->TraceConnectWithoutContext ("SeqNoTrace", MakeCallback (&Validator::OnNewSeqNo, this)); 
  }
  
  ~Validator ()
  {
    uint32_t total = 0;
    cout << "=== verification === for node " << m_id;// << endl;
    // cout << endl;
    for (std::map< std::string, std::set<uint32_t> >::iterator i = m_test.begin ();
         i != m_test.end ();
         i++ )
      {
        // cout << i->first << ": ";
        for (std::set<uint32_t>::iterator x = i->second.begin ();
             x != i->second.end ();
             x++)
          {
            // cout << *x << " ";
            total ++;
          }
        // cout << endl;
      }
    // cout << "===== end (" << total << ") =====" << endl;
    cout << " total: " << total << endl;
  }

  void
  OnNewSeqNo (const string &prefix, uint32_t seqNo)
  {
    m_test[prefix].insert (seqNo);
  }
  
private:
  std::map< std::string, std::set<uint32_t> > m_test;
  string m_id;
};
