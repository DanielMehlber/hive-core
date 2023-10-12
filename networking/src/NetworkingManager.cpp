#include <utility>

#include "networking/NetworkingManager.h"

using namespace networking;
using namespace networking::websockets;

NetworkingManager::NetworkingManager(props::SharedPropertyProvider properties,
                                     jobsystem::SharedJobManager job_manager)
    : m_job_manager{std::move(job_manager)},
      m_property_provider{std::move(properties)} {

  // configuration section
  bool auto_init_websocket_server =
      m_property_provider->GetOrElse<bool>("net.ws.autoInit", true);

  if (auto_init_websocket_server) {
    InitWebSocketServer();
  }
}

void NetworkingManager::InitWebSocketServer() {
  if (!m_web_socket_server) {
    m_web_socket_server = NetworkingFactory::CreateWebSocketPeer(
        m_job_manager, m_property_provider);
  }
}