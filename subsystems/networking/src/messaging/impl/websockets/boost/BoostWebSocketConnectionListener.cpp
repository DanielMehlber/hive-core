#include <utility>

#include "networking/messaging/impl/websockets/boost/BoostWebSocketConnectionListener.h"

using namespace hive::networking;
using namespace hive::networking::messaging;
using namespace hive::networking::messaging::websockets;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;           // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

BoostWebSocketConnectionListener::BoostWebSocketConnectionListener(
    std::string this_node_uuid,
    std::shared_ptr<boost::asio::io_context> execution_context,
    common::config::SharedConfiguration config,
    std::shared_ptr<boost::asio::ip::tcp::endpoint> local_endpoint,
    std::function<void(ConnectionInfo, stream_type &&)> connection_consumer)
    : m_connection_consumer{std::move(connection_consumer)},
      m_execution_context{std::move(execution_context)},
      m_config{std::move(config)}, m_local_endpoint{std::move(local_endpoint)},
      m_this_node_uuid(std::move(this_node_uuid)) {}

BoostWebSocketConnectionListener::~BoostWebSocketConnectionListener() {
  ShutDown();
}

void BoostWebSocketConnectionListener::Init() {
  // load configurations
  bool should_use_tls = m_config->GetBool("net.tls.enabled", true);

  if (should_use_tls) {
    // TODO: implement TLS for web-sockets
    LOG_WARN(
        "TLS in web-sockets is not implemented yet and is therefore not used")
  }

  // setup acceptor which listens for incoming connections asynchronously
  m_incoming_tcp_connection_acceptor =
      std::make_unique<tcp::acceptor>(*m_execution_context);

  beast::error_code error_code;

  // open the acceptor for incoming connections
  error_code = m_incoming_tcp_connection_acceptor->open(
      m_local_endpoint->protocol(), error_code);
  if (error_code) {
    LOG_ERR("cannot setup acceptor for incoming web-socket connections: "
            << error_code.message())
    THROW_EXCEPTION(
        WebSocketTcpServerException,
        "cannot setup acceptor for incoming web-socket connections: "
            << error_code.message())
  }

  error_code = m_incoming_tcp_connection_acceptor->set_option(
      asio::socket_base::reuse_address(true), error_code);
  if (error_code) {
    LOG_ERR("cannot set option for accepting incoming web-socket connections: "
            << error_code.message())
    THROW_EXCEPTION(
        WebSocketTcpServerException,
        "cannot set option for accepting incoming web-socket connections: "
            << error_code.message())
  }

  // Bind the acceptor to the host address
  error_code = m_incoming_tcp_connection_acceptor->bind(
      tcp::endpoint(m_local_endpoint->address(), m_local_endpoint->port()),
      error_code);
  if (error_code) {
    LOG_ERR(
        "cannot bind incoming web-socket connection acceptor to host address "
        << m_local_endpoint->address().to_string() << ":"
        << m_local_endpoint->port() << ": " << error_code.message())
    THROW_EXCEPTION(
        WebSocketTcpServerException,
        "cannot bind incoming web-socket connection acceptor to host address: "
            << error_code.message())
  }

  // Listen for incoming connections
  error_code = m_incoming_tcp_connection_acceptor->listen(
      asio::socket_base::max_listen_connections, error_code);
  if (error_code) {
    LOG_ERR("cannot listen for incoming web-socket connections: "
            << error_code.message())
    THROW_EXCEPTION(WebSocketTcpServerException,
                    "cannot listen for incoming web-socket connections: "
                        << error_code.message())
  }
}

void BoostWebSocketConnectionListener::StartAcceptingAnotherConnection() {
  if (!m_incoming_tcp_connection_acceptor->is_open()) {
    return;
  }
  LOG_DEBUG("waiting for web-socket connections on "
            << m_local_endpoint->address().to_string() << " port "
            << m_local_endpoint->port())
  // Accept incoming connections
  m_incoming_tcp_connection_acceptor->async_accept(
      asio::make_strand(*m_execution_context),
      beast::bind_front_handler(
          &BoostWebSocketConnectionListener::ProcessTcpConnection,
          shared_from_this()));
}

