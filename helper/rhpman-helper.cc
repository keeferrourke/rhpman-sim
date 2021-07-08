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

#include "../model/logging.h"
#include "../model/nsutil.h"
#include "../model/util.h"
#include "rhpman-helper.h"

namespace rhpman {

using namespace ns3;

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
      // m_factory.Set("DataId", IntegerValue(i));
    }
    apps.Add(createAndInstallApp(node));
    m_factory.Set("Role", EnumValue(RhpmanApp::Role::NON_REPLICATING));
    // m_factory.Set("DataId", IntegerValue(-1));
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
