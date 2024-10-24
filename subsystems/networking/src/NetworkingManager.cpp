#include "networking/NetworkingManager.h"
#include "common/uuid/UuidGenerator.h"
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