/// \file rhpman.h
/// \author Keefer Rourke <krourke@uoguelph.ca>
/// \brief Declarations for the a simulation that attempts to reproduce the
///     RHPMAN scheme and performance evaluation from Shi and Chen 2014.
///
///     *References*:
///      - K. Shi and H. Chen, "RHPMAN: Replication in highly partitioned mobile
///        ad hoc networks," International Journal of Distributed Sensor Networks
///        Jul. 2014.
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
///
#ifndef __rhpman_h
#define __rhpman_h

#include <bits/stdint-uintn.h>
#include <map>

#include "ns3/application-container.h"
#include "ns3/application.h"
#include "ns3/applications-module.h"
#include "ns3/attribute.h"
#include "ns3/core-module.h"
#include "ns3/node-container.h"
#include "ns3/object-base.h"
#include "ns3/object-factory.h"
#include "ns3/socket.h"

namespace rhpman {

using namespace ns3;

/// \brief RHPMAN network application, which defines data transfer, node
///     election, etc.
///     If this app instance is a data owner, its role will be set to
///     REPLICATING and its DataId will be non-negative.
///     App instances which are not data owners will have negative a DataId.
class RhpmanApp : public Application {
 public:
  enum Role { NON_REPLICATING = 0, REPLICATING };

  /// \brief Identifies the lifecycle state of this app.
  enum class State { NOT_STARTED = 0, RUNNING, STOPPED };

  static TypeId GetTypeId();

  RhpmanApp()
      : m_state(State::NOT_STARTED),
        m_role(Role::NON_REPLICATING),
        m_forwardingThreshold(0.4),
        m_carryingThreshold(0.6),
        m_wcdc(0.5),
        m_wcol(0.5),
        m_neighborhoodHops(2),
        m_electionNeighborhoodHops(4),
        m_profileDelay(),
        m_storage(),
        m_degreeConnectivity(),
        m_socket(0),
        m_dataId(-1){};

  Ptr<Socket> GetSocket() const;
  Role GetRole() const;
  State GetState() const;
  int32_t GetDataId() const {
    return m_dataId;
  }

 private:
  // Application lifecycle methods.

  void StartApplication() override;
  void StopApplication() override;

  // RHPMAN Scheme methods.

  void UpdateProfile();
  void ExchangeProfiles();

  // Member fields.

  State m_state;
  Role m_role;
  double m_forwardingThreshold;
  double m_carryingThreshold;
  double m_wcdc;
  double m_wcol;
  uint32_t m_neighborhoodHops;
  uint32_t m_electionNeighborhoodHops;
  Time m_profileDelay;
  std::vector<uint32_t> m_storage;
  std::map<Time, uint32_t> m_degreeConnectivity;
  Ptr<Socket> m_socket;
  int32_t m_dataId;
};

/// \brief Helper class to install the RhpmanApplication on a Node containers.
///     This can be configured to assign initial replication roles according to
///     to some random distribution.
///     The defaults of this class as described by Shi and Chen in their paper.
class RhpmanAppHelper {
 public:
  RhpmanAppHelper(uint32_t dataOwners = 0) : m_dataOwners(dataOwners) {
    m_factory.SetTypeId(RhpmanApp::GetTypeId());
    rand = CreateObject<UniformRandomVariable>();
  };

  void SetAttribute(std::string name, const AttributeValue& value);
  void SetDataOwners(uint32_t num);

  /// \brief Configures a RHPMAN application and installs it on each node.
  ApplicationContainer Install(NodeContainer nodes);
  ApplicationContainer Install(Ptr<Node> node) const;
  ApplicationContainer Install(std::string nodeName) const;

 private:
  Ptr<Application> createAndInstallApp(Ptr<Node> node) const;
  ObjectFactory m_factory;
  Ptr<UniformRandomVariable> rand;
  uint32_t m_dataOwners;
};

};  // namespace rhpman

#endif