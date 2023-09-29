#ifndef NETWORKINGMANAGER_H
#define NETWORKINGMANAGER_H

#include "NetworkingConfig.h"
#include "NetworkingFactory.h"
#include "jobsystem/JobSystem.h"

namespace networking {
class NetworkingManager {
private:
  NetworkingConfig m_config;

  SharedWebSocketServer m_web_socket_server;
  jobsystem::SharedJobManager m_job_manager;

  void InitWebSocketServer();

public:
};
} // namespace networking

#endif /* NETWORKINGMANAGER_H */
