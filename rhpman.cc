/// \file rhpman.cc
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

#include <algorithm>
#include <map>

#include "ns3/application-container.h"
#include "ns3/application.h"
#include "ns3/applications-module.h"
#include "ns3/attribute.h"
#include "ns3/core-module.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/node-container.h"
#include "ns3/object-base.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/udp-socket-factory.h"

#include "logging.h"
#include "nsutil.h"
#include "rhpman.h"
#include "util.h"

namespace rhpman {

using namespace ns3;

NS_OBJECT_ENSURE_REGISTERED(RhpmanApp);

// static
TypeId RhpmanApp::GetTypeId() {
  static TypeId id =
      TypeId("rhpman::RhpmanApp")
          .SetParent<Application>()
          .SetGroupName("Applications")
          .AddConstructor<RhpmanApp>()
          .AddAttribute(
              "Role",
              "The initial role of this app",
              EnumValue(Role::NON_REPLICATING),
              MakeEnumAccessor(&RhpmanApp::m_role),
              MakeEnumChecker<Role>(
                  Role::NON_REPLICATING,
                  "Role::NONE_REPLICATING",
                  Role::REPLICATING,
                  "Role::REPLICATING"))
          .AddAttribute(
              "DataId",
              "The data this application must distribute",
              IntegerValue(-1),
              MakeIntegerAccessor(&RhpmanApp::m_dataId),
              MakeEmptyAttributeChecker())
          .AddAttribute(
              "ForwardingThreshold",
              "If probability of delivery to a node is higher than this value, data is forwarded "
              "(sigma)",
              DoubleValue(0.4),
              MakeDoubleAccessor(&RhpmanApp::m_forwardingThreshold),
              MakeDoubleChecker<double>(0.0, 1.0))
          .AddAttribute(
              "CarryingThreshold",
              "If probability of delivery to a node is hight than this value, data is cached (tau)",
              DoubleValue(0.6),
              MakeDoubleAccessor(&RhpmanApp::m_carryingThreshold),
              MakeDoubleChecker<double>(0.0, 1.0))
          .AddAttribute(
              "DegreeConnectivityWeight",
              "Weight of degree connectivity for computing delivery probabilities (w_cdc)",
              DoubleValue(0.5),
              MakeDoubleAccessor(&RhpmanApp::m_wcdc),
              MakeDoubleChecker<double>(0.0))
          .AddAttribute(
              "ColocationWeight",
              "Weight of colocation for computing delivery probabilities (w_col)",
              DoubleValue(0.5),
              MakeDoubleAccessor(&RhpmanApp::m_wcol),
              MakeDoubleChecker<double>(0.0))
          .AddAttribute(
              "NeighborhoodSize",
              "Number of hops considered to be in the neighborhood of this node (h)",
              UintegerValue(2),
              MakeUintegerAccessor(&RhpmanApp::m_neighborhoodHops),
              MakeUintegerChecker<uint32_t>(1))
          .AddAttribute(
              "ElectionNeighborhoodSize",
              "Number of hops considered to be in the election neighborhood of this node (h_r)",
              UintegerValue(4),
              MakeUintegerAccessor(&RhpmanApp::m_electionNeighborhoodHops),
              MakeUintegerChecker<uint32_t>(1))
          .AddAttribute(
              "ProfileUpdateDelay",
              "Time to wait between profile update and exchange (T)",
              TimeValue(6.0_sec),
              MakeTimeAccessor(&RhpmanApp::m_profileDelay),
              MakeTimeChecker(0.1_sec));
  return id;
}

Ptr<Socket> RhpmanApp::GetSocket() const { return m_socket; }

RhpmanApp::Role RhpmanApp::GetRole() const { return m_role; }

RhpmanApp::State RhpmanApp::GetState() const { return m_state; }

// override
void RhpmanApp::StartApplication() {
  if (m_state == State::RUNNING) {
    NS_LOG_DEBUG("Ignoring RhpmanApp::StartApplication request on already started application");
    return;
  }
  NS_LOG_DEBUG("Starting RhpmanApp");
  m_state = State::NOT_STARTED;

  // TODO: I think I need multiple sockets? Maybe not though.
  // - One socket for replication election; broadcast special packet (TODO) to
  //   determine roles present in election neighborhood
  // - One socket for actual data transmission (some packet with unique data)
  //

  if (m_socket == 0) {
    m_socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());
    m_socket->Bind();
  }

  // TODO: Schedule events.

  m_state = State::RUNNING;
}

// override
void RhpmanApp::StopApplication() {
  if (m_state == State::NOT_STARTED) {
    NS_LOG_ERROR("Called RhpmanApp::StopApplication on a NOT_STARTED instance");
    return;
  }
  if (m_state == State::STOPPED) {
    NS_LOG_DEBUG("Ignoring RhpmanApp::StopApplication on already stopped instance");
  }

  // TODO: Cancel events.

  m_state = State::STOPPED;
}

void RhpmanAppHelper::SetAttribute(std::string name, const AttributeValue& value) {
  m_factory.Set(name, value);
}

void RhpmanAppHelper::SetDataOwners(uint32_t num) { m_dataOwners = num; }

ApplicationContainer RhpmanAppHelper::Install(NodeContainer nodes) {
  ApplicationContainer apps;

  rand->SetAttribute("Min", DoubleValue(0));
  rand->SetAttribute("Max", DoubleValue(nodes.GetN()));

  std::vector<uint32_t> dataOwnerIds;
  for (size_t i = 0; i < m_dataOwners; i++) {
    uint32_t id = 0;
    do {
      id = rand->GetInteger();
    } while (std::find(dataOwnerIds.begin(), dataOwnerIds.end(), id) != dataOwnerIds.end());
    dataOwnerIds.push_back(id);
  }

  NS_ASSERT(dataOwnerIds.size() == m_dataOwners);

  NS_LOG_DEBUG("Data owner nodes: " << dataOwnerIds);

  for (size_t i = 0; i < nodes.GetN(); i++) {
    Ptr<Node> node = nodes.Get(i);
    if (std::find(dataOwnerIds.begin(), dataOwnerIds.end(), i) != dataOwnerIds.end()) {
      m_factory.Set("Role", EnumValue(RhpmanApp::Role::REPLICATING));
      m_factory.Set("DataId", IntegerValue(i));
    }
    apps.Add(createAndInstallApp(node));
    m_factory.Set("Role", EnumValue(RhpmanApp::Role::NON_REPLICATING));
    m_factory.Set("DataId", IntegerValue(-1));
  }
  return apps;
}

ApplicationContainer RhpmanAppHelper::Install(Ptr<Node> node) const {
  ApplicationContainer apps;
  apps.Add(createAndInstallApp(node));
  return apps;
}

Ptr<Application> RhpmanAppHelper::createAndInstallApp(Ptr<Node> node) const {
  Ptr<Application> app = m_factory.Create<Application>();
  node->AddApplication(app);
  return app;
}

}  // namespace rhpman