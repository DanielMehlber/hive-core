#include <utility>

#include "networking/NetworkingManager.h"

using namespace networking;
using namespace networking::websockets;

NetworkingManager::NetworkingManager(
    const common::subsystems::SharedSubsystemManager &subsystems,
    const common::config::SharedConfiguration &config)
    : m_subsystems(subsystems), m_config(config) {

  // configuration section
  bool auto_init_websocket_server = config->GetAsInt("net.autoInit", true);

  if (auto_init_websocket_server) {
    StartMessageEndpointServer();
  }
}

void NetworkingManager::StartMessageEndpointServer() {
  if (!m_message_endpoint) {

    m_message_endpoint =
        NetworkingFactory::CreateNetworkingPeer(m_subsystems.lock(), m_config);

    m_subsystems.lock()->AddOrReplaceSubsystem(m_message_endpoint);
  }
}