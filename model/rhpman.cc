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

static Ptr<Packet> GeneratePacket(const rhpman::packets::Message message);

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
              "PingCooldown",
              "Time to wait between sending ping messages (T)",
              TimeValue(5.0_sec),
              MakeTimeAccessor(&RhpmanApp::m_ping_cooldown),
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

  if (m_socket_recv == 0) {
    m_socket_recv = SetupSocket(APPLICATION_PORT, 0);
  }

  if (m_socket_send == 0) {
    m_socket_send = SetupSocket(APPLICATION_PORT, m_neighborhoodHops);
  }

  m_storage.Init(m_storageSpace);
  m_buffer.Init(m_bufferSpace);

  // TODO: Schedule events.
  m_state = State::RUNNING;

  SchedulePing();
}

Ptr<Socket> RhpmanApp::SetupSocket(uint16_t port, uint32_t ttl) {
  Ptr<Socket> socket;

  socket = Socket::CreateSocket(GetNode(), UdpSocketFactory::GetTypeId());

  if (ttl == 0) {
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), port);
    if (socket->Bind(local) == -1) {
      NS_FATAL_ERROR("Failed to bind socket");
    }
  } else {
    socket->Connect(InetSocketAddress(Ipv4Address::GetBroadcast(), port));
    socket->SetAllowBroadcast(true);
    socket->SetIpTtl(ttl);
  }

  socket->SetRecvCallback(MakeCallback(&RhpmanApp::HandleRequest, this));

  return socket;
}

void RhpmanApp::DestroySocket(Ptr<Socket> socket) {
  socket->Close();
  socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
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

  if (m_socket_recv != 0) {
    DestroySocket(m_socket_recv);
    m_socket_recv = 0;
  }

  if (m_socket_send != 0) {
    DestroySocket(m_socket_send);
    m_socket_send = 0;
  }

  // TODO: Cancel events.

  m_state = State::STOPPED;
}

// this is public and is how any data lookup is made
void RhpmanApp::Lookup(uint64_t id) {
  // check cache
  DataItem* item = CheckLocalStorage(id);
  if (item != NULL) {
    if (!m_success.IsNull()) m_success(item);
    return;
  }

  // ask replica holder nodes
  if (m_replicating_nodes.size() != 0) {
    LookupFromReplicaHolders(id);
    return;
  }

  // run semi probabilistic lookup
  double sigma = CalculateProfile();
  std::set<uint32_t> addresses = GetRecipientAddresses(sigma);

  uint64_t requestID = GenerateMessageID();
  Ptr<Packet> message = GenerateLookup(requestID, id, sigma);
  for (std::set<uint32_t>::iterator it = addresses.begin(); it != addresses.end(); ++it) {
    SendMessage(Ipv4Address(*it), message);
  }

  ScheduleLookupTimeout(requestID, id);
}

// this is public and is how new data items are stored in the network
// if there is no more space in the local cache false is returned, otherwise it is true
bool RhpmanApp::Save(DataItem* data) {
  bool status = m_storage.StoreItem(data);
  SendToReplicaHolders(data);

  return status;
}

// ================================================
//  generate messages
// ================================================

Ptr<Packet> RhpmanApp::GenerateLookup(uint64_t messageID, uint64_t dataID, double sigma) {
  rhpman::packets::Message message;
  message.set_id(messageID);
  message.set_timestamp(Simulator::Now().GetMilliSeconds());

  rhpman::packets::Request* request = message.mutable_request();
  request->set_data_id(dataID);
  request->set_requestor(GetID());
  request->set_sigma(sigma);

  return GeneratePacket(message);
}

static Ptr<Packet> GeneratePacket(const rhpman::packets::Message message) {
  uint32_t size = message.ByteSizeLong();
  uint8_t* payload = new uint8_t[size];

  if (!message.SerializeToArray(payload, size)) {
    NS_LOG_ERROR("Failed to serialize the message for transmission");
  }

  Ptr<Packet> packet = Create<Packet>(payload, size);
  delete[] payload;

  return packet;
}

// ================================================
//  send messages
// ================================================

void RhpmanApp::SendMessage(Address dest, Ptr<Packet> packet) {
  // std::cout << "node: " << GetID() << " - " << Simulator::Now().GetSeconds() << "s\n";
  // std::cout << "sending " << packet->GetSize() << " bytes\n";
  // m_socket_send->SendTo(packet, 0, dest);
  m_socket_send->Send(packet);
}

void RhpmanApp::SendProbablisticData(DataItem* data) {
  // check all nodes profiles
  std::set<uint32_t> addresses = GetRecipientAddresses(m_forwardingThreshold);
}

// send the broadcast to all h_r nodes to start a new election
void RhpmanApp::SendStartElection() {
  // just send the broadcast
}

