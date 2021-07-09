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

#include <map>
#include <set>  // std::set

#include "ns3/application-container.h"
#include "ns3/application.h"
#include "ns3/applications-module.h"
#include "ns3/attribute.h"
#include "ns3/callback.h"
//#include "ns3/core-module.h"
#include "ns3/event-id.h"
#include "ns3/node-container.h"
#include "ns3/object-base.h"
#include "ns3/object-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

#include "dataItem.h"
//#include "proto/messages.pb.h"

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
        m_degreeConnectivity(),
        m_socket(0){};

  Ptr<Socket> GetSocket() const;
  Role GetRole() const;
  State GetState() const;

  void Lookup(uint64_t id);
  bool Save(DataItem* data);
  uint32_t GetFreeSpace();

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
  std::map<Time, uint32_t> m_degreeConnectivity;
  Ptr<Socket> m_socket;

  // timeouts
  Time m_request_timeout;
  Time m_election_watchdog_timeout;  // this is the timeout used
  Time m_election_timeout;
  Time m_election_cooldown;
  Time m_ping_cooldown;
  Time m_min_election_time;  // this is the earliest time that another election is allowed to be
                             // requested by a node

  // event handlers
  void LookupTimeout(uint64_t requestID);
  void ElectionWatchDog();
  void CheckElectionResults();

  EventId m_election_watchdog_event;

  // event triggers
  void SendStartElection();
  void SendPing();
  void SendFitness();
  void SendRoleChange(uint32_t newReplicationNode);
  void SendSyncLookup(uint64_t requestID, uint32_t nodeID, uint64_t dataID);
  void SendSyncStore(uint32_t nodeID, DataItem* data);

  // schedulers
  void ScheduleElectionCheck();
  void ScheduleElectionWatchdog();
  void ScheduleLookupTimeout(uint64_t requestID, uint64_t dataID);

  // other helpers
  void RunElection();
  void MakeReplicaHolderNode();
  void MakeNonReplicaHolderNode();
  void LookupFromReplicaHolders(uint64_t dataID);
  void SendToReplicaHolders(DataItem* data);
  uint32_t GetID();
  static uint64_t GenerateRequestID();

  // calculation helpers
  double CalculateElectionFitness();

  // message handlers
  void HandlePing(uint32_t nodeID, bool isReplication);
  void HandleModeChange(uint32_t oldNode, uint32_t newNode);
  void HandleElectionRequest();
  void HandleElectionFitness(uint32_t nodeID, double fitness);

  // data storage for the node
  uint32_t m_storageSpace;
  std::vector<DataItem*> m_storage;

  void InitStorage();
  bool StoreItem(DataItem* data);
  DataItem* GetItem(uint64_t dataID);
  bool RemoveItem(uint64_t dataID);
  void ClearStorage();

  // callbacks for lookup requests
  Callback<void, uint64_t> m_failed;
  Callback<void, DataItem*> m_success;

  std::set<uint64_t> m_pendingLookups;
  std::map<uint64_t, uint64_t> m_lookupMapping;

  // replication values

  std::map<uint32_t, double> m_peerFitness;

  double m_myFitness;
  std::set<uint32_t> m_replicating_nodes;
};

};  // namespace rhpman

#endif
