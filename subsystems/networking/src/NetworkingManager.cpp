#include "networking/NetworkingManager.h"
#include "common/uuid/UuidGenerator.h"
#include "data/DataLayer.h"
#include "logging/LogManager.h"
#include "networking/messaging/MessageConsumerJob.h"
#include <chrono>

using namespace hive::networking;
using namespace hive::networking::messaging;
using namespace std::literals::chrono_literals;

NetworkingManager::NetworkingManager(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems,
    const common::config::SharedConfiguration &config)
    : m_subsystems(subsystems), m_config(config) {

  // configuration section
  ConfigureNode(config);
  bool auto_init_websocket_server = config->GetAsInt("net.autoInit", true);

  if (auto_init_websocket_server) {
    StartDefaultEndpointImplementation();
  }
}

void NetworkingManager::StartDefaultEndpointImplementation() {
  common::memory::Owner<IMessageEndpoint> message_endpoint =
      common::memory::Owner<DefaultMessageEndpointImpl>(m_subsystems, m_config);

  if (!SupportsMessagingProtocol(message_endpoint->GetProtocol())) {
    InstallMessageEndpoint(std::move(message_endpoint), true);
  }
}

void NetworkingManager::ConfigureNode(
    const common::config::SharedConfiguration &config) {
  const std::string node_id =
      config->Get("net.node.id", common::uuid::UuidGenerator::Random());

  const auto property_provider =
      m_subsystems.Borrow()->RequireSubsystem<data::DataLayer>();

  property_provider->Set("net.node.id", node_id);

  LOG_INFO("this node is online and identifies as " << node_id
                                                    << " in the hive")
}

void NetworkingManager::SetupConsumerCleanUpJob() {
  auto this_reference = BorrowFromThis().ToReference();

  /* job detaches itself when this has been destroyed; no clean-up needed */
  auto clean_up_job = std::make_shared<jobsystem::TimerJob>(
      [this_reference](jobsystem::JobContext *context) mutable {
        if (auto maybe_network_manager = this_reference.TryBorrow()) {
          auto network_manager = maybe_network_manager.value();
          network_manager->CleanUpAllConsumers();
          return jobsystem::JobContinuation::REQUEUE;
        }
        return jobsystem::JobContinuation::DISPOSE;
      },
      "network-manager-msg-consumer-clean-up", 1s);

  auto job_manager =
      m_subsystems.Borrow()->RequireSubsystem<jobsystem::JobManager>();
  job_manager->KickJob(clean_up_job);
}

void NetworkingManager::AddMessageConsumer(
    std::weak_ptr<IMessageConsumer> consumer) {
  if (bool is_valid_consumer = !consumer.expired()) {
    SharedMessageConsumer shared_consumer = consumer.lock();
    const auto &consumer_message_type = shared_consumer->GetMessageType();
    std::unique_lock consumers_lock(m_consumers_mutex);
    m_consumers[consumer_message_type].push_back(consumer);
    LOG_DEBUG("added web-socket message consumer for message type '"
              << consumer_message_type << "'")
  } else {
    LOG_WARN("given web-socket message consumer has expired and cannot be "
             "added to the web-socket endpoint")
  }
}

std::list<SharedMessageConsumer>
NetworkingManager::GetConsumersOfMessageType(const std::string &type_name) {
  std::list<SharedMessageConsumer> ret_consumer_list;

  std::unique_lock consumers_lock(m_consumers_mutex);
  if (m_consumers.contains(type_name)) {
    auto &consumer_list = m_consumers[type_name];

    consumers_lock.unlock();
    // remove expired references to consumers
    CleanUpConsumersOfMessageType(type_name);
    consumers_lock.lock();

    // collect all remaining consumers in the list
    for (const auto &consumer : consumer_list) {
      if (!consumer.expired()) {
        ret_consumer_list.push_back(consumer.lock());
      }
    }
  }

  return ret_consumer_list;
}

void NetworkingManager::ProcessMessage(const SharedMessage &message,
                                       const ConnectionInfo &info) {

  DEBUG_ASSERT(m_subsystems.CanBorrow(), "subsystems shut down early")

  auto subsystems = m_subsystems.Borrow();
  auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();

  std::string message_type = message->GetType();
  auto consumer_list = GetConsumersOfMessageType(message_type);

  LOG_DEBUG("received message of type '" << message_type << "' ("
                                         << consumer_list.size()
                                         << " consumers registered)")

  for (const auto &consumer : consumer_list) {
    auto job = std::make_shared<MessageConsumerJob>(consumer, message, info);
    job_manager->KickJob(job);
  }
}

void NetworkingManager::InstallMessageEndpoint(
    common::memory::Owner<IMessageEndpoint> &&endpoint, bool is_default) {

  DEBUG_ASSERT(endpoint.GetState() != nullptr, "endpoint must not be null")
  std::unique_lock lock(m_endpoints_mutex);

  auto protocol = endpoint->GetProtocol();

  // remove old endpoint if it exists
  if (m_endpoints.contains(protocol)) {
    auto former_endpoint = m_endpoints.at(protocol);
    m_endpoints.erase(protocol);
    (*former_endpoint)->Shutdown();
  }

  // startup new endpoint and register it
  endpoint->Startup();
  auto endpoint_ptr = std::make_shared<common::memory::Owner<IMessageEndpoint>>(
      std::move(endpoint));
  m_endpoints[protocol] = endpoint_ptr;

  bool is_only_one_installed = m_endpoints.size() == 1;
  if (is_default || is_only_one_installed) {
    m_default_endpoint_protocol = protocol;
  }

  LOG_INFO("new messaging endpoint for protocol '" << protocol << "' installed")
}