void RhpmanApp::SendPing() {
  // calculate profile
  // check if replicating node
  // send UDP ping to h or h_r hops

  rhpman::packets::Message message;
  message.set_id(GenerateMessageID());
  message.set_timestamp(Simulator::Now().GetMilliSeconds());

  rhpman::packets::Ping* ping = message.mutable_ping();
  // ping.setReplicating_node(m_role == Role::REPLICATING);
  ping->set_delivery_probability(CalculateProfile());

  SendMessage(Ipv4Address::GetBroadcast(), GeneratePacket(message));
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

void RhpmanApp::SendSyncLookup(uint64_t requestID, uint32_t nodeID, uint64_t dataID) {
  // send direct message to peer
}

void RhpmanApp::SendSyncResponse(uint64_t requestID, uint32_t nodeID, DataItem* data) {
  // send direct message to peer
}

void RhpmanApp::SendSyncStore(uint32_t nodeID, DataItem* data) {
  // send direct message to peer
}

// ================================================
//  event schedulers
// ================================================

void RhpmanApp::SchedulePing() {
  if (m_state != State::RUNNING) return;

  SendPing();

  // schedule checking election results
  Simulator::Schedule(m_ping_cooldown, &RhpmanApp::SchedulePing, this);
}

void RhpmanApp::ScheduleElectionCheck() {
  if (m_state != State::RUNNING) return;

  // schedule checking election results
  Simulator::Schedule(m_election_timeout, &RhpmanApp::CheckElectionResults, this);
}

void RhpmanApp::ScheduleElectionWatchdog() {
  if (m_state != State::RUNNING) return;

  m_election_watchdog_event =
      Simulator::Schedule(m_election_watchdog_timeout, &RhpmanApp::ElectionWatchDog, this);
}

void RhpmanApp::ScheduleLookupTimeout(uint64_t requestID, uint64_t dataID) {
  if (m_state != State::RUNNING) return;

  EventId event =
      Simulator::Schedule(m_request_timeout, &RhpmanApp::LookupTimeout, this, requestID);
  m_pendingLookups.insert(requestID);
  m_lookupMapping[requestID] = dataID;
}

void RhpmanApp::ScheduleProfileTimeout(uint32_t nodeID) {
  if (m_state != State::RUNNING) return;

  EventId e = m_profileTimeouts[nodeID];

  if (e.IsRunning()) {
    e.Cancel();
  }

  m_profileTimeouts[nodeID] =
      Simulator::Schedule(m_profile_timeout, &RhpmanApp::ProfileTimeout, this, nodeID);
}

void RhpmanApp::ScheduleReplicaNodeTimeout(uint32_t nodeID) {
  if (m_state != State::RUNNING) return;

  EventId e = m_replicationNodeTimeouts[nodeID];

  if (e.IsRunning()) {
    e.Cancel();
  }

  m_replicationNodeTimeouts[nodeID] = Simulator::
      Schedule(m_missing_replication_timeout, &RhpmanApp::ReplicationNodeTimeout, this, nodeID);
}

// ================================================
//  message handlers
// ================================================

// this will handle all the inital messages to be received by the node
// and will delegate the specfic handling to the separate handlers
void RhpmanApp::HandleRequest(Ptr<Socket> socket) {
  Ptr<Packet> packet;
  Address from;
  Address localAddress;

  while ((packet = socket->RecvFrom(from))) {
    socket->GetSockName(localAddress);
    NS_LOG_INFO(
        "At time " << Simulator::Now().GetSeconds() << "s client received " << packet->GetSize()
                   << " bytes from " << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                   << InetSocketAddress::ConvertFrom(from).GetPort());

    // uint32_t srcAddress = InetSocketAddress::ConvertFrom(from).GetIpv4().Get();
    uint32_t size = packet->GetSize();
    uint8_t* payload = new uint8_t[size];
    packet->CopyData(payload, size);

    // std::cout << "received message from: " << srcAddress << "\n";
    // parse bytes into protobuf object

    // switch based on message type

    // do handling here
  }
}

void RhpmanApp::HandlePing(uint32_t nodeID, double profile, bool isReplication) {
  m_peerProfiles[nodeID] = profile;
  ScheduleProfileTimeout(nodeID);

// if the peer has a higher value then the current node, send items in the buffer to that node
// this is optional
#if defined(__RHPMAN_OPTIONAL_CARRIER_FORWARDING)

  if (profile > CalculateProfile()) {
    TransferBuffer(nodeID);
  }

#endif  // __RHPMAN_OPTIONAL_CARRIER_FORWARDING

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

void RhpmanApp::HandleSyncLookup(uint32_t nodeID, uint64_t requestID, uint64_t dataID) {
  DataItem* res = CheckLocalStorage(dataID);

  if (res != NULL) {
    return SendSyncResponse(requestID, nodeID, res);
  }

  // if not found do nothing, let the client timeout??
}

// this will handle the synchonous store message
// if the node is a replicating node then it will be stored in storage, and stored in the buffer
// otherwise
void RhpmanApp::HandleStore(DataItem* data) {
  Storage storage = m_role == Role::REPLICATING ? m_storage : m_buffer;

  bool res = storage.StoreItem(data);
  if (!res) {
    NS_LOG_DEBUG("not enough space to store the data item");
  }
}

void RhpmanApp::HandleProbabalisticStore(uint32_t nodeID, DataItem* data) {
  // check to see if the node already has the data item in the buffer
  if (CheckLocalStorage(data->getID()) != NULL) return;

  // if it is a replicating node store the data item
  if (m_role == Role::REPLICATING) {
    bool res = m_storage.StoreItem(data);
    if (!res) {
      NS_LOG_DEBUG("not enough space to store the data item");
    }
    return;
  }

  // store in the buffer if there is space
  bool res = m_buffer.StoreItem(data);
  if (!res) {
    NS_LOG_DEBUG("not enough space in the buffer to store the data item");
  }
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

// this will send the synchonous data lookup requests to all of the known replica holder nodes
void RhpmanApp::LookupFromReplicaHolders(uint64_t dataID) {
  // create lookup message (use the same ID for each message sent)
  uint64_t requestID = RhpmanApp::GenerateMessageID();

  for (std::set<uint32_t>::iterator it = m_replicating_nodes.begin();
       it != m_replicating_nodes.end();
       ++it) {
    SendSyncLookup(requestID, *it, dataID);
  }

  ScheduleLookupTimeout(requestID, dataID);
}

// this will be called after the node stores a new data item
// this will send it synchronously
void RhpmanApp::SendToReplicaHolders(DataItem* data) {
  for (std::set<uint32_t>::iterator it = m_replicating_nodes.begin();
       it != m_replicating_nodes.end();
       ++it) {
    SendSyncStore(*it, data);
  }
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

// this will generate the ID value to use for the requests this is a static function that should be
// called to generate all the ids to ensure they are unique
uint64_t RhpmanApp::GenerateMessageID() {
  static uint64_t id = 0;
  return ++id;
}

// reset the fitness map values, call this after checking the election results
void RhpmanApp::ResetFitnesses() {
  m_myFitness = 0;
  m_peerFitness.clear();
}

std::set<uint32_t> RhpmanApp::GetRecipientAddresses(double sigma) {
  std::set<uint32_t> addresses;

  for (std::map<uint32_t, double>::iterator it = m_peerProfiles.begin(); it != m_peerProfiles.end();
       ++it) {
    if (it->second >= sigma) addresses.insert(it->first);
  }

  return addresses;
}

std::set<uint32_t> RhpmanApp::FilterAddresses(
    const std::set<uint32_t> addresses,
    const std::set<uint32_t> exclude) {
  std::set<uint32_t> filtered = addresses;

  for (std::set<uint32_t>::iterator it = exclude.begin(); it != exclude.end(); ++it) {
    if (filtered.find(*it) != filtered.end()) filtered.erase(*it);
  }

  return filtered;
}

// call this to send the all of the contents of the buffer to a new node
void RhpmanApp::TransferBuffer(uint32_t nodeID) {
  std::vector<DataItem*> items = m_buffer.GetAll();

  for (std::vector<DataItem*>::iterator it = items.begin(); it != items.end(); ++it) {
    SendSyncStore(nodeID, *it);
  }

  // remove items from the buffer once they have been transfered so they cant be forwarded again
  m_buffer.ClearStorage();
}

// ================================================
//  calculation helpers
// ================================================

double RhpmanApp::CalculateElectionFitness() {
  m_myFitness = 0;
  return m_myFitness;
}

// this is the value of P_ij
double RhpmanApp::CalculateProfile() {
  if (m_role == Role::REPLICATING) return 1;

  return m_wcdc * CalculateChangeDegree() + m_wcol * CalculateColocation();
}

// this is the U_cdc value
double RhpmanApp::CalculateChangeDegree() { return 0; }

// this is the U_col value
// it is 1 if there is a replication node within h, 0 otherwise
double RhpmanApp::CalculateColocation() {
  if (m_role == Role::REPLICATING) return 1;

  // check routing table here

  return 0;
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

    if (!m_failed.IsNull()) m_failed(mapping->second);
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

  ResetFitnesses();

  Role oldRole = m_role;
  if (highest) {
    MakeReplicaHolderNode();
  } else if (oldRole == Role::REPLICATING && !highest) {
    MakeNonReplicaHolderNode();
  }
}

// this will remove the nodes information from the probabilistic table
void RhpmanApp::ProfileTimeout(uint32_t nodeID) { m_peerProfiles.erase(nodeID); }

// this will remove the replication node from the list if missed its checkins
void RhpmanApp::ReplicationNodeTimeout(uint32_t nodeID) { m_replicating_nodes.erase(nodeID); }

// ================================================
// storage array functions
// ================================================

// this is a helper and will return the number of data items that can be stored in local storage
uint32_t RhpmanApp::GetFreeSpace() { return m_storage.GetFreeSpace(); }

DataItem* RhpmanApp::CheckLocalStorage(uint64_t dataID) {
  // check cache
  DataItem* item = m_storage.GetItem(dataID);
  if (item != NULL) return item;

#if defined(__RHPMAN_OPTIONAL_CHECK_BUFFER)

  // check the data items in the buffer
  item = m_buffer.GetItem(dataID);
  if (item != NULL) return item;

#endif  // __RHPMAN_OPTIONAL_CHECK_BUFFER

  return NULL;
}

}  // namespace rhpman
