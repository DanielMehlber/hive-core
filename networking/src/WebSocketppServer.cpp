#include "networking/websockets/impl/WebSocketppServer.h"
#include "networking/websockets/WebSocketMessageConverter.h"
#include <functional>
#include <logging/Logging.h>

using namespace networking::websockets;
using namespace std::placeholders;

WebSocketppServer::WebSocketppServer(jobsystem::SharedJobManager job_manager,
                                     props::SharedPropertyProvider properties)
    : m_job_manager{job_manager}, m_property_provider{properties} {

  // configuration section
  bool use_tls = properties->GetOrElse<bool>("net.ws.tls.enabled", true);
  size_t port = properties->GetOrElse("net.ws.port", 9000);

  if (use_tls) {
    LOG_WARN("Using TLS in web-socket connections is not implemented yet. "
             "Using unsafe connection.")
  }

  m_server_endpoint.set_open_handler(
      std::bind(&WebSocketppServer::OnConnectionOpened, this, _1));
  m_server_endpoint.set_close_handler(
      std::bind(&WebSocketppServer::OnConnectionClosed, this, _1));
  m_server_endpoint.set_message_handler(
      std::bind(&WebSocketppServer::OnMessageReceived, this, _1, _2));

  m_server_endpoint.init_asio();
  m_server_endpoint.listen(port);
  m_server_endpoint.start_accept();
  m_server_endpoint.run();
}

void WebSocketppServer::AddConsumer(
    std::weak_ptr<IWebSocketMessageConsumer> consumer) {
  if (consumer.expired()) {
    return;
  }

  auto &type = consumer.lock()->GetMessageType();

  // do not only check if there is a consummer, but also if the consumer is
  // still valid (if there is one). Otherwise replace it.
  if (m_consumers.contains(type) && !m_consumers.at(type).expired()) {
    THROW_EXCEPTION(DuplicateConsumerTypeException,
                    "consumer for websocket message type '"
                        << type << "' already exists");
  }

  m_consumers[type] = consumer;
}

std::optional<SharedWebSocketMessageConsumer>
WebSocketppServer::GetConsumerForType(const std::string &type_name) noexcept {
  if (m_consumers.contains(type_name)) {

    // a weak_ptr can expire, i.e. when the referenced object has been destroyed
    // in the mean-time. We have to check for this case as well and remove the
    // invalid weak_ptr accordingly.
    auto weak_consumer_reference = m_consumers.at(type_name);
    if (weak_consumer_reference.expired()) {
      m_consumers.erase(type_name);
      return {};
    } else {
      return m_consumers.at(type_name).lock();
    }
  }

  return {};
}

void WebSocketppServer::OnConnectionOpened(websocketpp::connection_hdl conn) {
  if (conn.expired()) {
    return;
  }

  auto remote_uri =
      m_server_endpoint.get_con_from_hdl(conn)->get_remote_endpoint();
  LOG_INFO("remote host '" << remote_uri << "' connected via a web-socket");

  m_connections[remote_uri] = conn;
}

void WebSocketppServer::OnConnectionClosed(websocketpp::connection_hdl conn) {
  if (conn.expired()) {
    return;
  }

  auto remote_uri =
      m_server_endpoint.get_con_from_hdl(conn)->get_remote_endpoint();

  m_connections.erase(remote_uri);
}

void WebSocketppServer::OnMessageReceived(websocketpp::connection_hdl conn,
                                          server::message_ptr msg) {
  const auto &payload = msg->get_payload();

  // try to parse the received message
  WebSocketMessageConverter converter;
  SharedWebSocketMessage message;
  try {
    message = converter.FromJson(payload);
  } catch (const MessagePayloadInvalidException &ex) {
    LOG_ERR("invalid message payload received via web-sockets: " << ex.what());
    return;
  }

  std::string type = message->GetType();
  std::optional<SharedWebSocketMessageConsumer> optional_consumer =
      GetConsumerForType(type);

  if (optional_consumer.has_value()) {
    SharedWebSocketMessageConsumer consumer = optional_consumer.value();

    // convert to weak_ptr because job is executed in the future at which time
    // the consumer could already be un-registered or deleted.
    std::weak_ptr<IWebSocketMessageConsumer> weak_consumer = consumer;
    SharedJob processing_job = jobsystem::JobSystemFactory::CreateJob(
        [weak_consumer, message, type](jobsystem::JobContext *) {
          if (!weak_consumer.expired()) {
            weak_consumer.lock()->ProcessReceivedMessage(message);
          } else {
            LOG_WARN(
                "web-socket message of type '"
                << type
                << "' received, but consumer has expired. Message ignored.");
          }
          return jobsystem::job::JobContinuation::DISPOSE;
        });

    m_job_manager->KickJob(processing_job);
  } else {
    LOG_WARN("web-socket message of type '"
             << type
             << "' received, but no consumer registered. Message ignored.")
  }
}