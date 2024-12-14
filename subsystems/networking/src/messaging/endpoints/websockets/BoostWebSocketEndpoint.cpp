#include "networking/messaging/endpoints/websockets/BoostWebSocketEndpoint.h"

#include "data/DataLayer.h"
#include "events/broker/IEventBroker.h"
#include "jobsystem/jobs/TimerJob.h"
#include "logging/LogManager.h"
#include "networking/NetworkingManager.h"
#include "networking/messaging/converter/MultipartMessageConverter.h"
#include "networking/messaging/events/ConnectionClosedEvent.h"
#include "networking/messaging/events/ConnectionEstablishedEvent.h"

#include "networking/messaging/endpoints/websockets/BoostWebSocketConnection.h"
#include "networking/messaging/endpoints/websockets/BoostWebSocketConnectionEstablisher.h"
#include "networking/messaging/endpoints/websockets/BoostWebSocketConnectionListener.h"

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

struct BoostWebSocketEndpoint::Impl {
  bool running{false};
  mutable recursive_mutex running_mutex;

  common::memory::Reference<common::subsystems::SubsystemManager> subsystems;
  common::config::SharedConfiguration config;

  /** Necessary for clean-up jobs because they are kicked in the constructor,
   * where shared_from_this() is not available yet. */
  std::shared_ptr<BoostWebSocketEndpoint *> this_pointer;

  /** Acts as execution environment for asynchronous operations, such as
   * receiving events */
  std::shared_ptr<asio::io_context> execution_context;

  /** Maps host addresses to the connection established with the host. */
  std::map<std::string, SharedBoostWebSocketConnection> connections;
  mutable recursive_mutex connections_mutex;

  std::shared_ptr<BoostWebSocketConnectionEstablisher> connection_establisher;
  std::shared_ptr<BoostWebSocketConnectionListener> connection_listener;

  /** Thread pool that handles asynchronous callbacks (required by ASIO) */
  std::vector<std::thread> execution_threads;

  /** This is the local endpoint over which the endpoint should communicate */
  std::shared_ptr<asio::ip::tcp::endpoint> local_endpoint;

  /**
   * Constructs a new connection object from a stream
   * @param connection_info connection specification
   * @param stream web-socket stream
   * registered and ready to use.
   */
  void AddConnection(const ConnectionInfo &connection_info,
                     stream_type &&stream);

  std::optional<SharedBoostWebSocketConnection>
  GetConnection(const std::string &connection_id);

  void
  ProcessReceivedMessage(const std::string &data,
                         const SharedBoostWebSocketConnection &over_connection);

  void OnConnectionClose(const std::string &id);
};

BoostWebSocketEndpoint::BoostWebSocketEndpoint(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems,
    const common::config::SharedConfiguration &config)
    : m_impl(std::make_unique<Impl>()) {
  m_impl->config = config;
  m_impl->subsystems = subsystems;
}

void BoostWebSocketEndpoint::Startup() {
  // read configuration

  const auto &config = m_impl->config;

  bool init_server_at_startup = config->GetBool("net.server.auto-init", true);
  int thread_count = config->GetAsInt("net.threads", 1);
  int local_endpoint_port = config->GetAsInt("net.port", 9000);
  std::string local_endpoint_address = config->Get("net.address", "127.0.0.1");

  m_impl->local_endpoint = std::make_shared<boost::asio::ip::tcp::endpoint>(
      asio::ip::make_address(local_endpoint_address), local_endpoint_port);

  m_impl->execution_context = std::make_shared<boost::asio::io_context>();

  if (init_server_at_startup) {
    InitAndStartConnectionListener();
    std::unique_lock running_lock(m_impl->running_mutex);
    m_impl->running = true;
  }

  // hand threads to boost.asio for handling websocket operations.
  std::weak_ptr weak_execution_context = m_impl->execution_context;
  for (size_t i = 0; i < thread_count; i++) {
    std::thread execution([weak_execution_context]() {
      if (auto execution_context = weak_execution_context.lock()) {
        execution_context->run();
      }
    });
    m_impl->execution_threads.push_back(std::move(execution));
  }

  m_impl->this_pointer = std::make_shared<BoostWebSocketEndpoint *>(this);
  SetupCleanUpJob();
}

