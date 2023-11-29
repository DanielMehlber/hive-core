#include "networking/websockets/impl/boost/BoostWebSocketPeer.h"
#include "networking/util/UrlParser.h"
#include "networking/websockets/PeerMessageConsumerJob.h"
#include "networking/websockets/PeerMessageConverter.h"
#include <regex>
#include <utility>

using namespace networking;
using namespace networking::websockets;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;           // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace std::chrono_literals;

BoostWebSocketPeer::BoostWebSocketPeer(
    const common::subsystems::SharedSubsystemManager &subsystems)
    : m_subsystems(subsystems) {

  auto properties =
      m_subsystems.lock()->RequireSubsystem<props::PropertyProvider>();

  bool init_server_at_startup =
      properties->GetOrElse("net.ws.server.auto-init", true);
  auto thread_count = properties->GetOrElse<size_t>("net.ws.threads", 1);
  auto local_endpont_port = properties->GetOrElse<size_t>("net.ws.port", 9000);
  auto local_endpoint_address =
      properties->GetOrElse<std::string>("net.ws.address", "127.0.0.1");

  m_local_endpoint = std::make_shared<boost::asio::ip::tcp::endpoint>(
      asio::ip::make_address(local_endpoint_address), local_endpont_port);

  m_execution_context = std::make_shared<boost::asio::io_context>();

  if (init_server_at_startup) {
    InitAndStartConnectionListener();
    std::unique_lock running_lock(m_running_mutex);
    m_running = true;
  }

  for (size_t i = 0; i < thread_count; i++) {
    std::thread execution([this]() { m_execution_context->run(); });
    m_execution_threads.push_back(std::move(execution));
  }

  SetupCleanUpJob();
}

void BoostWebSocketPeer::SetupCleanUpJob() {
  SharedJob clean_up_job = jobsystem::JobSystemFactory::CreateJob<TimerJob>(
      [weak_this = weak_from_this()](jobsystem::JobContext *context) {
        if (weak_this.expired()) {
          return DISPOSE;
        }

        // clean up consumers that are not referencing any valid ones
        auto _this = weak_this.lock();
        for (auto &consumer : _this->m_consumers) {
          _this->CleanUpConsumersOf(consumer.first);
        }

        // clean up connections that are not usable anymore
        std::unique_lock lock(_this->m_connections_mutex);
        size_t size_before = _this->m_connections.size();
        for (auto it = _this->m_connections.begin();
             it != _this->m_connections.end();
             /* no increment here */) {
          if (it->second->IsUsable()) {
            it = _this->m_connections.erase(it);
          } else {
            ++it;
          }
        }

        size_t difference = size_before - _this->m_connections.size();
        if (difference > 0) {
          LOG_INFO("cleaned up " << difference
                                 << " unusable or dead connections")
        }

        return REQUEUE;
      },
      "boost-web-socket-peer-clean-up-" +
          std::to_string(m_local_endpoint->port()),
      5s, CLEAN_UP);

  auto job_manager =
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();
  job_manager->KickJob(clean_up_job);
}

BoostWebSocketPeer::~BoostWebSocketPeer() {
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
    execution.join();
  }

  LOG_DEBUG("local web-socket peer has been shut down")
}

void BoostWebSocketPeer::AddConsumer(
    std::weak_ptr<IPeerMessageConsumer> consumer) {
  bool is_valid_consumer = !consumer.expired();
  if (is_valid_consumer) {
    SharedWebSocketMessageConsumer shared_consumer = consumer.lock();
    const auto &consumer_message_type = shared_consumer->GetMessageType();
    std::unique_lock consumers_lock(m_consumers_mutex);
    m_consumers[consumer_message_type].push_back(consumer);
    LOG_DEBUG("added web-socket message consumer for message type '"
              << consumer_message_type << "'")
  } else {
    LOG_WARN("given web-socket message consumer has expired and cannot be "
             "added to the web-socket peer")
  }
}

