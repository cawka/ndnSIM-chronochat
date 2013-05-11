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

#include <ns3/core-module.h>
#include <ns3/ndnSIM-module.h>

using namespace ns3;
using namespace std;
using namespace boost;
using namespace ns3;

int 
main (int argc, char *argv[])
{
  string topology;
  
  CommandLine cmd;
  cmd.AddValue ("topology", "Topology prefix", topology);
  cmd.Parse (argc, argv);

  if (topology.empty ())
    {
      cerr << cmd << endl;
      return 1;
    }
  
  AnnotatedTopologyReader reader;
  reader.SetFileName ("topologies/" + topology + ".txt");
  reader.Read ();

  reader.SaveGraphviz (topology + ".dot");

  Simulator::Stop (Seconds (1.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