std::optional<hive::common::memory::Borrower<IMessageEndpoint>>
NetworkingManager::GetMessageEndpoint(const std::string &protocol) const {

  DEBUG_ASSERT(!protocol.empty(), "protocol must not be empty")
  std::unique_lock lock(m_endpoints_mutex);

  if (m_endpoints.contains(protocol)) {
    return m_endpoints.at(protocol)->Borrow();
  }

  return {};
}

std::optional<hive::common::memory::Borrower<IMessageEndpoint>>
NetworkingManager::GetDefaultMessageEndpoint() const {
  return GetMessageEndpoint(m_default_endpoint_protocol);
}

hive::common::memory::Borrower<IMessageEndpoint>
NetworkingManager::RequireDefaultMessageEndpoint() const {
  auto maybe_endpoint = GetDefaultMessageEndpoint();
  if (!maybe_endpoint.has_value()) {
    THROW_EXCEPTION(NoEndpointsException,
                    "no messaging endpoints have been installed")
  }

  return maybe_endpoint.value();
}

void NetworkingManager::SetDefaultMessageEndpoint(const std::string &protocol) {
  DEBUG_ASSERT(!protocol.empty(), "can't set empty protocol as default")

  std::unique_lock lock(m_endpoints_mutex);
  if (m_endpoints.contains(protocol)) {
    m_default_endpoint_protocol = protocol;
    LOG_DEBUG("default messaging endpoint set to protocol '" << protocol << "'")
  } else {
    LOG_ERR("no endpoint for protocol '" << protocol
                                         << "' has been installed, so it can't "
                                            "be set at default endpoint")
    THROW_EXCEPTION(
        ProtocolNotSupportedException,
        "no endpoint for protocol '"
            << protocol
            << "' has been installed, so it can't be set at default endpoint")
  }
}

void NetworkingManager::UninstallMessageEndpoint(const std::string &protocol) {
  DEBUG_ASSERT(!protocol.empty(), "protocol must not be empty")

  std::unique_lock lock(m_endpoints_mutex);

  if (!m_endpoints.contains(protocol)) {
    LOG_WARN("protocol '" << protocol
                          << "' has no endpoint implementation to uninstall: "
                             "skipping uninstallation")
    return;
  }

  auto endpoint_ptr = m_endpoints.at(protocol);
  m_endpoints.erase(protocol);
  (*endpoint_ptr)->Shutdown();

  // check if default endpoint has been uninstalled
  if (m_default_endpoint_protocol == protocol) {
    // take first endpoint as new default
    if (!m_endpoints.empty()) {
      const auto it = m_endpoints.begin();
      m_default_endpoint_protocol = it->first;
    } else {
      m_default_endpoint_protocol.clear();
    }
  }
}

bool NetworkingManager::SupportsMessagingProtocol(
    const std::string &protocol) const {
  std::unique_lock lock(m_endpoints_mutex);
  return m_endpoints.contains(protocol);
}

std::vector<std::string> NetworkingManager::GetSupportedProtocols() const {
  std::vector<std::string> protocols;
  std::unique_lock lock(m_endpoints_mutex);

  for (const auto &[protocol, _] : m_endpoints) {
    protocols.push_back(protocol);
  }

  return protocols;
}

bool NetworkingManager::HasInstalledMessageEndpoints() const {
  return !m_endpoints.empty();
}

std::optional<hive::common::memory::Borrower<IMessageEndpoint>>
NetworkingManager::GetSomeMessageEndpointConnectedTo(
    const std::string &endpoint_id) const {
  std::unique_lock lock(m_endpoints_mutex);

  if (m_endpoints.empty()) {
    return {};
  }

  // check default endpoint first
  auto default_endpoint = m_endpoints.at(m_default_endpoint_protocol);
  if ((*default_endpoint)->HasConnectionTo(endpoint_id)) {
    return default_endpoint->Borrow();
  }

  // check other endpoints after
  for (const auto &[protocol, endpoint] : m_endpoints) {
    if (protocol == m_default_endpoint_protocol) {
      continue;
    }

    if ((*endpoint)->HasConnectionTo(endpoint_id)) {
      return endpoint->Borrow();
    }
  }

  return {};
}

void NetworkingManager::CleanUpConsumersOfMessageType(const std::string &type) {
  if (m_consumers.contains(type)) {
    std::unique_lock consumers_lock(m_consumers_mutex);
    auto &consumer_list = m_consumers[type];
    consumer_list.remove_if([](const std::weak_ptr<IMessageConsumer> &con) {
      return con.expired();
    });
  }
}

void NetworkingManager::CleanUpAllConsumers() {
  for (const auto &[message_type, consumer_list] : m_consumers) {
    CleanUpConsumersOfMessageType(message_type);
  }
}