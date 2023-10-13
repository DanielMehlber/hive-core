#ifndef NETWORKINGMANAGER_H
#define NETWORKINGMANAGER_H

#include "NetworkingFactory.h"
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
  props::SharedPropertyProvider m_property_provider;
  jobsystem::SharedJobManager m_job_manager;

  SharedWebSocketPeer m_web_socket_server;

  void InitWebSocketServer();

public:
  NetworkingManager(props::SharedPropertyProvider properties,
                    jobsystem::SharedJobManager job_manager);
};

typedef std::shared_ptr<NetworkingManager> SharedNetworkingManager;

} // namespace networking

#endif /* NETWORKINGMANAGER_H */
