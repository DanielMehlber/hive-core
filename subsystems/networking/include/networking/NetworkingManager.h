#ifndef NETWORKINGMANAGER_H
#define NETWORKINGMANAGER_H

#include "common/config/Configuration.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "networking/messaging/IMessageEndpoint.h"
#include "networking/messaging/impl/websockets/boost/BoostWebSocketEndpoint.h"
#include <memory>

namespace networking {

typedef networking::messaging::websockets::BoostWebSocketEndpoint
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

public:
  NetworkingManager(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      const common::config::SharedConfiguration &config);
};

} // namespace networking

#endif /* NETWORKINGMANAGER_H */
