#include "networking/messaging/impl/websockets/boost/BoostWebSocketEndpoint.h"

#include "data/DataLayer.h"
#include "events/broker/IEventBroker.h"
#include "logging/LogManager.h"
#include "networking/NetworkingManager.h"
#include "networking/messaging/MessageConsumerJob.h"
#include "networking/messaging/MessageConverter.h"
#include "networking/messaging/events/ConnectionClosedEvent.h"
#include "networking/messaging/events/ConnectionEstablishedEvent.h"
#include "networking/util/UrlParser.h"
#include <regex>
#include <utility>

using namespace hive::jobsystem;
using namespace hive::networking;
using namespace hive::networking::messaging;
using namespace hive::networking::messaging::websockets;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;           // from <boost/asio.hpp>
using tcp = asio::ip::tcp;              // from <boost/asio/ip/tcp.hpp>
using namespace std::chrono_literals;

BoostWebSocketEndpoint::BoostWebSocketEndpoint(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems,
    common::config::SharedConfiguration config)
    : m_subsystems(subsystems), m_config(std::move(config)) {}

void BoostWebSocketEndpoint::Startup() {
  // read configuration
  bool init_server_at_startup = m_config->GetBool("net.server.auto-init", true);
  int thread_count = m_config->GetAsInt("net.threads", 1);
  int local_endpoint_port = m_config->GetAsInt("net.port", 9000);
  std::string local_endpoint_address =
      m_config->Get("net.address", "127.0.0.1");

  m_local_endpoint = std::make_shared<boost::asio::ip::tcp::endpoint>(
      asio::ip::make_address(local_endpoint_address), local_endpoint_port);

  m_execution_context = std::make_shared<boost::asio::io_context>();

  if (init_server_at_startup) {
    InitAndStartConnectionListener();
    std::unique_lock running_lock(m_running_mutex);
    m_running = true;
  }

  // hand threads to boost.asio for handling websocket operations.
  std::weak_ptr<asio::io_context> weak_execution_context = m_execution_context;
  for (size_t i = 0; i < thread_count; i++) {
    std::thread execution([weak_execution_context]() {
      if (auto execution_context = weak_execution_context.lock()) {
        execution_context->run();
      }
    });
    m_execution_threads.push_back(std::move(execution));
  }

  m_this_pointer = std::make_shared<BoostWebSocketEndpoint *>(this);
  SetupCleanUpJob();
}

void BoostWebSocketEndpoint::Shutdown() {
  std::unique_lock running_lock(m_running_mutex);
  m_running = false;
  running_lock.unlock();

  // close all endpoints
  std::unique_lock conn_lock(m_connections_mutex);
  m_connection_listener->ShutDown();
  for (const auto &connection : m_connections) {
    connection.second->Close();
  }
  conn_lock.unlock();

  // wait until all worker threads have returned
  for (auto &execution : m_execution_threads) {
    DEBUG_ASSERT(execution.get_id() != std::this_thread::get_id(),
                 "Execution thread should not join itself")
    execution.join();
  }

  LOG_DEBUG("local web-socket endpoint has been shut down")
}

std::string BoostWebSocketEndpoint::GetProtocol() const { return "ws"; }

void BoostWebSocketEndpoint::SetupCleanUpJob() {
  std::weak_ptr<BoostWebSocketEndpoint *> weak_endpoint = m_this_pointer;
  SharedJob clean_up_job = std::make_shared<TimerJob>(
      [weak_endpoint](jobsystem::JobContext *context) mutable {
        if (auto shared_ptr_to_endpoint = weak_endpoint.lock()) {
          auto endpoint = *shared_ptr_to_endpoint;

          // clean up connections that are not usable anymore
          std::unique_lock lock(endpoint->m_connections_mutex);
          size_t size_before = endpoint->m_connections.size();
          for (auto it = endpoint->m_connections.begin();
               it != endpoint->m_connections.end();
               /* no increment here */) {
            if (!it->second->IsUsable()) {
              endpoint->OnConnectionClose(it->first);
              it = endpoint->m_connections.erase(it);
            } else {
              ++it;
            }
          }

          size_t difference = size_before - endpoint->m_connections.size();
          if (difference > 0) {
            LOG_INFO("cleaned up " << difference
                                   << " unusable or dead connections")
          }

          return REQUEUE;
        } else {
          return DISPOSE;
        }
      },
      "boost-web-socket-endpoints-clean-up-" +
          std::to_string(m_local_endpoint->port()),
      1s, CLEAN_UP);

  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();
    job_manager->KickJob(clean_up_job);
  } else /* if subsystems are not available */ {
    LOG_WARN("cannot setup web-socket connection clean-up job because required "
             "subsystems are not available")
  }
}

