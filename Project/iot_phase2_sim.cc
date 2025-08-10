// iot_phase2_sim.cc (defensive version)
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("IoTPhase2Sim");

int main (int argc, char *argv[])
{
  uint32_t numDevices = 10;
  bool enableAttack = false;
  bool enableRateLimit = false;
  double simTime = 30.0;

  CommandLine cmd;
  cmd.AddValue ("numDevices", "Number of IoT devices (clients)", numDevices);
  cmd.AddValue ("enableAttack", "Enable DoS attacker (one device)", enableAttack);
  cmd.AddValue ("enableRateLimit", "Enable simple rate-limit on edge->fog link", enableRateLimit);
  cmd.AddValue ("simTime", "Simulation time (s)", simTime);
  cmd.Parse (argc, argv);

  std::cout << "IoTPhase2Sim starting: numDevices=" << numDevices
            << " enableAttack=" << enableAttack << " enableRateLimit=" << enableRateLimit
            << " simTime=" << simTime << std::endl;

  // Nodes
  NodeContainer iotDevices; iotDevices.Create (numDevices);
  NodeContainer edgeNode; edgeNode.Create (1);
  NodeContainer fogNode;  fogNode.Create (1);
  NodeContainer cloudNode; cloudNode.Create (1);

  // Internet stack
  InternetStackHelper internet;
  internet.InstallAll ();

  // WiFi: create channel/phy/helper properly
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211g);
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("iot-net");

  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));
  NetDeviceContainer staDevices = wifi.Install (phy, mac, iotDevices);

  mac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (phy, mac, edgeNode);

  // Mobility
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (iotDevices);
  mobility.Install (edgeNode);
  mobility.Install (fogNode);
  mobility.Install (cloudNode);

  // Edge <-> Fog P2P
  PointToPointHelper p2pEdgeFog;
  p2pEdgeFog.SetDeviceAttribute ("DataRate", StringValue ("50Mbps"));
  p2pEdgeFog.SetChannelAttribute ("Delay", StringValue ("5ms"));
  NetDeviceContainer edgeFog = p2pEdgeFog.Install (edgeNode.Get (0), fogNode.Get (0));

  // Fog <-> Cloud P2P
  PointToPointHelper p2pFogCloud;
  p2pFogCloud.SetDeviceAttribute ("DataRate", StringValue ("200Mbps"));
  p2pFogCloud.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer fogCloud = p2pFogCloud.Install (fogNode.Get (0), cloudNode.Get (0));

  // Rate limit (attach to NetDeviceContainer when requested)
  if (enableRateLimit)
    {
      TrafficControlHelper tch;
      tch.SetRootQueueDisc ("ns3::CoDelQueueDisc");
      // Install on the edge device (the device facing the fog node)
      // edgeFog has two devices: index 0 is edge side, index 1 is fog side
      if (edgeFog.GetN () > 0)
        {
          NetDeviceContainer tmp;
          tmp.Add(edgeFog.Get (0));
          tch.Install (tmp);
          std::cout << "RateLimit installed on edge device" << std::endl;
        }
      else
        {
          std::cout << "Warning: edgeFog has no devices to install rate-limit" << std::endl;
        }
    }

  // IP addressing — do each subnet separately and capture the returned containers
  Ipv4AddressHelper address;

  // WiFi subnet (STA + AP)
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces = address.Assign (staDevices);
  Ipv4InterfaceContainer apIf = address.Assign (apDevice);

  std::cout << "WiFi: sta count=" << wifiInterfaces.GetN()
            << " ap count=" << apIf.GetN() << std::endl;

  // Edge-Fog subnet
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer edgeFogIf = address.Assign (edgeFog);
  std::cout << "EdgeFog: dev count=" << edgeFogIf.GetN() << std::endl;

  // Fog-Cloud subnet
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer fogCloudIf = address.Assign (fogCloud);
  std::cout << "FogCloud: dev count=" << fogCloudIf.GetN() << std::endl;

  // Sanity checks before using GetAddress(1)
  if (fogCloudIf.GetN () < 2)
    {
      std::cerr << "ERROR: fogCloudIf.GetN()=" << fogCloudIf.GetN()
                << " <-- expected at least 2 (fog, cloud). Aborting." << std::endl;
      return 1;
    }
  Ipv4Address cloudAddr = fogCloudIf.GetAddress (1); // cloud address
  std::cout << "Cloud address (fog-cloud side) = " << cloudAddr << std::endl;

  // UDP server on cloud
  uint16_t port = 4000;
  UdpServerHelper server (port);
  ApplicationContainer serverApp = server.Install (cloudNode.Get (0));
  serverApp.Start (Seconds (1.0));
  serverApp.Stop (Seconds (simTime));

  // Clients (IoT devices)
  for (uint32_t i = 0; i < iotDevices.GetN (); ++i)
    {
      UdpClientHelper client (cloudAddr, port);
      client.SetAttribute ("MaxPackets", UintegerValue (1));
      client.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
      client.SetAttribute ("PacketSize", UintegerValue (92));
      ApplicationContainer app = client.Install (iotDevices.Get (i));
      app.Start (Seconds (2.0 + i * 0.2));
      app.Stop (Seconds (simTime));
    }

  // Attacker
  if (enableAttack && numDevices > 0)
    {
      Ptr<Node> attacker = iotDevices.Get (0);
      OnOffHelper attackerApp ("ns3::UdpSocketFactory", InetSocketAddress (cloudAddr, port));
      attackerApp.SetAttribute ("OnTime",    StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
      attackerApp.SetAttribute ("OffTime",   StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      attackerApp.SetAttribute ("DataRate",  StringValue ("20Mbps"));
      attackerApp.SetAttribute ("PacketSize", UintegerValue (512));
      ApplicationContainer atk = attackerApp.Install (attacker);
      atk.Start (Seconds (5.0));
      atk.Stop (Seconds (simTime));
      std::cout << "Attacker configured to target " << cloudAddr << std::endl;
    }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Flow monitor
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  // PCAPs — only call pcap on devices that exist
  if (apDevice.GetN () > 0)
    {
      phy.EnablePcap ("iot_wifi_ap", apDevice.Get (0));
    }
  if (edgeFog.GetN () > 0)
    {
      p2pEdgeFog.EnablePcapAll ("iot_edgefog");
    }

  Simulator::Stop (Seconds (simTime + 1.0));
  Simulator::Run ();

  monitor->SerializeToXmlFile ("iot_flowmon.xml", true, true);

  Simulator::Destroy ();
  std::cout << "Simulation complete, flowmon written to iot_flowmon.xml" << std::endl;
  return 0;
}