/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018-2020 The IoD_Sim Authors.
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
 */

#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/lte-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/config-store-module.h>
#include <ns3/applications-module.h>
#include <ns3/drone-client.h>
#include <ns3/drone-server.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Scenario");

int main (int argc, char *argv[])
{
  LogComponentEnable ("Scenario", LOG_LEVEL_ALL);

  CommandLine cmd;
  std::string configFile;
  cmd.AddValue ("config", "config file for IoD_Sim", configFile);
  cmd.Parse (argc, argv);
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  cmd.Parse (argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  NodeContainer enbNodes, ueNodes, hostNodes;
  enbNodes.Create (1);
  ueNodes.Create (2);
  hostNodes.Create (1);

  MobilityHelper staticNodeMobility;
  staticNodeMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  staticNodeMobility.Install (enbNodes);
  staticNodeMobility.Install (ueNodes);

  Ptr<EpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  Ptr<Node> host = hostNodes.Get (0);

  InternetStackHelper internet;
  internet.Install (ueNodes);
  internet.Install (hostNodes);

  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  NetDeviceContainer p2pDevs = p2ph.Install (pgw, host);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("127.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer hostsIpInterfaces = ipv4.Assign (p2pDevs);
  //Ipv4Address hostIp = hostsIpInterfaces.GetAddress (1); // 0 is localhost

  NetDeviceContainer enbDevices = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueDevices = lteHelper->InstallUeDevice (ueNodes);
  lteHelper->Attach (ueDevices, enbDevices.Get (0));

  ipv4.SetBase ("127.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer lteDevs = epcHelper->AssignUeIpv4Address (ueDevices);

  // CREATE ROUTING

  EpsBearer dataRadioBearer (EpsBearer::GBR_CONV_VIDEO);
  lteHelper->ActivateDataRadioBearer (ueDevices, dataRadioBearer);
/*
  Ptr<DroneClient> clientApps = CreateObject<DroneClient> ();
  clientApps.Start (Seconds (1.0));
  clientApps.Stop (Seconds (9.0))
  clientApps.install (ueNodes.Get (0));

  ApplicationContainer serverApps = CreateObject<DroneServer> ();
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (10.0));
  serverApps.install(ueNodes.Get(1));

  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
*/
  return 0;
}