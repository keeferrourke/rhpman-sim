/// \file simulation-params.cc
/// \author Keefer Rourke <krourke@uoguelph.ca>
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

#include <inttypes.h>
#include <cmath>
#include <utility>

#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/random-walk-2d-mobility-model.h"

#include "simulation-params.h"
#include "util.h"

namespace rhpman {
// static
std::pair<SimulationParameters, bool> SimulationParameters::parse(int argc, char* argv[]) {
  /* Default simulation values. */
  // Simulation run time.
  double optRuntime = 2.0_minutes;

  // Simulation seed.
  uint32_t optSeed = 1;

  // Node parameters.
  uint32_t optTotalNodes = 160;
  uint32_t optNodesPerPartition = 8;
  double optPercentageDataOwners = 10;

  // Simulation area parameters.
  double optAreaWidth = 1000.0_meters;
  double optAreaLength = 1000.0_meters;

  // Shi and Chen describe a 4x4 grid of 16 partitions in the simulation,
  // but we allow this to be configurable.
  uint32_t optRows = 4;
  uint32_t optCols = 4;

  // Traveller mobility model parameters.
  double optTravellerVelocity = 20.0_mps;

  // Traveller random 2d walk mobility model parameters.
  // Note: Shi and Chen do not specify any parameters of their random walk
  //   mobility models.
  double optTravellerWalkDistance = 0.0_meters;
  double optTravellerWalkTime = 30.0_seconds;
  std::string optTravellerWalkMode = "distance";

  // Partition-bound node mobility model parameters.
  double optPbnVelocityMin = 1.0_mps;
  double optPbnVelocityMax = 1.0_mps;
  double optPbnVelocityChangeAfter = 100.0_seconds;

  // Link and network parameters.
  std::string optRoutingProtocol = "dsdv";
  double optWifiRadius = 100.0_meters;

  // RHPMAN app parameters.
  double optCarryingThreshold = 0.6;
  double optForwardingThreshold = 0.4;
  uint8_t optNeighborhoodSize = 2;
  uint8_t optElectionNeighborhoodSize = 4;
  double optWcdc = 0.5;
  double optWcol = 0.5;
  double optProfileUpdateDelay = 6.0_seconds;

  // Animation parameters.
  std::string animationTraceFilePath = "rhpman.xml";

  /* Setup commandline option for each simulation parameter. */
  CommandLine cmd;
  cmd.AddValue("run-time", "Simulation run time in seconds", optRuntime);
  cmd.AddValue("seed", "Simulation seed", optSeed);
  cmd.AddValue("total-nodes", "Total number of nodes in the simulation", optTotalNodes);
  cmd.AddValue(
      "percent-data-owners",
      "Percent of nodes who have original data to deciminate",
      optPercentageDataOwners);
  cmd.AddValue("partition-nodes", "The number of nodes placed per partition", optNodesPerPartition);
  cmd.AddValue(
      "carrying-threshold",
      "The delivery probability threshold for a node to cache data",
      optCarryingThreshold);
  cmd.AddValue(
      "forwarding-threshold",
      "The delivery probability threshold for a node to forward data",
      optForwardingThreshold);
  cmd.AddValue(
      "hops",
      "The number of hops to consider in the neighborhood of a node",
      optNeighborhoodSize);
  cmd.AddValue(
      "replication-hops",
      "The number of hops to consider in the neighborhood of a node for replicating node elections",
      optElectionNeighborhoodSize);
  cmd.AddValue(
      "wcdc",
      "Weight of degree connectivity in delivery probability calculations",
      optWcdc);
  cmd.AddValue("wcol", "Weight of colocation in delivery probability calculations", optWcol);
  cmd.AddValue(
      "profile-update-delay",
      "Number of seconds between profile updates",
      optProfileUpdateDelay);
  cmd.AddValue("area-width", "Width of the simulation area in meters", optAreaWidth);
  cmd.AddValue("area-length", "Length of the simulation area in meters", optAreaLength);
  cmd.AddValue("grid-rows", "Number of rows in the partition grid", optRows);
  cmd.AddValue("grid-cols", "Number of columns in the partition grid", optCols);
  cmd.AddValue("traveller-velocity", "Velocity of traveller nodes in m/s", optTravellerVelocity);
  cmd.AddValue(
      "traveller-walk-dist",
      "The distance in meters that traveller walks before changing "
      "directions",
      optTravellerWalkDistance);
  cmd.AddValue(
      "traveller-walk-time",
      "The time in seconds that should pass before a traveller changes "
      "directions",
      optTravellerWalkTime);
  cmd.AddValue(
      "traveller-walk-mode",
      "Should a traveller change direction after distance walked or time "
      "passed; options are 'distance' or 'time' ",
      optTravellerWalkMode);
  cmd.AddValue(
      "pbn-velocity-min",
      "Minimum velocity of partition-bound-nodes in m/s",
      optPbnVelocityMin);
  cmd.AddValue(
      "pbn-velocity-max",
      "Maximum velocity of partition-bound-nodes in m/s",
      optPbnVelocityMax);
  cmd.AddValue(
      "pbn-velocity-change-after",
      "Number of seconds after which each partition-bound node should change velocity",
      optPbnVelocityChangeAfter);
  cmd.AddValue("routing", "One of either 'DSDV' or 'AODV'", optRoutingProtocol);
  cmd.AddValue("wifi-radius", "The radius of connectivity for each node in meters", optWifiRadius);
  cmd.AddValue("animation-xml", "Output file path for NetAnim trace file", animationTraceFilePath);
  cmd.Parse(argc, argv);

  /* Parse the parameters. */

  bool ok = true;
  SimulationParameters result;
  if (optCarryingThreshold < 0 || optCarryingThreshold > 1) {
    std::cerr << "Carrying threshold (" << optCarryingThreshold << ") is not a probability"
              << std::endl;
    return std::pair<SimulationParameters, bool>(result, false);
  }
  if (optForwardingThreshold < 0 || optForwardingThreshold > 1) {
    std::cerr << "Forwarding threshold (" << optForwardingThreshold << ") is not a probability"
              << std::endl;
    return std::pair<SimulationParameters, bool>(result, false);
  }
  if (optWcol < 0 || optWcol > 1) {
    std::cerr << "Colocation weight (" << optWcol << ") is not a probability" << std::endl;
    return std::pair<SimulationParameters, bool>(result, false);
  }
  if (optWcdc < 0 || optWcdc > 1) {
    std::cerr << "Degree connectivity weight (" << optWcdc << ") is not a probability" << std::endl;
    return std::pair<SimulationParameters, bool>(result, false);
  }

  RandomWalk2dMobilityModel::Mode travellerWalkMode;
  std::tie(travellerWalkMode, ok) = getWalkMode(optTravellerWalkMode);
  if (!ok) {
    std::cerr << "Unrecognized walk mode '" + optTravellerWalkMode + "'." << std::endl;
  }
  if (!optTravellerWalkDistance) {
    optTravellerWalkDistance = std::min(optAreaWidth, optAreaLength);
  }

  RoutingType routingType = getRoutingType(optRoutingProtocol);
  if (routingType == RoutingType::UNKNOWN) {
    std::cerr << "Unrecognized routing type '" + optRoutingProtocol + "'." << std::endl;
    return std::pair<SimulationParameters, bool>(result, false);
  }

  if (optNodesPerPartition * optCols * optRows > optTotalNodes) {
    std::cerr << "Too few nodes (" << optTotalNodes << ") to populate all " << optCols * optRows
              << " partitions with " << optNodesPerPartition << " nodes." << std::endl;
    return std::pair<SimulationParameters, bool>(result, false);
  }

  if (optPercentageDataOwners < 0.0 || optPercentageDataOwners > 100.0) {
    std::cerr << "percentage of data owners (" << optPercentageDataOwners << "%) is out of range"
              << std::endl;
    return std::pair<SimulationParameters, bool>(result, false);
  }

  Ptr<ConstantRandomVariable> travellerVelocityGenerator = CreateObject<ConstantRandomVariable>();
  travellerVelocityGenerator->SetAttribute("Constant", DoubleValue(optTravellerVelocity));

  Ptr<UniformRandomVariable> pbnVelocityGenerator = CreateObject<UniformRandomVariable>();
  pbnVelocityGenerator->SetAttribute("Min", DoubleValue(optPbnVelocityMin));
  pbnVelocityGenerator->SetAttribute("Max", DoubleValue(optPbnVelocityMax));

  result.seed = optSeed;
  result.runtime = Seconds(optRuntime);
  result.area = SimulationArea(
      std::pair<double, double>(0.0, 0.0),
      std::pair<double, double>(optAreaWidth, optAreaLength));
  result.rows = optRows;
  result.cols = optCols;

  result.totalNodes = optTotalNodes;
  result.dataOwners = std::round(optTotalNodes * (optPercentageDataOwners / 100.0));

  result.travellerNodes = optTotalNodes - (optNodesPerPartition * (optRows * optCols));
  result.travellerVelocity = travellerVelocityGenerator;
  result.travellerDirectionChangePeriod = Seconds(optTravellerWalkTime);
  result.travellerDirectionChangeDistance = optTravellerWalkDistance;
  result.travellerWalkMode = travellerWalkMode;

  result.nodesPerPartition = optNodesPerPartition;
  result.pbnVelocity = pbnVelocityGenerator;
  result.pbnVelocityChangePeriod = Seconds(optPbnVelocityChangeAfter);

  result.routingProtocol = routingType;
  result.wifiRadius = optWifiRadius;
  result.carryingThreshold = optCarryingThreshold;
  result.forwardingThreshold = optForwardingThreshold;
  result.neighborhoodSize = optNeighborhoodSize;
  result.electionNeighborhoodSize = optElectionNeighborhoodSize;
  result.wcdc = optWcdc;
  result.wcol = optWcol;
  result.profileUpdateDelay = Seconds(optProfileUpdateDelay);

  result.netanimTraceFilePath = animationTraceFilePath;

  return std::pair<SimulationParameters, bool>(result, ok);
}
}  // namespace rhpman
