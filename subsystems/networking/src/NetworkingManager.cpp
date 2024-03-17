#include "networking/NetworkingManager.h"

using namespace networking;
using namespace networking::messaging;

NetworkingManager::NetworkingManager(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems,
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

    common::memory::Owner<IMessageEndpoint> message_endpoint =
        common::memory::Owner<DefaultMessageEndpointImpl>(m_subsystems,
                                                          m_config);

    m_message_endpoint = message_endpoint.CreateReference();

    m_subsystems.Borrow()->AddOrReplaceSubsystem<IMessageEndpoint>(
        std::move(message_endpoint));
  }
}