std::list<SharedWebSocketMessageConsumer>
BoostWebSocketPeer::GetConsumersOfType(const std::string &type_name) noexcept {
  std::list<SharedWebSocketMessageConsumer> ret_consumer_list;

  std::unique_lock consumers_lock(m_consumers_mutex);
  if (m_consumers.contains(type_name)) {
    auto &consumer_list = m_consumers[type_name];

    consumers_lock.unlock();
    // remove expired references to consumers
    CleanUpConsumersOf(type_name);
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

void BoostWebSocketPeer::CleanUpConsumersOf(const std::string &type) noexcept {
  if (m_consumers.contains(type)) {
    std::unique_lock consumers_lock(m_consumers_mutex);
    auto &consumer_list = m_consumers[type];
    consumer_list.remove_if([](const std::weak_ptr<IPeerMessageConsumer> &con) {
      return con.expired();
    });
  }
}

void BoostWebSocketPeer::ProcessReceivedMessage(
    std::string data, SharedBoostWebSocketConnection over_connection) {

  if (m_subsystems.expired()) {
    LOG_ERR("Received message but subsystems have shut down. Cannot process "
            "message.");
    return;
  }

  auto job_manager =
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();

  std::unique_lock running_lock(m_running_mutex);
  if (!m_running) {
    return;
  }

  // convert payload into message
  SharedWebSocketMessage message;
  try {
    message = networking::websockets::PeerMessageConverter::FromJson(data);
  } catch (const MessagePayloadInvalidException &ex) {
    LOG_WARN("message received from host "
             << over_connection->GetRemoteHostAddress()
             << " contained invalid payload")
    return;
  }

  std::string message_type = message->GetType();
  auto consumer_list = GetConsumersOfType(message_type);
  for (auto consumer : consumer_list) {
    auto job = std::make_shared<PeerMessageConsumerJob>(
        consumer, message, over_connection->GetConnectionInfo());
    job_manager->KickJob(job);
  }
}

std::optional<std::string> connectionIdFromUrl(const std::string &url) {
  auto opt_parsed_url = util::UrlParser::parse(url);
  if (opt_parsed_url.has_value()) {
    auto parsed_url = opt_parsed_url.value();
    std::stringstream ss;
    ss << parsed_url.host << ":" << parsed_url.port << parsed_url.path;
    return ss.str();
  } else {
    return {};
  }
}

void BoostWebSocketPeer::AddConnection(std::string url, stream_type &&stream) {
  std::unique_lock running_lock(m_running_mutex);
  if (!m_running) {
    return;
  }

  SharedBoostWebSocketConnection connection =
      std::make_shared<BoostWebSocketConnection>(
          std::move(stream),
          std::bind(&BoostWebSocketPeer::ProcessReceivedMessage, this,
                    std::placeholders::_1, std::placeholders::_2));
  connection->StartReceivingMessages();

  auto connection_id = connectionIdFromUrl(url).value();

  std::unique_lock conn_lock(m_connections_mutex);
  m_connections[connection_id] = connection;
}

std::optional<SharedBoostWebSocketConnection>
networking::websockets::BoostWebSocketPeer::GetConnection(
    const std::string &uri) {

  const std::string connection_id = connectionIdFromUrl(uri).value();
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

void BoostWebSocketPeer::InitAndStartConnectionListener() {
  if (!m_connection_listener) {
    auto property_provider =
        m_subsystems.lock()->RequireSubsystem<props::PropertyProvider>();
    m_connection_listener = std::make_shared<BoostWebSocketConnectionListener>(
        m_execution_context, property_provider, m_local_endpoint,
        std::bind(&BoostWebSocketPeer::AddConnection, this,
                  std::placeholders::_1, std::placeholders::_2));
    m_connection_listener->Init();
    m_connection_listener->StartAcceptingAnotherConnection();
  }
}

void BoostWebSocketPeer::InitConnectionEstablisher() {
  if (!m_connection_establisher) {
    auto property_provider =
        m_subsystems.lock()->RequireSubsystem<props::PropertyProvider>();
    m_connection_establisher =
        std::make_shared<BoostWebSocketConnectionEstablisher>(
            m_execution_context, property_provider,
            std::bind(&BoostWebSocketPeer::AddConnection, this,
                      std::placeholders::_1, std::placeholders::_2));
  }
}

std::future<void> BoostWebSocketPeer::Send(const std::string &uri,
                                           SharedWebSocketMessage message) {
  auto opt_connection = GetConnection(uri);
  if (opt_connection.has_value()) {
    return opt_connection.value()->Send(message);
  } else {
    THROW_EXCEPTION(NoSuchPeerException, "peer " << uri << " does not exist")
  }
}

std::future<void>
BoostWebSocketPeer::EstablishConnectionTo(const std::string &uri) noexcept {

  auto opt_connection = GetConnection(uri);

  // if connection has already been established
  if (opt_connection.has_value()) {
    std::promise<void> prom;
    prom.set_value();
    return prom.get_future();
  }

  // check if connection establishment component has been initlialized.
  if (!m_connection_establisher) {
    InitConnectionEstablisher();
  }

  return m_connection_establisher->EstablishConnectionTo(uri);
}

void BoostWebSocketPeer::CloseConnectionTo(const std::string &uri) noexcept {
  auto opt_connection_id = connectionIdFromUrl(uri);
  if (opt_connection_id.has_value()) {
    auto connection_id = opt_connection_id.value();
    std::unique_lock lock(m_connections_mutex);
    if (m_connections.contains(connection_id)) {
      m_connections.at(connection_id)->Close();
      m_connections.erase(connection_id);
    }
  }
}

bool BoostWebSocketPeer::HasConnectionTo(
    const std::string &uri) const noexcept {
  std::unique_lock lock(m_connections_mutex);
  if (m_connections.contains(uri)) {
    return m_connections.at(uri)->IsUsable();
  } else {
    return false;
  }
}

std::future<size_t>
BoostWebSocketPeer::Broadcast(const SharedWebSocketMessage &message) {

  std::shared_ptr<std::promise<size_t>> promise =
      std::make_shared<std::promise<size_t>>();
  std::future<size_t> future = promise->get_future();

  SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [_this = shared_from_this(), message,
       promise](jobsystem::JobContext *context) {
        std::list<std::future<void>> futures;
        size_t count{};

        // first fire message using all available connections
        std::unique_lock lock(_this->m_connections_mutex);
        for (auto &connection_tuple : _this->m_connections) {
          auto connection = connection_tuple.second;
          if (connection->IsUsable()) {
            auto future = connection->Send(message);
            futures.push_back(std::move(future));
          }
        }

        // wait for messages to be sent
        lock.unlock();
        for (auto &future : futures) {
          context->GetJobManager()->WaitForCompletion(future);
          count++;
          try {
            future.get();
          } catch (std::exception &exception) {
            count--;
          }
        }

        promise->set_value(count);
        return JobContinuation::DISPOSE;
      });

  auto job_manager =
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();
  job_manager->KickJob(job);
  return future;
}

size_t BoostWebSocketPeer::GetActiveConnectionCount() const {
  std::unique_lock lock(m_connections_mutex);
  size_t count = 0;
  for (auto &connection : m_connections) {
    if (connection.second->IsUsable()) {
      count++;
    }
  }

  return count;
};