BoostWebSocketEndpoint::~BoostWebSocketEndpoint() {
  std::unique_lock running_lock(m_running_mutex);
  if (m_running) {
    BoostWebSocketEndpoint::Shutdown();
  }
}

void BoostWebSocketEndpoint::ProcessReceivedMessage(
    const std::string &data,
    const SharedBoostWebSocketConnection &over_connection) {

  std::unique_lock running_lock(m_running_mutex);
  if (!m_running) {
    return;
  }

  // convert payload into message
  SharedMessage message;
  try {
    message = MessageConverter::FromMultipartFormData(data);
  } catch (const MessagePayloadInvalidException &ex) {
    LOG_WARN("message received from host "
             << over_connection->GetRemoteHostAddress()
             << " contained invalid payload: " << ex.what())
    return;
  }

  if (auto maybe_networking_manager =
          m_subsystems.Borrow()->GetSubsystem<NetworkingManager>()) {
    auto networking_manager = maybe_networking_manager.value();
    networking_manager->ProcessMessage(message, over_connection->GetInfo());
  } else {
    LOG_ERR("message received from host "
            << over_connection->GetRemoteHostAddress()
            << " could not be processed because networking subsystem has "
               "already shut down")
  }
}

void BoostWebSocketEndpoint::AddConnection(
    const ConnectionInfo &connection_info, stream_type &&stream) {
  std::unique_lock running_lock(m_running_mutex);
  if (!m_running) {
    return;
  }

  SharedBoostWebSocketConnection connection =
      std::make_shared<BoostWebSocketConnection>(
          connection_info, std::move(stream),
          std::bind(&BoostWebSocketEndpoint::ProcessReceivedMessage, this,
                    std::placeholders::_1, std::placeholders::_2),
          std::bind(&BoostWebSocketEndpoint::OnConnectionClose, this,
                    std::placeholders::_1));
  connection->StartReceivingMessages();

  std::unique_lock conn_lock(m_connections_mutex);

  // if connection to this node already exists, close old connection
  bool connection_already_exists =
      m_connections.contains(connection_info.endpoint_id);
  if (connection_already_exists) {
    LOG_WARN("established connection with node " << connection_info.endpoint_id
                                                 << " already exists.")
    auto old_connection = m_connections[connection_info.endpoint_id];
    old_connection->Close();
  }

  // register new connection
  m_connections[connection_info.endpoint_id] = connection;

  // fire an event that signals the establishment of a new connection
  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();

    // fire event if event subsystem is found
    if (subsystems->ProvidesSubsystem<events::IEventBroker>()) {
      ConnectionEstablishedEvent event;
      event.SetEndpointId(connection_info.endpoint_id);

      auto event_subsystem =
          subsystems->RequireSubsystem<events::IEventBroker>();
      event_subsystem->FireEvent(event.GetEvent());
    }
  }
}

std::optional<SharedBoostWebSocketConnection>
BoostWebSocketEndpoint::GetConnection(const std::string &connection_id) {
  std::unique_lock lock(m_connections_mutex);
  if (m_connections.contains(connection_id)) {
    auto connection = m_connections.at(connection_id);
    if (connection->IsUsable()) {
      return connection;
    } else {
      return {};
    }
  } else {
    return {};
  }
}

void BoostWebSocketEndpoint::InitAndStartConnectionListener() {

  // get id of this node (required for handshake)
  auto property_provider =
      m_subsystems.Borrow()->RequireSubsystem<data::DataLayer>();
  auto node_uuid = property_provider->Get("net.node.id").get().value_or("");
  /**
   * Consumers are stored as expireable weak-pointers. When the actual
   * referenced consumer is destroyed, the list of consumers holds expired
   * pointers that can be removed.
   * @param type message type name of consumers to clean up
   */
  void CleanUpConsumersOfMessageType(const std::string &type);
  if (!m_connection_listener) {
    m_connection_listener = std::make_shared<BoostWebSocketConnectionListener>(
        node_uuid, m_execution_context, m_config, m_local_endpoint,
        std::bind(&BoostWebSocketEndpoint::AddConnection, this,
                  std::placeholders::_1, std::placeholders::_2));
    m_connection_listener->Init();
    m_connection_listener->StartAcceptingAnotherConnection();
  }
}

