#pragma once

#include "common/config/Configuration.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "networking/messaging/IMessageEndpoint.h"
#include "networking/messaging/impl/websockets/boost/BoostWebSocketEndpoint.h"
#include <memory>

namespace hive::networking {

typedef messaging::websockets::BoostWebSocketEndpoint
    DefaultMessageEndpointImpl;

/**
 * Central networking manager that provides connectivity for higher level
 * subsystems.
 */
class NetworkingManager {
private:
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;
  common::config::SharedConfiguration m_config;

  /** Local message-oriented endpoint for communication with other nodes */
  common::memory::Reference<messaging::IMessageEndpoint> m_message_endpoint;

  void StartMessageEndpointServer();
  void ConfigureNode(const common::config::SharedConfiguration &config);

public:
  NetworkingManager(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      const common::config::SharedConfiguration &config);
};

} // namespace hive::networking
