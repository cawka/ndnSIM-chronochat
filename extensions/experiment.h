/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

// hard-code topology files
const string weights   ("./evaluation/topologies/sprint-pops.weights");
const string latencies ("./evaluation/topologies/sprint-pops.latencies");
const string positions ("./evaluation/topologies/sprint-pops.positions");

const string simpleTopo2 ("./evaluation/topologies/simple-topology-2node.txt");
const string simpleTopo4 ("./evaluation/topologies/simple-topology-4node.txt");
// const string simpleTopo ("./evaluation/topologies/simple-topology.txt");

#include <ns3/mobility-module.h>
#include <boost/lexical_cast.hpp>

struct Experiment
{
  AnnotatedTopologyReader *reader;
  vector< uint32_t > clientIds;
  uint32_t run;
  uint32_t loss;
  double   randomLoss;
  uint32_t numClients;
  string   m_type; // ipv4 or ccnx
  bool     m_ccnxData;

  ofstream m_trace;
  // ofstream m_trace2;
  string m_inputFile;

  typedef map<uint32_t, tuple <uint64_t, uint64_t, uint64_t, uint64_t> > PacketsInChannel;
  PacketsInChannel m_packetsInChannels;
  PacketsInChannel m_dropsInChannels;
  
  Experiment (uint32_t run, uint32_t numClients, uint32_t loss, double randomLoss)
  {
    this->run = run;
    this->numClients = numClients;
    this->loss = loss;
    this->randomLoss = randomLoss;

    if (numClients == 2)
      {
        reader = new AnnotatedTopologyReader ();
        
        reader->SetFileName (simpleTopo2);
        reader->Read ();
      }
    else if (numClients == 4)
      {
        reader = new AnnotatedTopologyReader ();
        
        reader->SetFileName (simpleTopo4);
        reader->Read ();
      }
    else if (numClients == 52 || numClients == 26)
      {
        RocketfuelWeightsReader *reader = new RocketfuelWeightsReader ();
        this->reader = reader;
        
        reader->SetFileName (positions);
        reader->SetFileType (RocketfuelWeightsReader::POSITIONS);
        reader->Read ();
 
        reader->SetFileName (weights);
        reader->SetFileType (RocketfuelWeightsReader::WEIGHTS);    
        reader->Read ();

        reader->SetFileName (latencies);
        reader->SetFileType (RocketfuelWeightsReader::LATENCIES);    
        reader->Read ();
    
        reader->Commit ();    
      }
    else
      {
        cerr << "Scenario is not supported" << endl;
        exit (-1);
      }
  }

