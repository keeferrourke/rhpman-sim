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

// Uncomment this to enable the optional carrier forwarding
#define __RHPMAN_OPTIONAL_CARRIER_FORWARDING

// uncomment to enable the optional checking the data items in the buffer when doing a lookup
#define __RHPMAN_OPTIONAL_CHECK_BUFFER

#define APPLICATION_PORT 5000

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
#include "storage.h"

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
        m_socket_recv(0),
        m_neighborhood_socket(0),
        m_election_socket(0),
        m_success(MakeNullCallback<void, DataItem*>()),
        m_failed(MakeNullCallback<void, uint64_t>()){};

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

  Ptr<Socket> m_socket_recv;
  Ptr<Socket> m_neighborhood_socket;
  Ptr<Socket> m_election_socket;

  // callbacks for lookup requests
  // callbacks for  requests
  Callback<void, DataItem*> m_success;
  Callback<void, uint64_t> m_failed;

  // timeouts
  Time m_request_timeout;
  Time m_missing_replication_timeout;  // if a replication node does not checkin before this time it
                                       // is removed from the list
  Time m_profile_timeout;
  Time m_election_timeout;
  Time m_election_cooldown;
  Time m_min_election_time;  // this is the earliest time that another election is allowed to be
                             // requested by a node

  // event handlers
  void LookupTimeout(uint64_t requestID);
  void TriggerElection();
  void CheckElectionResults();
  void ProfileTimeout(uint32_t nodeID);
  void ReplicationNodeTimeout(uint32_t nodeID);
  void HandleReplicationAnnouncement(uint32_t nodeID);

  EventId m_election_watchdog_event;
  EventId m_replica_announcement_event;

  // event triggers
  void BroadcastToNeighbors(Ptr<Packet> packet);
  void BroadcastToElection(Ptr<Packet> packet);
  void SendMessage(Ipv4Address dest, Ptr<Packet> packet);
  void SendStartElection();
  void SendPing();
  void SendReplicationAnnouncement();
  void SendFitness();
  void SendRoleChange(uint32_t newReplicationNode);
  void SendSyncLookup(uint64_t requestID, uint32_t nodeID, uint64_t dataID);
  void SendSyncStore(uint32_t nodeID, DataItem* data);
  void SendResponse(uint64_t requestID, uint32_t nodeID, const DataItem* data);

  // schedulers
  void ScheduleElectionCheck();
  void ScheduleElectionWatchdog();
  void ScheduleLookupTimeout(uint64_t requestID, uint64_t dataID);
  void ScheduleProfileTimeout(uint32_t nodeID);
  void ScheduleReplicaNodeTimeout(uint32_t nodeID);
  void SchedulePing();
  void ScheduleReplicaHolderAnnouncement();

  // other helpers
  void RunElection();
  void MakeReplicaHolderNode();
  void MakeNonReplicaHolderNode();
  void LookupFromReplicaHolders(uint64_t dataID, uint64_t requestID, uint32_t srcNode);
  uint32_t GetID();
  static uint64_t GenerateMessageID();
  void ResetFitnesses();
  std::set<uint32_t> GetRecipientAddresses(double sigma);
  std::set<uint32_t> FilterAddresses(
      const std::set<uint32_t> addresses,
      const std::set<uint32_t> exclude);
  std::set<uint32_t> FilterAddress(const std::set<uint32_t> addresses, uint32_t exclude);
  void TransferBuffer(uint32_t nodeID);
  DataItem* CheckLocalStorage(uint64_t dataID);

  Ptr<Socket> SetupSocket(uint16_t port, uint32_t ttl);
  Ptr<Socket> SetupRcvSocket(uint16_t port);
  Ptr<Socket> SetupSendSocket(uint16_t port, uint8_t ttl);
  RhpmanApp::Role GetNewRole();
  void ChangeRole(Role newRole);
  void CancelEventMap(std::map<uint32_t, EventId> events);
  void RunProbabilisticLookup(uint64_t requestID, uint64_t dataID, uint32_t srcNode);
  bool CheckDuplicateMessage(uint64_t messageID);
  bool IsResponsePending(uint64_t requestID);
  void DestroySocket(Ptr<Socket> socket);

  void SemiProbabilisticSend(Ptr<Packet> message, uint32_t srcAddr, double sigma);
  void SendToNodes(Ptr<Packet> message, const std::set<uint32_t> nodes);

  // calculation helpers
  double CalculateElectionFitness();
  double CalculateProfile();
  double CalculateChangeDegree();
  double CalculateColocation();

  // message handlers
  void HandleRequest(Ptr<Socket> socket);
  void HandlePing(uint32_t nodeID, double profile);
  void HandleModeChange(uint32_t oldNode, uint32_t newNode);
  void HandleElectionRequest();
  void HandleElectionFitness(uint32_t nodeID, double fitness);
  void HandleLookup(uint32_t nodeID, uint64_t requestID, uint64_t dataID);
  void HandleStore(uint32_t nodeID, DataItem* data, Ptr<Packet> message);
  uint32_t HandleTransfer(std::vector<DataItem*> data);
  void HandleResponse(uint64_t requestID, DataItem* data);

  // message generators
  Ptr<Packet> GenerateLookup(uint64_t messageID, uint64_t dataID, double sigma, uint32_t srcNode);
  Ptr<Packet> GenerateStore(const DataItem* data);
  Ptr<Packet> GeneratePing(double profile);
  Ptr<Packet> GenerateReplicaAnnouncement();
  Ptr<Packet> GenerateElectionRequest();
  Ptr<Packet> GenerateModeChange(uint32_t newNode);
  Ptr<Packet> GenerateTransfer(std::vector<DataItem*> items);
  Ptr<Packet> GenerateResponse(uint64_t responseTo, const DataItem* data);

  // data storage for the node
  uint32_t m_storageSpace;
  uint32_t m_bufferSpace;

  uint32_t m_address;

  Storage m_storage;
  Storage m_buffer;

  std::set<uint64_t> m_pendingLookups;
  std::map<uint64_t, uint64_t> m_lookupMapping;

  // replication values

  std::map<uint32_t, double> m_peerFitness;

  std::map<uint32_t, double> m_peerProfiles;
  std::map<uint32_t, EventId> m_profileTimeouts;
  std::map<uint32_t, EventId> m_replicationNodeTimeouts;

  double m_myFitness;
  std::set<uint32_t> m_replicating_nodes;

  std::set<uint64_t> m_received_messages;
};

};  // namespace rhpman

#endif
