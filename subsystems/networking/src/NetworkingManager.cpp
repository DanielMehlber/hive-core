#include "networking/NetworkingManager.h"
#include "common/uuid/UuidGenerator.h"

using namespace hive::networking;
using namespace hive::networking::messaging;

NetworkingManager::NetworkingManager(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems,
    const common::config::SharedConfiguration &config)
    : m_subsystems(subsystems), m_config(config) {

  // configuration section
  ConfigureNode(config);
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

void NetworkingManager::ConfigureNode(
    const common::config::SharedConfiguration &config) {
  const std::string node_id =
      config->Get("net.node.id", common::uuid::UuidGenerator::Random());

  const auto property_provider =
      m_subsystems.Borrow()->RequireSubsystem<data::PropertyProvider>();

  property_provider->Set("net.node.id", node_id);

  LOG_INFO("this node is online and identifies as " << node_id
                                                    << " in the cluster")
}