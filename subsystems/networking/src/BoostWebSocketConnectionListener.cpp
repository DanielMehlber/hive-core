#include "networking/messaging/impl/websockets/boost/BoostWebSocketConnectionListener.h"

using namespace networking;
using namespace networking::messaging;
using namespace networking::messaging::websockets;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;           // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

BoostWebSocketConnectionListener::BoostWebSocketConnectionListener(
    std::shared_ptr<boost::asio::io_context> execution_context,
    common::config::SharedConfiguration config,
    std::shared_ptr<boost::asio::ip::tcp::endpoint> local_endpoint,
    std::function<void(std::string, stream_type &&)> connection_consumer)
    : m_connection_consumer{std::move(connection_consumer)},
      m_execution_context{std::move(execution_context)}, m_config{config},
      m_local_endpoint{std::move(local_endpoint)} {}

BoostWebSocketConnectionListener::~BoostWebSocketConnectionListener() {
  ShutDown();
}

void BoostWebSocketConnectionListener::Init() {
  // load configurations
  bool use_tls = m_config->GetBool("net.tls.enabled", true);

  if (use_tls) {
    LOG_WARN(
        "TLS in web-sockets is not implemented yet and is therefore not used")
  }

  // setup acceptor which listens for incoming connections asynchronously
  m_incoming_connection_acceptor =
      std::make_unique<tcp::acceptor>(*m_execution_context);

  beast::error_code error_code;

  // open the acceptor for incoming connections
  m_incoming_connection_acceptor->open(m_local_endpoint->protocol(),
                                       error_code);
  if (error_code) {
    LOG_ERR("cannot setup acceptor for incoming web-socket connections: "
            << error_code.message())
    THROW_EXCEPTION(
        WebSocketTcpServerException,
        "cannot setup acceptor for incoming web-socket connections: "
            << error_code.message())
  }

  m_incoming_connection_acceptor->set_option(
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
  m_incoming_connection_acceptor->bind(
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
  m_incoming_connection_acceptor->listen(
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
  if (!m_incoming_connection_acceptor->is_open()) {
    return;
  }
  LOG_DEBUG("waiting for web-socket connections on "
            << m_local_endpoint->address().to_string() << " port "
            << m_local_endpoint->port())
  // Accept incoming connections
  m_incoming_connection_acceptor->async_accept(
      asio::make_strand(*m_execution_context),
      beast::bind_front_handler(
          &BoostWebSocketConnectionListener::ProcessTcpConnection,
          shared_from_this()));
}

void BoostWebSocketConnectionListener::ProcessTcpConnection(
    boost::beast::error_code error_code, boost::asio::ip::tcp::socket socket) {

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

  auto remote_endpoint = socket.remote_endpoint();
  auto remote_endpoint_address = remote_endpoint.address();

  LOG_DEBUG("new TCP connection for web-socket established by host "
            << remote_endpoint_address.to_string())

  // create stream and perform handshake
  auto stream = std::make_shared<stream_type>(std::move(socket));
  asio::dispatch(
      stream->get_executor(),
      beast::bind_front_handler(
          &BoostWebSocketConnectionListener::PerformWebSocketHandshake,
          shared_from_this(), stream));
}

void BoostWebSocketConnectionListener::PerformWebSocketHandshake(
    std::shared_ptr<stream_type> current_stream) {

  // Set a decorator to change the Server of the handshake
  current_stream->set_option(
      websocket::stream_base::decorator([](websocket::response_type &res) {
        res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) +
                                         " websocket-server-async");
      }));

  // Accept the websocket handshake
  current_stream->async_accept(beast::bind_front_handler(
      &BoostWebSocketConnectionListener::ProcessWebSocketHandshake,
      shared_from_this(), current_stream));
}

void BoostWebSocketConnectionListener::ProcessWebSocketHandshake(
    std::shared_ptr<stream_type> current_stream, beast::error_code ec) {

  auto address =
      current_stream->next_layer().socket().remote_endpoint().address();
  if (ec) {
    LOG_ERR("performing web-socket handshake with " << address.to_string()
                                                    << ": " << ec.message())
    // TODO: close TCP connection
    StartAcceptingAnotherConnection();
    return;
  }

  auto port = current_stream->next_layer().socket().remote_endpoint().port();
  std::string host = address.to_string() + ":" + std::to_string(port);

  LOG_INFO("web-socket connection established by " << address.to_string() << ":"
                                                   << port)
  m_connection_consumer(host, std::move(*current_stream));

  StartAcceptingAnotherConnection();
}

void BoostWebSocketConnectionListener::ShutDown() {
  LOG_DEBUG("web-socket connection listener has been shut down")
  if (m_incoming_connection_acceptor->is_open()) {
    m_incoming_connection_acceptor->cancel();
    m_incoming_connection_acceptor->close();
  }
}