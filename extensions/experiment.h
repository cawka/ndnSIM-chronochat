/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */


#include <boost/lexical_cast.hpp>


  // void
  // RandomizeClients ()
  // {
  //   NS_ASSERT (numClients <= reader->GetNodes ().GetN ());

  //   std::set<uint32_t> selected;
  //   UniformVariable rand (0, reader->GetNodes ().GetN ());

  //   while (selected.size () < numClients)
  //     {
  //       uint32_t nodeId = rand.GetValue ();
  //       if (selected.count (nodeId) > 0)
  //         continue;

  //       selected.insert (nodeId);
  //       clientIds.push_back (nodeId);
  //     }
  // }


  // void SetUpLosses ()
  // {
  //   if (randomLoss == 0.0) return;
    
  //   ObjectFactory lossFactory ("ns3::RandomPointToPointLossModel");
  //   lossFactory.Set ("Probability", DoubleValue (randomLoss));
    
  //   for (std::list<TopologyReader::Link>::const_iterator linkI = reader->GetLinks ().begin ();
  //        linkI != reader->GetLinks ().end ();
  //        linkI ++)
  //     {
  //       linkI->GetFromNetDevice ()->GetChannel ()->SetAttribute ("LossModel", PointerValue (lossFactory.Create ()));
  //     }
  // }
  
  
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