void BoostWebSocketEndpoint::InitConnectionEstablisher() {

  // get id of this node (required for handshake)
  auto property_provider =
      m_subsystems.Borrow()->RequireSubsystem<data::DataLayer>();
  auto node_uuid = property_provider->Get("net.node.id").get().value_or("");

  if (!m_connection_establisher) {
    m_connection_establisher =
        std::make_shared<BoostWebSocketConnectionEstablisher>(
            node_uuid, m_execution_context,
            std::bind(&BoostWebSocketEndpoint::AddConnection, this,
                      std::placeholders::_1, std::placeholders::_2));
  }
}

std::future<void> BoostWebSocketEndpoint::Send(const std::string &node_id,
                                               SharedMessage message) {
  auto maybe_connection = GetConnection(node_id);

  bool no_connection_to_node_exists = !maybe_connection.has_value();
  if (no_connection_to_node_exists) {
    THROW_EXCEPTION(NoSuchEndpointException,
                    "node " << node_id << " does not exist")
  }

  return maybe_connection.value()->Send(message);
}

std::future<ConnectionInfo>
BoostWebSocketEndpoint::EstablishConnectionTo(const std::string &uri) {
  // check if connection establishment component has been initialized.
  if (!m_connection_establisher) {
    InitConnectionEstablisher();
  }

  return m_connection_establisher->EstablishConnectionTo(uri);
}

void BoostWebSocketEndpoint::CloseConnectionTo(const std::string &node_id) {
  std::unique_lock lock(m_connections_mutex);
  if (m_connections.contains(node_id)) {
    m_connections.at(node_id)->Close();
    m_connections.erase(node_id);
  }
}

bool BoostWebSocketEndpoint::HasConnectionTo(const std::string &node_id) const {
  std::unique_lock lock(m_connections_mutex);
  if (m_connections.contains(node_id)) {
    return m_connections.at(node_id)->IsUsable();
  }

  return false;
}

std::future<size_t>
BoostWebSocketEndpoint::IssueBroadcastAsJob(const SharedMessage &message) {

  DEBUG_ASSERT(!message->GetId().empty(), "message id should not be empty")
  DEBUG_ASSERT(!message->GetType().empty(), "message type should not be empty")

  std::shared_ptr<std::promise<size_t>> promise =
      std::make_shared<std::promise<size_t>>();
  std::future<size_t> future = promise->get_future();

  SharedJob job = std::make_shared<Job>(
      [_this = BorrowFromThis(), message,
       promise](jobsystem::JobContext *context) {
        std::list<std::future<void>> futures;
        size_t count{};

        // first fire message using all available connections
        std::unique_lock lock(_this->m_connections_mutex);
        for (auto &connection_tuple : _this->m_connections) {
          auto connection = connection_tuple.second;
          if (connection->IsUsable()) {
            auto sending_progress = connection->Send(message);
            futures.push_back(std::move(sending_progress));
          } else {
            LOG_WARN("web-socket connection to remote endpoint "
                     << connection->GetRemoteHostAddress()
                     << " is not usable or broken. Skipped for broadcasting.")
          }
        }

        // wait for messages to be sent
        lock.unlock();
        for (auto &sending_progress : futures) {
          context->GetJobManager()->WaitForCompletion(sending_progress);
          count++;
          try {
            sending_progress.get();
          } catch (const std::exception &ex) {
            LOG_ERR("failed to broadcast message: " << ex.what())
            count--;
          }
        }

        promise->set_value(count);
        return JobContinuation::DISPOSE;
      },
      "broadcast-web-socket-message-" + message->GetId());

  auto subsystems = m_subsystems.Borrow();
  auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();
  job_manager->KickJob(job);
  return future;
}

size_t BoostWebSocketEndpoint::GetActiveConnectionCount() const {
  std::unique_lock lock(m_connections_mutex);
  size_t count = 0;
  for (auto &connection : m_connections) {
    if (connection.second->IsUsable()) {
      count++;
    }
  }

  return count;
}

void BoostWebSocketEndpoint::OnConnectionClose(const std::string &id) {
  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();

    // trigger event notifying other parties that a connection has closed
    if (subsystems->ProvidesSubsystem<events::IEventBroker>()) {
      auto event_broker = subsystems->RequireSubsystem<events::IEventBroker>();

      ConnectionClosedEvent event;
      event.SetEndpointId(id);
      event_broker->FireEvent(event.GetEvent());
    }
  }
}
