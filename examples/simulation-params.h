/// \file simulation-params.h
/// \author Keefer Rourke <mail@krourke.org>
/// \brief All simulation parameters are declared in this file.
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

#ifndef __simulation_params_h
#define __simulation_params_h

#include <inttypes.h>
#include <utility>

#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"
#include "ns3/random-walk-2d-mobility-model.h"

#include "nsutil.h"
#include "simulation-area.h"
#include "util.h"

namespace rhpman {

// TODO: Add parameters for neighborhood sizes, tau and sigma.

/// \brief Encapsulates parsing parameters from the CommandLine.
///     Each public member of this class corresponds to a strongly typed object
///     built from one or more of the commandline arguments.
///     Default parameter values are defined in the SimulationParameters::parse
///     method.
/// \see SimulationParameters::parse
class SimulationParameters {
 public:
  /// Simulation RNG seed.
  uint32_t seed;
  /// Simulation runtime.
  ns3::Time runtime;
  /// Total nodes used in the simulation.
  uint32_t totalNodes;
  /// Number of nodes per partition.
  uint32_t nodesPerPartition;
  /// Number of traveller nodes (computed).
  uint32_t travellerNodes;
  /// Number of nodes who own data at the start of the simulation (computed).
  uint32_t dataOwners;
  /// RHPMAN carrying threshold (tau).
  double carryingThreshold;
  /// RHPMAN forwarding threshold (sigma).
  double forwardingThreshold;
  /// Weight of degree connectivity in delivery calculations.
  double wcdc;
  /// Weight of colocation in delivery calculations.
  double wcol;
  /// Time between profile updates.
  ns3::Time profileUpdateDelay;
  /// The number of hops defining the neighborhood of the node.
  uint8_t neighborhoodSize;
  /// The number of hops defining the neighborhood considered for a replicating
  /// node election.
  uint8_t electionNeighborhoodSize;
  /// The simulation area.
  SimulationArea area;
  /// The number of horizontal partitions.
  uint32_t rows;
  /// The number of vertical partitions.
  uint32_t cols;
  /// The velocity of the travellers.
  ns3::Ptr<ns3::ConstantRandomVariable> travellerVelocity;
  /// The period after which traveller nodes should change their direction if
  /// the travellerWalkMode is MODE_TIME.
  ns3::Time travellerDirectionChangePeriod;
  /// The distance after which traveller nodes should change their direction if
  /// the travellerWalkMode is MODE_DISTANCE.
  double travellerDirectionChangeDistance;
  /// Governs the behaviour of the traveller nodes' walking.
  ns3::RandomWalk2dMobilityModel::Mode travellerWalkMode;
  /// The velocity of the partition-bound nodes.
  ns3::Ptr<ns3::UniformRandomVariable> pbnVelocity;
  /// The period after which partition-bound nodes change velocity.
  ns3::Time pbnVelocityChangePeriod;
  /// Indicates the type of routing to use for the simulation.
  rhpman::RoutingType routingProtocol;
  /// The radius of connectivity for each node.
  double wifiRadius;
  /// The path on disk to output the NetAnim trace XML file for visualizing the
  /// results of the simulation.
  std::string netanimTraceFilePath;

  SimulationParameters() {}

  /// \brief Parses command line options to set simulation parameters.
  ///
  /// \param argc The number of command line options passed to the program.
  /// \param argv The raw program arguments.
  /// \return std::pair<SimulationParameters, bool>
  ///   If the boolean value is false, there was an error calling the program,
  ///   and so the construction of the simulation parameters does not make sense.
  ///   If it is true, construction succeeded and the simulation may run.
  ///
  static std::pair<SimulationParameters, bool> parse(int argc, char* argv[]);
};

}  // namespace rhpman

#endif