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
  cmd.AddValue ("topology", "RocketfuelWeights topology prefix", topology);
  cmd.Parse (argc, argv);

  if (topology.empty ())
    {
      cerr << cmd << endl;
      return 1;
    }
  
  RocketfuelWeightsReader reader;
  reader.SetDefaultBandwidth ("100Mbps");
  reader.SetDefaultQueue ("2000");
  
  reader.SetFileType (RocketfuelWeightsReader::POSITIONS);
  reader.SetFileName ("topologies/rocketfuel/" + topology+".positions");
  reader.Read ();
  
  reader.SetFileName ("topologies/rocketfuel/" + topology+".weights");
  reader.SetFileType (RocketfuelWeightsReader::WEIGHTS);    
  reader.Read ();

  reader.SetFileName ("topologies/rocketfuel/" + topology+".latencies");
  reader.SetFileType (RocketfuelWeightsReader::LATENCIES);    
  reader.Read ();

  reader.Commit ();

  reader.SaveTopology ("topologies/" + topology + ".txt");

  return 0;
}
