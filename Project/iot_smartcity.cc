#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("IoTSmartCity");

int main (int argc, char *argv[])
{
  uint32_t nIotDevices = 100;
  uint32_t nEdgeNodes = 5;
  uint32_t nFogNodes = 2;
  uint32_t port = 9000;

  CommandLine cmd;
  cmd.Parse (argc, argv);

  NodeContainer iotDevices;
  iotDevices.Create(nIotDevices);

  NodeContainer edgeNodes;
  edgeNodes.Create(nEdgeNodes);

  NodeContainer fogNodes;
  fogNodes.Create(nFogNodes);

  // Install network stack
  InternetStackHelper stack;
  stack.Install(iotDevices);
  stack.Install(edgeNodes);
  stack.Install(fogNodes);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("2ms"));

  Ipv4AddressHelper address;
  std::vector<Ipv4InterfaceContainer> iotToEdgeInterfaces;

  // Connect IoT devices to Edge nodes (20 devices per edge node)
  for (uint32_t i = 0; i < nIotDevices; ++i) {
    NodeContainer link(iotDevices.Get(i), edgeNodes.Get(i % nEdgeNodes));
    NetDeviceContainer devices = p2p.Install(link);

    std::ostringstream subnet;
    subnet << "10.1." << i+1 << ".0";
    address.SetBase(subnet.str().c_str(), "255.255.255.0");
    Ipv4InterfaceContainer iface = address.Assign(devices);
    iotToEdgeInterfaces.push_back(iface);
  }

  // Connect each Edge to all Fog nodes
  for (uint32_t i = 0; i < nEdgeNodes; ++i) {
    for (uint32_t j = 0; j < nFogNodes; ++j) {
      NodeContainer link(edgeNodes.Get(i), fogNodes.Get(j));
      NetDeviceContainer devices = p2p.Install(link);

      std::ostringstream subnet;
      subnet << "192.168." << i << j << ".0";
      address.SetBase(subnet.str().c_str(), "255.255.255.0");
      address.Assign(devices);
    }
  }

  // Assign IPs and setup routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // Set up UDP servers on fog nodes
  for (uint32_t i = 0; i < nFogNodes; ++i) {
    UdpEchoServerHelper server(port + i);
    ApplicationContainer serverApp = server.Install(fogNodes.Get(i));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(Seconds(20.0));
  }

  // Set up UDP clients on IoT devices targeting their assigned fog node
  for (uint32_t i = 0; i < nIotDevices; ++i) {
    Ptr<Node> fogTarget = fogNodes.Get(i % nFogNodes);
    Ptr<Ipv4> ipv4 = fogTarget->GetObject<Ipv4>();
    Ipv4Address fogAddr = ipv4->GetAddress(1, 0).GetLocal();

    UdpEchoClientHelper client(fogAddr, port + (i % nFogNodes));
    client.SetAttribute("MaxPackets", UintegerValue(3));
    client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    client.SetAttribute("PacketSize", UintegerValue(64));

    ApplicationContainer clientApp = client.Install(iotDevices.Get(i));
    clientApp.Start(Seconds(2.0 + (i % 5)));  // small stagger
    clientApp.Stop(Seconds(20.0));
  }

  Simulator::Stop(Seconds(21.0));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}