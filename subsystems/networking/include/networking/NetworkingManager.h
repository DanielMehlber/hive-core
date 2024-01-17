#ifndef NETWORKINGMANAGER_H
#define NETWORKINGMANAGER_H

#include "NetworkingFactory.h"
#include "common/config/Configuration.h"
#include "common/subsystems/SubsystemManager.h"
#include <memory>

namespace networking {

/**
 * @brief Central networking manager that provides connectivity.
 */
class NetworkingManager {
private:
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;
  common::config::SharedConfiguration m_config;

  SharedWebSocketPeer m_networking_peer;

  void InitPeerNetworkingServer();

public:
  NetworkingManager(
      const common::subsystems::SharedSubsystemManager &subsystems,
      const common::config::SharedConfiguration &config);
};

typedef std::shared_ptr<NetworkingManager> SharedNetworkingManager;

} // namespace networking

#endif /* NETWORKINGMANAGER_H */
