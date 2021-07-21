/// \file rhpman.cc
/// \author Keefer Rourke <krourke@uoguelph.ca>
/// \brief Implementation and main function for a simulation that attempts to
///     reproduce the RHPMAN scheme and performance evaluation from Shi and Chen
///     2014.
///
/// Copyright (c) 2020 by Keefer Rourke <krourke@uoguelph.ca>
/// Permission to use, copy, modify, and/or distribute this software for any
/// purpose with or without fee is hereby granted, provided that the above
/// copyright notice and this permission notice appear in all copies.
///
/// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
/// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
/// AND FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
/// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
/// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
/// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
/// PERFORMANCE OF THIS SOFTWARE.

#include <sysexits.h>

#include "ns3/animation-interface.h"
#include "ns3/aodv-helper.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/double.h"
#include "ns3/dsdv-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/netanim-module.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/random-walk-2d-mobility-model.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-standards.h"
#include "ns3/yans-wifi-helper.h"

//#include "logging.h"
#include "ns3/rhpman-helper.h"
#include "nsutil.h"
#include "simulation-area.h"
#include "simulation-params.h"
#include "util.h"

using namespace ns3;
using namespace rhpman;

NS_LOG_COMPONENT_DEFINE("RhpmanExample");

/// \brief Creates and configures the traveller nodes according to simulation parameters.
///
/// \param params Simulation parameters.
/// \param nodes The container to add the new travellers to.
void setupTravellerNodes(const SimulationParameters& params, NodeContainer& nodes) {
  NS_LOG_UNCOND("Setting up traveller node mobility models...");
  NodeContainer travellers;
  travellers.Create(params.travellerNodes);

  MobilityHelper travellerMobilityHelper;

  travellerMobilityHelper.SetPositionAllocator(params.area.getRandomRectanglePositionAllocator());
  travellerMobilityHelper.SetMobilityModel(
      "ns3::RandomWalk2dMobilityModel",
      "Bounds",
      RectangleValue(params.area.asRectangle()),
      "Speed",
      PointerValue(params.travellerVelocity),
      "Distance",
      DoubleValue(params.travellerDirectionChangeDistance),
      "Time",
      TimeValue(params.travellerDirectionChangePeriod),
      "Mode",
      EnumValue(params.travellerWalkMode));

  travellerMobilityHelper.Install(travellers);
  nodes.Add(travellers);
}

/// \brief Creates and configures the partition-bound nodes according to simulation parameters.
///
/// \param params Simulation parameters.
/// \param nodes The container to add the new pb nodes to.
void setupPbNodes(const SimulationParameters& params, NodeContainer& nodes) {
  NS_LOG_UNCOND("Setting up partition-bound node mobility models...");
  auto partitions = params.area.splitIntoGrid(params.rows, params.cols);
  std::vector<NodeContainer> pbnGroups(partitions.size());
  for (size_t i = 0; i < partitions.size(); i++) {
    auto partition = partitions[i];
    NS_LOG_DEBUG(
        "part [" << i << "] "
                 << "from (" << partition.minX() << "," << partition.minY() << ") to ("
                 << partition.maxX() << "," << partition.maxY() << ").");
    auto nodeContainer = pbnGroups[i];
    nodeContainer.Create(params.nodesPerPartition);
    MobilityHelper mobilityHelper;

    auto distance = std::min(partition.deltaX(), partition.deltaY());

    mobilityHelper.SetPositionAllocator(partition.getRandomRectanglePositionAllocator());
    mobilityHelper.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds",
        RectangleValue(partition.asRectangle()),
        "Speed",
        PointerValue(params.pbnVelocity),
        "Distance",
        DoubleValue(distance),
        "Time",
        TimeValue(params.pbnVelocityChangePeriod));
    mobilityHelper.Install(nodeContainer);

    nodes.Add(nodeContainer);
  }
}