void BoostWebSocketConnectionListener::ProcessTcpConnection(
    beast::error_code error_code, asio::ip::tcp::socket socket) {

  if (error_code == asio::error::operation_aborted) {
    LOG_DEBUG("local web-socket connection acceptor at "
              << m_local_endpoint->address().to_string() << ":"
              << m_local_endpoint->port() << " has stopped")
    return;
  } else if (error_code) {
    LOG_ERR(
        "server was not able to accept TCP connection for web-socket stream: "
        << error_code.message())
    StartAcceptingAnotherConnection();
    return;
  }

  if (!socket.is_open()) {
    LOG_ERR("server was not able to accept TCP connection for web-socket "
            "stream: TCP connection has already been closed")
    return;
  }

  auto remote_address = socket.remote_endpoint().address();
  auto remote_port = socket.remote_endpoint().port();
  auto local_address = socket.local_endpoint().address();
  auto local_port = socket.local_endpoint().port();

  LOG_DEBUG("accepted incoming TCP connection "
            << local_address.to_string() << ":" << local_port << "<-"
            << remote_address.to_string() << remote_port)

  // create stream and perform handshake
  auto stream = std::make_shared<stream_type>(std::move(socket));
  asio::dispatch(
      stream->get_executor(),
      beast::bind_front_handler(
          &BoostWebSocketConnectionListener::PerformWebSocketHandshake,
          shared_from_this(), stream));
}

void BoostWebSocketConnectionListener::PerformWebSocketHandshake(
    std::shared_ptr<stream_type> plain_tcp_stream) {

  // Set a decorator to change the Server of the handshake
  plain_tcp_stream->set_option(
      websocket::stream_base::decorator([](websocket::response_type &res) {
        res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) +
                                         " websocket-server-async");
      }));

  // Accept the websocket handshake
  plain_tcp_stream->async_accept(beast::bind_front_handler(
      &BoostWebSocketConnectionListener::ProcessWebSocketHandshake,
      shared_from_this(), plain_tcp_stream));
}

void BoostWebSocketConnectionListener::ProcessWebSocketHandshake(
    std::shared_ptr<stream_type> web_socket_stream, beast::error_code ec) {

  auto address =
      web_socket_stream->next_layer().socket().remote_endpoint().address();
  if (ec) {
    LOG_ERR("performing web-socket handshake with " << address.to_string()
                                                    << ": " << ec.message())
    StartAcceptingAnotherConnection();
    return;
  }

  auto port = web_socket_stream->next_layer().socket().remote_endpoint().port();
  std::string host = address.to_string() + ":" + std::to_string(port);

  auto &socket = web_socket_stream->next_layer().socket();
  auto remote_address = socket.remote_endpoint().address();
  auto remote_port = socket.remote_endpoint().port();
  auto local_address = socket.local_endpoint().address();
  auto local_port = socket.local_endpoint().port();

  LOG_DEBUG("accepted incoming web-socket connection "
            << local_address.to_string() << ":" << local_port << "<-"
            << remote_address.to_string() << ":" << remote_port)

  ConnectionInfo connection_info;
  connection_info.hostname = host;

  // wait for handshake initiation
  auto handshake_request_buffer = std::make_shared<beast::flat_buffer>();
  web_socket_stream->async_read(
      *handshake_request_buffer,
      beast::bind_front_handler(
          &BoostWebSocketConnectionListener::ProcessNodeHandshakeRequest,
          shared_from_this(), std::move(web_socket_stream), connection_info,
          handshake_request_buffer));
}
void BoostWebSocketConnectionListener::ProcessNodeHandshakeRequest(
    std::shared_ptr<stream_type> web_socket_stream,
    ConnectionInfo connection_info,
    std::shared_ptr<boost::beast::flat_buffer> handhake_request_buffer,
    boost::beast::error_code ec, std::size_t bytes_transferred) {

  if (ec) {
    LOG_ERR("node handshake request failed with host "
            << connection_info.hostname << ": " << ec.message())
    web_socket_stream->close(beast::websocket::close_code::none);
    return;
  }

  // extract sent bytes and pass them to the callback function
  std::string other_endpoint_id =
      boost::beast::buffers_to_string(handhake_request_buffer->data());
  handhake_request_buffer->consume(handhake_request_buffer->size());

  connection_info.endpoint_id = other_endpoint_id;

  web_socket_stream->write(boost::asio::buffer(m_this_node_uuid), ec);

  if (ec) {
    LOG_ERR("node handshake response failed with host "
            << connection_info.hostname << ": " << ec.message())
    web_socket_stream->close(beast::websocket::close_code::none);
    return;
  }

  LOG_INFO("node " << connection_info.endpoint_id << " from "
                   << connection_info.hostname
                   << " sucessfully connected via web-sockets")

  m_connection_consumer(connection_info, std::move(*web_socket_stream));

  StartAcceptingAnotherConnection();
}

void BoostWebSocketConnectionListener::ShutDown() {
  LOG_DEBUG("web-socket connection listener has been shut down")
  if (m_incoming_tcp_connection_acceptor->is_open()) {
    m_incoming_tcp_connection_acceptor->cancel();
    m_incoming_tcp_connection_acceptor->close();
  }
}