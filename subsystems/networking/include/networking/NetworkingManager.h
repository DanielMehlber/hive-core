#ifndef NETWORKINGMANAGER_H
#define NETWORKINGMANAGER_H

#include "NetworkingFactory.h"
#include "common/config/Configuration.h"
#include "common/subsystems/SubsystemManager.h"
#include <memory>

namespace networking {

/**
 * Central networking manager that provides connectivity for higher level
 * subsystems.
 */
class NetworkingManager {
private:
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;
  common::config::SharedConfiguration m_config;

  /** Local message-oriented endpoint for communication with other nodes */
  SharedMessageEndpoint m_message_endpoint;

  void StartMessageEndpointServer();

public:
  NetworkingManager(
      const common::subsystems::SharedSubsystemManager &subsystems,
      const common::config::SharedConfiguration &config);
};

typedef std::shared_ptr<NetworkingManager> SharedNetworkingManager;

} // namespace networking

#endif /* NETWORKINGMANAGER_H */
