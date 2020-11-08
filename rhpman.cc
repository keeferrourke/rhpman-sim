/// @file rhpman.cc
/// @author Keefer Rourke (krourke@uoguelph.ca)
/// @brief Implementation and main function for a simulation that attempts to
///     reproduce the RHPMAN scheme and performance evaluation from Shi and Chen
///     2014.
///
/// Copyright (c) 2020 by Keefer Rourke <krourke@uoguelph.ca>
/// Permission to use, copy, modify, and/or distribute this software for any
/// purpose with or without fee is hereby granted, provided that the above
/// copyright notice and this permission notice appear in all copies.
///
/// THE SOFTWARE IS PROVIDED “AS IS” AND ISC DISCLAIMS ALL WARRANTIES WITH
/// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
/// AND FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
/// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
/// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
/// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
/// PERFORMANCE OF THIS SOFTWARE.

#include <sysexits.h>

#include "ns3/animation-interface.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/double.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/netanim-module.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/random-walk-2d-mobility-model.h"

#include "nsutil.h"
#include "rhpman.h"
#include "simulation-area.h"
#include "util.h"

using namespace ns3;
using namespace rhpman;

NS_LOG_COMPONENT_DEFINE("RHPMAN");

int main(int argc, char* argv[]) {
  // Simulation run time.
  double optRuntime = 5.0_minutes;

  // Node parameters.
  uint32_t optTotalNodes = 160;
  uint32_t optNodesPerPartition = 8;

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
  double optPbnVelocityMin = 1.0_meters;
  double optPbnVelocityMax = 1.0_meters;
  double optPbnVelocityChangeAfter = 100.0_seconds;

  // Animation parameters.
  std::string animationTraceFilename = "rhpman.xml";

  CommandLine cmd;
  cmd.AddValue("run-time", "Simulation run time in seconds", optRuntime);
  cmd.AddValue("total-nodes", "Total number of nodes in the simulation", optTotalNodes);
  cmd.AddValue("partition-nodes", "The number of nodes placed per partition", optNodesPerPartition);
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
  cmd.AddValue("animation-xml", "Output file path for NetAnim trace file", animationTraceFilename);
  cmd.Parse(argc, argv);

  auto result = getWalkMode(optTravellerWalkMode);
  auto traveller_walk_mode = result.first;
  if (!result.second) {
    NS_LOG_ERROR("Unrecognized walk mode '" + optTravellerWalkMode + "'.");
    return EX_USAGE;
  }
  if (!optTravellerWalkDistance) {
    optTravellerWalkDistance = std::min(optAreaWidth, optAreaLength);
  }

  // Set up the travellers.
  // Travellers can move across the whole simulation space.
  int32_t num_travellers = optTotalNodes - (optNodesPerPartition * (optRows * optCols));
  NodeContainer travellers;
  travellers.Create(num_travellers);

  SimulationArea area(
      std::pair<double, double>(0.0, 0.0),
      std::pair<double, double>(optAreaWidth, optAreaLength));

  MobilityHelper travellerMobilityHelper;
  Ptr<ConstantRandomVariable> travellerVelocityGenerator = CreateObject<ConstantRandomVariable>();
  travellerVelocityGenerator->SetAttribute("Constant", DoubleValue(optTravellerVelocity));

  travellerMobilityHelper.SetPositionAllocator(area.getRandomRectanglePositionAllocator());
  travellerMobilityHelper.SetMobilityModel(
      "ns3::RandomWalk2dMobilityModel",
      "Bounds",
      RectangleValue(area.asRectangle()),
      "Speed",
      PointerValue(travellerVelocityGenerator),
      "Distance",
      DoubleValue(optTravellerWalkDistance),
      "Time",
      TimeValue(Seconds(optTravellerWalkTime)),
      "Mode",
      EnumValue(traveller_walk_mode));

  travellerMobilityHelper.Install(travellers);

  // Set up the partition-bound nodes.
  // Partition-bound nodes can only move within their grid.
  auto partitions = area.splitIntoGrid(optRows, optCols);

  std::vector<NodeContainer> partitionBoundGroups(partitions.size());
  for (size_t i = 0; i < partitions.size(); i++) {
    auto partition = partitions[i];
    auto nodes = partitionBoundGroups[i];
    nodes.Create(optNodesPerPartition);
    MobilityHelper mobilityHelper;
    Ptr<UniformRandomVariable> velocityGenerator = CreateObject<UniformRandomVariable>();
    velocityGenerator->SetAttribute("Min", DoubleValue(optPbnVelocityMin));
    velocityGenerator->SetAttribute("Max", DoubleValue(optPbnVelocityMax));

    auto distance = std::min(partition.deltaX(), partition.deltaY());

    mobilityHelper.SetPositionAllocator(partition.getRandomRectanglePositionAllocator());
    mobilityHelper.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds",
        RectangleValue(partition.asRectangle()),
        "Speed",
        PointerValue(velocityGenerator),
        "Distance",
        DoubleValue(distance),
        "Time",
        TimeValue(Seconds(optPbnVelocityChangeAfter)));
    mobilityHelper.Install(nodes);
  }

  // Run the simulation with support for animations.
  AnimationInterface anim(animationTraceFilename);
  NS_LOG_UNCOND("Running simulation for " << optRuntime << " seconds...");
  Simulator::Stop(Seconds(optRuntime));
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_UNCOND("Done.");

  return EX_OK;
}
