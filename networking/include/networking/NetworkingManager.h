#ifndef NETWORKINGMANAGER_H
#define NETWORKINGMANAGER_H

#include "NetworkingFactory.h"
#include "common/subsystems/SubsystemManager.h"
#include "jobsystem/JobSystem.h"
#include "networking/Networking.h"
#include "properties/PropertyProvider.h"
#include <memory>

namespace networking {

/**
 * @brief Central networking manager that provides connectivity.
 */
class NETWORKING_API NetworkingManager {
private:
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

  SharedWebSocketPeer m_web_socket_server;

  void InitWebSocketServer();

public:
  NetworkingManager(
      const common::subsystems::SharedSubsystemManager &subsystems);
};

typedef std::shared_ptr<NetworkingManager> SharedNetworkingManager;

} // namespace networking

#endif /* NETWORKINGMANAGER_H */