int main(int argc, char* argv[]) {
  Time::SetResolution(Time::NS);

  /* Setup and parse the command line options. */
  // Set up the command line options.
  SimulationParameters params;
  bool ok;
  std::tie(params, ok) = SimulationParameters::parse(argc, argv);

  if (!ok) {
    std::cerr << "Error parsing the parameters.\n";
    return -1;
  }

  /* Create nodes, network topology, and start simulation. */
  RngSeedManager::SetSeed(params.seed);
  NodeContainer allAdHocNodes;
  NS_LOG_DEBUG("Simulation running over area: " << params.area);

  // Set up the traveller nodes.
  // Travellers can move across the whole simulation space.
  setupTravellerNodes(params, allAdHocNodes);

  // Set up the partition-bound nodes.
  // Partition-bound nodes can only move within their grid.
  setupPbNodes(params, allAdHocNodes);

  NS_LOG_UNCOND("Setting up wireless devices for all nodes...");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  auto wifiChannel = YansWifiChannelHelper::Default();
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");

  // Shi and Chen refer to a 100m radius of connectivity for each node.
  // They do not assume any propagation loss model, so we use a constant
  // propagation loss model which amounts to having connectivity withing the
  // radius, and having no connectivity outside the radius.
  wifiChannel.AddPropagationLoss(
      "ns3::RangePropagationLossModel",
      "MaxRange",
      DoubleValue(params.wifiRadius));

  wifiPhy.SetChannel(wifiChannel.Create());

  WifiMacHelper wifiMac;
  wifiMac.SetType("ns3::AdhocWifiMac");

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211b);

  NS_LOG_UNCOND("Assigning MAC addresses in ad-hoc mode...");
  auto adhocDevices = wifi.Install(wifiPhy, wifiMac, allAdHocNodes);
  wifiPhy.EnablePcap("rhpman", adhocDevices);

  NS_LOG_UNCOND("Setting up Internet stacks...");
  InternetStackHelper internet;

  if (params.routingProtocol == RoutingType::DSDV) {
    NS_LOG_DEBUG("Using DSDV routing.");
    DsdvHelper dsdv;
    internet.SetRoutingHelper(dsdv);
  } else if (params.routingProtocol == RoutingType::AODV) {
    NS_LOG_DEBUG("Using AODV routing.");
    AodvHelper aodv;
    internet.SetRoutingHelper(aodv);
  }
  internet.Install(allAdHocNodes);
  Ipv4AddressHelper adhocAddresses;
  adhocAddresses.SetBase("10.1.0.0", "255.255.0.0");
  auto adhocInterfaces = adhocAddresses.Assign(adhocDevices);

  // Install the RHPMAN Scheme onto each node.
  RhpmanAppHelper rhpman;
  rhpman.SetAttribute("CarryingThreshold", DoubleValue(params.carryingThreshold));
  rhpman.SetAttribute("ForwardingThreshold", DoubleValue(params.forwardingThreshold));
  rhpman.SetAttribute("NeighborhoodSize", UintegerValue(params.neighborhoodSize));
  rhpman.SetAttribute("ElectionNeighborhoodSize", UintegerValue(params.electionNeighborhoodSize));
  rhpman.SetAttribute("ColocationWeight", DoubleValue(params.wcol));
  rhpman.SetAttribute("DegreeConnectivityWeight", DoubleValue(params.wcdc));
  rhpman.SetAttribute("ProfileUpdateDelay", TimeValue(params.profileUpdateDelay));
  rhpman.SetDataOwners(params.dataOwners);
  rhpman.Install(allAdHocNodes);

  // Run the simulation with support for animations.
  AnimationInterface anim(params.netanimTraceFilePath);
  NS_LOG_UNCOND("Running simulation for " << params.runtime.GetSeconds() << " seconds...");
  Simulator::Stop(params.runtime);
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_UNCOND("Done.");

  return EX_OK;
}