void BoostWebSocketEndpoint::Shutdown() {
  std::unique_lock running_lock(m_impl->running_mutex);
  m_impl->running = false;
  running_lock.unlock();

  const auto local_host = m_impl->local_endpoint->address().to_string() + ":" +
                          std::to_string(m_impl->local_endpoint->port());

  // close all endpoints
  std::unique_lock conn_lock(m_impl->connections_mutex);
  m_impl->connection_listener->ShutDown();
  for (const auto &connection : m_impl->connections) {
    connection.second->Close();
  }
  conn_lock.unlock();

  // wait until all worker threads have returned
  for (auto &execution : m_impl->execution_threads) {
    DEBUG_ASSERT(execution.get_id() != std::this_thread::get_id(),
                 "Execution thread should not join itself")
    execution.join();
  }

  LOG_INFO("web-socket endpoint at " << local_host << " has been shut down")
}

std::string BoostWebSocketEndpoint::GetProtocol() const { return "ws"; }

void BoostWebSocketEndpoint::SetupCleanUpJob() {
  std::weak_ptr<BoostWebSocketEndpoint *> weak_endpoint = m_impl->this_pointer;
  SharedJob clean_up_job = std::make_shared<TimerJob>(
      [weak_endpoint](jobsystem::JobContext *context) mutable {
        if (auto shared_ptr_to_endpoint = weak_endpoint.lock()) {
          auto endpoint = *shared_ptr_to_endpoint;

          // clean up connections that are not usable anymore
          std::unique_lock lock(endpoint->m_impl->connections_mutex);
          size_t size_before = endpoint->m_impl->connections.size();
          for (auto it = endpoint->m_impl->connections.begin();
               it != endpoint->m_impl->connections.end();
               /* no increment here */) {
            if (!it->second->IsUsable()) {
              endpoint->m_impl->OnConnectionClose(it->first);
              it = endpoint->m_impl->connections.erase(it);
            } else {
              ++it;
            }
          }

          size_t difference =
              size_before - endpoint->m_impl->connections.size();
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
          std::to_string(m_impl->local_endpoint->port()),
      1s, CLEAN_UP);

  if (auto maybe_subsystems = m_impl->subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();
    job_manager->KickJob(clean_up_job);
  } else /* if subsystems are not available */ {
    LOG_WARN("cannot setup web-socket connection clean-up job because required "
             "subsystems are not available")
  }
}

BoostWebSocketEndpoint::~BoostWebSocketEndpoint() {
  std::unique_lock running_lock(m_impl->running_mutex);
  if (m_impl->running) {
    BoostWebSocketEndpoint::Shutdown();
  }
}

void BoostWebSocketEndpoint::Impl::ProcessReceivedMessage(
    const std::string &data,
    const SharedBoostWebSocketConnection &over_connection) {

  std::unique_lock running_lock(running_mutex);
  if (!running) {
    return;
  }

  // convert payload into message
  SharedMessage message;
  try {
    // TODO: support multiple message types
    // this also requires sending the MIME type with each web-socket message
    // (headers?)
    MultipartMessageConverter converter;
    message = converter.Deserialize(data);
  } catch (const MessagePayloadInvalidException &ex) {
    LOG_WARN("message received from host "
             << over_connection->GetRemoteHostAddress()
             << " contained invalid payload: " << ex.what())
    return;
  }

  if (auto maybe_networking_manager =
          subsystems.Borrow()->GetSubsystem<NetworkingManager>()) {
    auto networking_manager = maybe_networking_manager.value();
    networking_manager->ProcessMessage(message, over_connection->GetInfo());
  } else {
    LOG_ERR("message received from host "
            << over_connection->GetRemoteHostAddress()
            << " could not be processed because networking subsystem has "
               "already shut down")
  }
}

void BoostWebSocketEndpoint::Impl::AddConnection(
    const ConnectionInfo &connection_info, stream_type &&stream) {
  std::unique_lock running_lock(running_mutex);
  if (!running) {
    return;
  }

  SharedBoostWebSocketConnection connection =
      std::make_shared<BoostWebSocketConnection>(
          connection_info, std::move(stream),
          std::bind(&BoostWebSocketEndpoint::Impl::ProcessReceivedMessage, this,
                    std::placeholders::_1, std::placeholders::_2),
          std::bind(&BoostWebSocketEndpoint::Impl::OnConnectionClose, this,
                    std::placeholders::_1));
  connection->StartReceivingMessages();

  std::unique_lock conn_lock(connections_mutex);

  // if connection to this node already exists, close old connection
  bool connection_already_exists =
      connections.contains(connection_info.remote_endpoint_id);
  if (connection_already_exists) {
    LOG_WARN("established connection with node "
             << connection_info.remote_endpoint_id << " already exists.")
    auto old_connection = connections[connection_info.remote_endpoint_id];
    old_connection->Close();
  }

  // register new connection
  connections[connection_info.remote_endpoint_id] = connection;

  // fire an event that signals the establishment of a new connection
  if (auto maybe_subsystems = subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();

    // fire event if event subsystem is found
    if (subsystems->ProvidesSubsystem<events::IEventBroker>()) {
      ConnectionEstablishedEvent event;
      event.SetEndpointId(connection_info.remote_endpoint_id);

      auto event_subsystem =
          subsystems->RequireSubsystem<events::IEventBroker>();
      event_subsystem->FireEvent(event.GetEvent());
    }
  }
}

std::optional<SharedBoostWebSocketConnection>
BoostWebSocketEndpoint::Impl::GetConnection(const std::string &connection_id) {
  std::unique_lock lock(connections_mutex);
  if (connections.contains(connection_id)) {
    auto connection = connections.at(connection_id);
    if (connection->IsUsable()) {
      return connection;
    }
    return {};
  }
  return {};
}

void BoostWebSocketEndpoint::InitAndStartConnectionListener() {

  // get id of this node (required for handshake)
  auto property_provider =
      m_impl->subsystems.Borrow()->RequireSubsystem<data::DataLayer>();
  auto node_uuid = property_provider->Get("net.node.id").get().value_or("");
  /**
   * Consumers are stored as expireable weak-pointers. When the actual
   * referenced consumer is destroyed, the list of consumers holds expired
   * pointers that can be removed.
   * @param type message type name of consumers to clean up
   */
  void CleanUpConsumersOfMessageType(const std::string &type);
  if (!m_impl->connection_listener) {
    m_impl->connection_listener =
        std::make_shared<BoostWebSocketConnectionListener>(
            node_uuid, m_impl->execution_context, m_impl->config,
            m_impl->local_endpoint,
            std::bind(&Impl::AddConnection, m_impl.get(), std::placeholders::_1,
                      std::placeholders::_2));
    m_impl->connection_listener->Init();
    m_impl->connection_listener->StartAcceptingAnotherConnection();
  }
}

void BoostWebSocketEndpoint::InitConnectionEstablisher() {

  // get id of this node (required for handshake)
  auto property_provider =
      m_impl->subsystems.Borrow()->RequireSubsystem<data::DataLayer>();
  auto node_uuid = property_provider->Get("net.node.id").get().value_or("");

  if (!m_impl->connection_establisher) {
    m_impl->connection_establisher =
        std::make_shared<BoostWebSocketConnectionEstablisher>(
            node_uuid, m_impl->execution_context,
            std::bind(&Impl::AddConnection, m_impl.get(), std::placeholders::_1,
                      std::placeholders::_2));
  }
}

std::future<void> BoostWebSocketEndpoint::Send(const std::string &node_id,
                                               SharedMessage message) {
  auto maybe_connection = m_impl->GetConnection(node_id);

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
  if (!m_impl->connection_establisher) {
    InitConnectionEstablisher();
  }

  return m_impl->connection_establisher->EstablishConnectionTo(uri);
}

void BoostWebSocketEndpoint::CloseConnectionTo(const std::string &node_id) {
  std::unique_lock lock(m_impl->connections_mutex);
  if (m_impl->connections.contains(node_id)) {
    m_impl->connections.at(node_id)->Close();
    m_impl->connections.erase(node_id);
  }
}

bool BoostWebSocketEndpoint::HasConnectionTo(const std::string &node_id) const {
  std::unique_lock lock(m_impl->connections_mutex);
  if (m_impl->connections.contains(node_id)) {
    return m_impl->connections.at(node_id)->IsUsable();
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
        std::unique_lock lock(_this->m_impl->connections_mutex);
        for (auto &connection_tuple : _this->m_impl->connections) {
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
          context->GetJobManager()->Await(sending_progress);
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

  auto subsystems = m_impl->subsystems.Borrow();
  auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();
  job_manager->KickJob(job);
  return future;
}

size_t BoostWebSocketEndpoint::GetActiveConnectionCount() const {
  std::unique_lock lock(m_impl->connections_mutex);
  size_t count = 0;
  for (auto &connection : m_impl->connections) {
    if (connection.second->IsUsable()) {
      count++;
    }
  }

  return count;
}

void BoostWebSocketEndpoint::Impl::OnConnectionClose(const std::string &id) {
  if (auto maybe_subsystems = subsystems.TryBorrow()) {
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
