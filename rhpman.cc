/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include <sysexits.h>

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/core-module.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/netanim-module.h"
#include "ns3/node-container.h"

#include "grid-divider.h"
#include "rhpman.h"
#include "util.h"

using namespace ns3;
using namespace rhpman;

/// This simulation attempts to reproduce an implementation of the RHPMAN
/// Scheme and performance evaluation, as discussed in:
///
///   K. Shi and H. Chen, "RHPMAN: Replication in highly partitioned mobile
///   ad hoc networks," International Journal of Distributed Sensor Networks
///   Jul. 2014.
///
int main(int argc, char* argv[]) {
    int32_t totalNodes = 160;
    int32_t nodesPerPartition = 8;

    double gridWidth = 1000.0_m;
    double gridHeight = 1000.0_m;

    int32_t partitionsX = 4;
    int32_t partitionsY = 4;

    double travellerVelocity = 20.0_mps;

    std::string animationTraceFile = "rhpman.xml";

    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // Set up the travellers.
    // Travellers can move across the whole simulation space.
    int32_t numTravellers =
        totalNodes - (nodesPerPartition * (partitionsX * partitionsY));
    NodeContainer travellers;
    travellers.Create(numTravellers);

    GridDivider grid(std::pair<double, double>(0.0, 0.0),
        std::pair<double, double>(gridWidth, gridHeight));
    std::cout << grid.to_string() << std::endl;

    MobilityHelper travellerMobility;
    Ptr<ConstantRandomVariable> travellerSpeed = CreateObject<ConstantRandomVariable>();
    travellerSpeed->SetAttribute("Constant", DoubleValue(travellerVelocity));

    travellerMobility.SetPositionAllocator(grid.getRandomBoxPositionAllocator());
    travellerMobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
        "Bounds",
        RectangleValue(grid.asRectangle()),
        "Speed", PointerValue(travellerSpeed));

    travellerMobility.Install(travellers);

    auto partitions = grid.split(partitionsX, partitionsY);
    std::cout << partitions << std::endl;

    // Shi and Chen describe a 4x4 grid of 16 partitions in the simulation.
    // std::vector<NodeContainer> partitionBoundGroups(16);

    AnimationInterface anim(animationTraceFile);
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    Simulator::Destroy();

    return EX_OK;
}