  ~Experiment ()
  {    
    m_trace.close ();
    PrintTraces (Seconds(10000000));

    delete reader;
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
  
  void
  RandomizeClients ()
  {
    NS_ASSERT (numClients <= reader->GetNodes ().GetN ());

    std::set<uint32_t> selected;
    UniformVariable rand (0, reader->GetNodes ().GetN ());

    while (selected.size () < numClients)
      {
        uint32_t nodeId = rand.GetValue ();
        if (selected.count (nodeId) > 0)
          continue;

        selected.insert (nodeId);
        clientIds.push_back (nodeId);
      }
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

  void SetUpLosses ()
  {
    if (randomLoss == 0.0) return;
    
    ObjectFactory lossFactory ("ns3::RandomPointToPointLossModel");
    lossFactory.Set ("Probability", DoubleValue (randomLoss));
    
    for (std::list<TopologyReader::Link>::const_iterator linkI = reader->GetLinks ().begin ();
         linkI != reader->GetLinks ().end ();
         linkI ++)
      {
        linkI->GetFromNetDevice ()->GetChannel ()->SetAttribute ("LossModel", PointerValue (lossFactory.Create ()));
      }
  }
  
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

  
  ApplicationContainer
  CcnxApps ()
  {
    NodeContainer clients; // random list of clients
    for (vector< uint32_t >::iterator i = clientIds.begin ();
         i != clientIds.end ();
         i++)
      {
        // clients.Add (reader->GetNodes ().Get (*i));
        clients.Add (Names::Find<Node> (boost::lexical_cast<string> (*i)));
      }
    
    // Install CCNx stack on all nodes
    NS_LOG_INFO ("Installing CCNx stack on all nodes");
    CcnxStackHelper ccnxHelper;
    ccnxHelper.SetDefaultRoutes (true);
    ccnxHelper.Install (reader->GetNodes ());

    reader->ApplyOspfMetric ();

    CcnxGlobalRoutingHelper ccnxGlobalRoutingHelper;
    ccnxGlobalRoutingHelper.Install (reader->GetNodes ());

    // broadcast prefix
    ccnxGlobalRoutingHelper.AddOrigins ("/bcast", clients);

    ApplicationContainer apps;
    for (vector< uint32_t >::iterator i = clientIds.begin ();
         i != clientIds.end ();
         i++)
      {
        Ptr<Node> node = Names::Find<Node> (boost::lexical_cast<string> (*i));
        // Ptr<Node> node = reader->GetNodes ().Get (*i);

        ostringstream nodePrefix;
        nodePrefix << "/" << boost::lexical_cast<string> (*i);
        ccnxGlobalRoutingHelper.AddOrigin (nodePrefix.str (), node);

        Ptr<SyncMuc> syncMuc = CreateObject<SyncMuc> ("/bcast", nodePrefix.str (), boost::lexical_cast<string> (*i), m_inputFile, m_ccnxData);
        syncMuc->SetStartTime (Seconds (1.0)); // clients should start at the same time !
        node->AddApplication (syncMuc);

        Names::Add (":"+boost::lexical_cast<string> (*i), syncMuc);

        apps.Add (syncMuc);
      }

    SetUpChannelTrace ();
    SetUpDropTrace ();
    // SetUpLosses ();
    
    // Calculate and install FIBs
    // ccnxGlobalRoutingHelper.CalculateRoutes ();

    return apps;
  }

  ApplicationContainer
  Ipv4Apps ()
  {
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
    
    NodeContainer clients; // random list of clients
    for (vector< uint32_t >::iterator i = clientIds.begin ();
         i != clientIds.end ();
         i++)
      {
        // clients.Add (reader->GetNodes ().Get (*i));
        clients.Add (Names::Find<Node> (boost::lexical_cast<string> (*i)));
      }

    // Install CCNx stack on all nodes
    NS_LOG_INFO ("Installing Ipv4 stack on all nodes");
    InternetStackHelper ipv4Helper;
    ipv4Helper.Install (reader->GetNodes ());
    
    reader->AssignIpv4Addresses (Ipv4Address ("10.0.0.0"));
    reader->ApplyOspfMetric ();

    NodeContainer serverNodes;
    serverNodes.Create (1);
    
    PointToPointHelper p2pH;
    NS_ASSERT (clients.GetN () > 0);
    NetDeviceContainer serverLink = p2pH.Install (serverNodes.Get (0), clients.Get (0));
    ipv4Helper.Install (serverNodes.Get (0));
    
    MobilityHelper m;
    m.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    m.Install (serverNodes.Get (0));
    Ipv4AddressHelper addressH (Ipv4Address ("5.5.5.0"), Ipv4Mask ("/24"));
    addressH.Assign (serverLink);
    
    Ipv4Address serverIp ("5.5.5.1");
      
    Ptr<Application> server = CreateObject<MucServer> ();
    server->SetStartTime (Seconds (0.0));
    serverNodes.Get (0)->AddApplication (server);


    Ipv4Address base ("1.0.0.0");
    Ipv4AddressHelper address (base, Ipv4Mask ("/24"));
        
    ApplicationContainer apps;
    for (vector< uint32_t >::iterator i = clientIds.begin ();
         i != clientIds.end ();
         i++)
      {
        Ptr<Node> node = Names::Find<Node> (boost::lexical_cast<string> (*i));
        // cout << node << endl;
        // Ptr<Node> node = reader->GetNodes ().Get (*i);

        Ptr<Node> realNode = CreateObject<Node> ();
        m.Install (realNode);
        NetDeviceContainer nd = p2pH.Install (realNode, node);
        ipv4Helper.Install (realNode);
        address.Assign (nd);
        
        Ptr<MucClient> client = CreateObject<MucClient> (serverIp, boost::lexical_cast<string> (*i), m_inputFile);
        client->SetStartTime (Seconds (1.0)); // clients should start at the same time !
        realNode->AddApplication (client);
        Names::Add (":"+boost::lexical_cast<string> (*i), client);

        apps.Add (client);

        base = Ipv4Address (base.Get () + 256);
        address.SetBase (base, Ipv4Mask ("/24"));        
      }
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    SetUpChannelTrace ();
    SetUpDropTrace ();
    // SetUpLosses ();
    return apps;
  }
};
