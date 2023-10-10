#ifndef NETWORKINGMANAGER_H
#define NETWORKINGMANAGER_H

#include "NetworkingFactory.h"
#include "jobsystem/JobSystem.h"
#include "properties/PropertyProvider.h"

namespace networking {
class NetworkingManager {
private:
  props::SharedPropertyProvider m_property_provider;
  jobsystem::SharedJobManager m_job_manager;

  SharedWebSocketPeer m_web_socket_server;

  void InitWebSocketServer();

public:
  NetworkingManager(props::SharedPropertyProvider properties,
                    jobsystem::SharedJobManager job_manager);
};
} // namespace networking

#endif /* NETWORKINGMANAGER_H */
