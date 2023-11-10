#include <utility>

#include "networking/NetworkingManager.h"

using namespace networking;
using namespace networking::websockets;

NetworkingManager::NetworkingManager(
    const common::subsystems::SharedSubsystemManager &subsystems)
    : m_subsystems(subsystems) {

  auto property_provider =
      m_subsystems.lock()->RequireSubsystem<props::PropertyProvider>();

  // configuration section
  bool auto_init_websocket_server =
      property_provider->GetOrElse<bool>("net.ws.autoInit", true);

  if (auto_init_websocket_server) {
    InitWebSocketServer();
  }
}

void NetworkingManager::InitWebSocketServer() {
  if (!m_web_socket_server) {
    auto property_provider =
        m_subsystems.lock()->RequireSubsystem<props::PropertyProvider>();
    auto job_manager =
        m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();
    m_web_socket_server =
        NetworkingFactory::CreateWebSocketPeer(m_subsystems.lock());

    m_subsystems.lock()->AddOrReplaceSubsystem(m_web_socket_server);
  }
}