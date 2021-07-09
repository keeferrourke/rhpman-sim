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
//#include "ns3/core-module.h"
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

#include "proto/messages.pb.h"

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
              MakeTimeChecker(0.1_sec))
          .AddAttribute(
              "lookup_success_cb",
              "a callback to be called when a data item is successfully found",
              CallbackValue(),
              MakeCallbackAccessor(&RhpmanApp::m_success),
              MakeCallbackChecker())
          .AddAttribute(
              "lookup_failed_cb",
              "a callback to be called when a data lookup times out",
              CallbackValue(),
              MakeCallbackAccessor(&RhpmanApp::m_failed),
              MakeCallbackChecker());
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

  // set the default callbacks so it does not crash
  m_success = MakeNullCallback<void, uint64_t, DataItem*>();
  m_failed = MakeNullCallback<void, uint64_t, uint64_t>();

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

// this is public and is how any data lookup is made
void RhpmanApp::Lookup(uint64_t id) {
  // check local storage
  // ask peers
  // schedule timeouts
}

// this is public and is how new data items are stored in the network
// if there is no more space in the local cache false is returned, otherwise it is true
bool RhpmanApp::Save(DataItem* data) {
  // store in local data storage
  // send to peers
  return false;
}

// ================================================
//  send messages
// ================================================

// send the broadcast to all h_r nodes to start a new election
void RhpmanApp::SendStartElection() {
  // just send the broadcast
}

void RhpmanApp::SendPing() {
  // calculate fitness
  // check if replicating node
  // send UDP ping to h or h_r hops

  // schedule next ping
  if (m_state == State::RUNNING) {
    Simulator::Schedule(m_ping_cooldown, &RhpmanApp::SendPing, this);
  }
}

// broadcast fitness value to all nodes in h_r hops
void RhpmanApp::SendFitness() {
  // send UDP broadcast
  // set TTL to configure the distance it can travel
}

// this will take the IP address of the new node to become a replicating node
// if the node is stepping up then this should be set to the current ip
// if the node is stepping down this should be 0
void RhpmanApp::SendRoleChange(uint32_t newReplicationNode) {
  // send broadcast to h_r nodes
}

// ================================================
//  event schedulers
// ================================================

void RhpmanApp::ScheduleElectionCheck() {
  // schedule checking election results
  if (m_state == State::RUNNING) {
    Simulator::Schedule(m_election_timeout, &RhpmanApp::CheckElectionResults, this);
  }
}

void RhpmanApp::ScheduleElectionWatchdog() {
  if (m_state == State::RUNNING) {
    m_election_watchdog_event =
        Simulator::Schedule(m_election_watchdog_timeout, &RhpmanApp::ElectionWatchDog, this);
  }
}

// ================================================
//  message handlers
// ================================================

void RhpmanApp::HandlePing(uint32_t nodeID, bool isReplication) {
  // reset the watchdog timer
  if (isReplication && m_election_watchdog_event.IsRunning()) {
    m_election_watchdog_event.Cancel();
    ScheduleElectionWatchdog();
  }
}

// this will handle updating the nodes in the replication node list
// step up: old === new             -> insert into list
// step down: new == 0              -> remove from list
// handover: old != new, new != 0   -> remove old from list, insert new into list
void RhpmanApp::HandleModeChange(uint32_t oldNode, uint32_t newNode) {
  if (oldNode == newNode) {
    m_replicating_nodes.insert(newNode);
  } else if (newNode == 0) {
    m_replicating_nodes.erase(oldNode);
  } else {
    m_replicating_nodes.erase(oldNode);
    m_replicating_nodes.insert(newNode);
  }
}

void RhpmanApp::HandleElectionRequest() {
  // check if currently running an election
  if (Simulator::Now() < m_min_election_time) {
    NS_LOG_DEBUG("too early to run another election");
    return;
  }

  RunElection();
}

void RhpmanApp::HandleElectionFitness(uint32_t nodeID, double fitness) {
  m_peerFitness[nodeID] = fitness;
}

// ================================================
//   helpers
// ================================================

void RhpmanApp::RunElection() {
  m_min_election_time = Simulator::Now() + m_election_cooldown;

  CalculateElectionFitness();
  SendFitness();
  ScheduleElectionCheck();
}

// this helper will convert the new role to the ip address that should be sent
void RhpmanApp::MakeReplicaHolderNode() {
  m_role = Role::REPLICATING;
  SendRoleChange(GetID());
}

// this helper will convert the new role to the ip address that should be sent
void RhpmanApp::MakeNonReplicaHolderNode() {
  m_role = Role::NON_REPLICATING;
  SendRoleChange(0);
}

// this will get the nodes IPv4 address and return it as a 32 bit integer
uint32_t RhpmanApp::GetID() {
  Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4>();
  Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1, 0);
  Ipv4Address ipAddr = iaddr.GetLocal();

  uint32_t addr = ipAddr.Get();

  // This next line will print the ipv4 address in a.b.c.d format
  // std::cout << "addr: " << ((addr >> 24) & 0x00ff) << "." << ((addr >> 16) & 0x00ff) << "." <<
  // ((addr >> 8) & 0x00ff) << "." << ((addr) & 0x00ff) << "\n";

  return addr;
}

// ================================================
//  calculation helpers
// ================================================

double RhpmanApp::CalculateElectionFitness() {
  m_myFitness = 0;
  return m_myFitness;
}

// ================================================
//  Scheduled event handlers
// ================================================

// this will call the lookup failed callback for the request
void RhpmanApp::LookupTimeout(uint64_t requestID) {
  NS_LOG_FUNCTION(this);

  // check to see if it is still in the pending lookup list
  std::set<uint64_t>::iterator it = m_pendingLookups.find(requestID);
  std::map<uint64_t, uint64_t>::iterator mapping;

  if (it != m_pendingLookups.end()) {
    mapping = m_lookupMapping.find(requestID);

    if (!m_failed.IsNull()) m_failed(requestID, mapping->second);
    m_pendingLookups.erase(it);
  }
}

// this event must be canceled / restarted whenever a 'ping' is received from one of the replicating
// nodes do not schedule this if current node is a replicating node if this triggers then start an
// election
void RhpmanApp::ElectionWatchDog() {
  // check if currently running an election
  if (Simulator::Now() < m_min_election_time) {
    NS_LOG_DEBUG("too early to run another election");
    return;
  }

  NS_LOG_DEBUG("Cant connect to any replicating nodes, triggering an election");

  SendStartElection();
  RunElection();
}

// this will be triggered after an election delay to check to see if the current node should become
// a replication node announce to all the other nodes the results if the status is changing
void RhpmanApp::CheckElectionResults() {
  bool highest = true;
  for (std::map<uint32_t, double>::iterator it = m_peerFitness.begin(); it != m_peerFitness.end();
       ++it) {
    if (m_myFitness < it->second) {
      highest = false;
      break;
    }
  }

  Role oldRole = m_role;
  if (highest) {
    MakeReplicaHolderNode();
  } else if (oldRole == Role::REPLICATING && !highest) {
    MakeNonReplicaHolderNode();
  }
}

// ================================================
// storage array functions
// ================================================

// this will set all possitions in the storage list to NULL so it can be used
void RhpmanApp::InitStorage() {
  m_storage.resize(m_storageSpace);
  for (uint32_t i = 0; i < m_storageSpace; i++) {
    m_storage[i] = NULL;
  }
}

// this will do the actual storage. will store the item not a copy
// true if there was space false otherwise
bool RhpmanApp::StoreItem(DataItem* data) {
  bool saved = false;
  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] == NULL) {
      m_storage[i] = data;
      saved = true;
      break;
    }
  }

  return saved;
}

// this will return a pointer to the data item if it is found or NULL if it is not
DataItem* RhpmanApp::GetItem(uint64_t dataID) {
  DataItem* found = NULL;

  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL && m_storage[i]->getID() == dataID) {
      found = m_storage[i];
      break;
    }
  }

  return found;
}

// return true if the item was removed from storage, false if it was not found
bool RhpmanApp::RemoveItem(uint64_t dataID) {
  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL && m_storage[i]->getID() == dataID) {
      free(m_storage[i]);
      m_storage[i] = NULL;
      return true;
    }
  }

  return false;
}

// this will empty all data items from storage
void RhpmanApp::ClearStorage() {
  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL) {
      free(m_storage[i]);
      m_storage[i] = NULL;
    }
  }
}

// this is a helper and will return the number of data items that can be stored in local storage
uint32_t RhpmanApp::GetFreeSpace() {
  uint32_t count = 0;

  for (uint32_t i = 0; i < m_storageSpace; i++) {
    if (m_storage[i] != NULL) {
      count++;
    }
  }

  return m_storageSpace - count;
}

}  // namespace rhpman
