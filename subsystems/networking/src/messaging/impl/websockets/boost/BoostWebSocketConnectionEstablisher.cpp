#include "networking/messaging/impl/websockets/boost/BoostWebSocketConnectionEstablisher.h"
#include "logging/LogManager.h"
#include "networking/util/UrlParser.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <regex>
#include <string>
#include <utility>

using namespace networking::messaging::websockets;
using namespace networking::messaging;
using namespace networking;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;           // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

BoostWebSocketConnectionEstablisher::BoostWebSocketConnectionEstablisher(
    std::string this_node_uuid,
    std::shared_ptr<asio::io_context> execution_context,
    std::function<void(ConnectionInfo, stream_type &&)> connection_consumer)
    : m_resolver{asio::make_strand(*execution_context)},
      m_execution_context{execution_context},
      m_connection_consumer{std::move(connection_consumer)},
      m_this_node_uuid(std::move(this_node_uuid)) {}

std::future<ConnectionInfo>
BoostWebSocketConnectionEstablisher::EstablishConnectionTo(
    const std::string &uri) {

  std::promise<ConnectionInfo> connection_promise;

  auto maybe_valid_url = util::UrlParser::parse(uri);
  if (!maybe_valid_url.has_value()) {
    LOG_WARN("failed to establish web-socket connection due to malformed URL '"
             << uri << "'")
    THROW_EXCEPTION(UrlMalformedException, "URL is malformed")
  }

  LOG_DEBUG("connection attempt to host " << uri << " started")

  util::ParsedUrl valid_url = maybe_valid_url.value();

  std::future<ConnectionInfo> connection_future =
      connection_promise.get_future();

  m_resolver.async_resolve(
      valid_url.host, valid_url.port,
      beast::bind_front_handler(
          &BoostWebSocketConnectionEstablisher::ProcessResolvedHostnameOfServer,
          shared_from_this(), uri, std::move(connection_promise)));

  return connection_future;
}

void BoostWebSocketConnectionEstablisher::ProcessResolvedHostnameOfServer(
    std::string uri, std::promise<ConnectionInfo> &&connection_promise,
    beast::error_code error_code, tcp::resolver::results_type results) {

  if (error_code) {
    LOG_WARN("cannot create web-socket connection to "
             << uri
             << " due to hostname resolving issues: " << error_code.message())
    auto exception =
        BUILD_EXCEPTION(CannotResolveHostException, "cannot resolve host");
    connection_promise.set_exception(std::make_exception_ptr(exception));
    return;
  }

  auto plain_tcp_stream =
      std::make_shared<stream_type>(asio::make_strand(*m_execution_context));

  // Set the timeout for the operation
  get_lowest_layer(*plain_tcp_stream).expires_after(std::chrono::seconds(30));

  // Make the connection on the IP address we get from a lookup
  get_lowest_layer(*plain_tcp_stream)
      .async_connect(results,
                     beast::bind_front_handler(
                         &BoostWebSocketConnectionEstablisher::
                             ProcessEstablishedTcpConnection,
                         shared_from_this(), std::move(connection_promise), uri,
                         plain_tcp_stream));
}

void BoostWebSocketConnectionEstablisher::ProcessEstablishedTcpConnection(
    std::promise<ConnectionInfo> &&connection_promise, std::string uri,
    std::shared_ptr<stream_type> plain_tcp_stream, beast::error_code error_code,
    tcp::resolver::results_type::endpoint_type endpoint_type) {

  if (error_code) {
    LOG_WARN("cannot establish TCP connection to "
             << endpoint_type.address() << " (" << uri << ") "
             << ": " << error_code.message())

    auto exception = BUILD_EXCEPTION(ConnectionFailedException,
                                     "cannot establish TCP connection to "
                                         << endpoint_type.address() << ": "
                                         << error_code.message());
    connection_promise.set_exception(std::make_exception_ptr(exception));
    return;
  }

  LOG_DEBUG("established outgoing TCP connection to "
            << endpoint_type.address().to_string())

  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  get_lowest_layer(*plain_tcp_stream).expires_never();

  // Set suggested timeout settings for the websocket
  plain_tcp_stream->set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::client));

  // Set a decorator to change the User-Agent of the handshake
  plain_tcp_stream->set_option(
      websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-async");
      }));

  auto host = endpoint_type.address().to_string() + ":" +
              std::to_string(endpoint_type.port());

  // Perform the websocket handshake
  plain_tcp_stream->async_handshake(
      host, "/",
      beast::bind_front_handler(
          &BoostWebSocketConnectionEstablisher::ProcessWebSocketHandshake,
          shared_from_this(), std::move(connection_promise), uri,
          plain_tcp_stream));
}

void BoostWebSocketConnectionEstablisher::ProcessWebSocketHandshake(
    std::promise<ConnectionInfo> &&connection_promise, std::string uri,
    std::shared_ptr<stream_type> web_socket_stream,
    beast::error_code error_code) {

  auto remote_endpoint_info =
      web_socket_stream->next_layer().socket().remote_endpoint();
  auto local_endpoint_info =
      web_socket_stream->next_layer().socket().local_endpoint();

  auto remote_address = remote_endpoint_info.address();
  auto remote_port = remote_endpoint_info.port();
  auto local_address = local_endpoint_info.address();
  auto local_port = local_endpoint_info.port();

  if (error_code) {
    LOG_ERR(
        "web-socket handshake with remote host over established tcp connection "
        << local_address.to_string() << ":" << local_port << "->"
        << remote_address.to_string() << ":" << remote_port
        << " failed: " << error_code.message())
    auto exception = BUILD_EXCEPTION(
        ConnectionFailedException,
        "web-socket handshake with remote host over established tcp connection "
            << local_address.to_string() << ":" << local_port << "->"
            << remote_address.to_string() << ":" << remote_port
            << " failed: " << error_code.message());
    connection_promise.set_exception(std::make_exception_ptr(exception));
    return;
  }

  LOG_DEBUG("established outgoing web-socket connection "
            << local_address.to_string() << ":" << local_port << "->"
            << remote_address.to_string() << ":" << remote_port);

  ConnectionInfo connection_info;
  connection_info.hostname = uri;

  PerformNodeHandshake(std::move(connection_promise),
                       std::move(connection_info), web_socket_stream);
}

void BoostWebSocketConnectionEstablisher::PerformNodeHandshake(
    std::promise<ConnectionInfo> &&connection_promise, ConnectionInfo &&info,
    std::shared_ptr<stream_type> web_socket_stream) {

  // send this node's endpoint ID to the other endpoint
  beast::error_code error_code;
  web_socket_stream->write(boost::asio::buffer(m_this_node_uuid), error_code);

  if (error_code) {
    LOG_ERR("node handshake failed: " << error_code.message())
    auto exception =
        BUILD_EXCEPTION(ConnectionFailedException,
                        "node handshake failed: " << error_code.message());
    connection_promise.set_exception(std::make_exception_ptr(exception));
    return;
  }

  // wait for other node's endpoint ID response
  auto handshake_response_buffer = std::make_shared<beast::flat_buffer>();
  web_socket_stream->async_read(
      *handshake_response_buffer,
      beast::bind_front_handler(
          &BoostWebSocketConnectionEstablisher::ProcessNodeHandshakeResponse,
          shared_from_this(), std::move(connection_promise), std::move(info),
          web_socket_stream, handshake_response_buffer));
}

void BoostWebSocketConnectionEstablisher::ProcessNodeHandshakeResponse(
    std::promise<ConnectionInfo> &&connection_promise, ConnectionInfo &&info,
    std::shared_ptr<stream_type> web_socket_stream,
    std::shared_ptr<beast::flat_buffer> handshake_response_buffer,
    beast::error_code error_code, std::size_t bytes_transferred) {

  if (error_code) {
    LOG_ERR("node handshake failed: " << error_code.message())
    auto exception =
        BUILD_EXCEPTION(ConnectionFailedException,
                        "node handshake failed: " << error_code.message());
    connection_promise.set_exception(std::make_exception_ptr(exception));
    return;
  }

  // extract sent bytes and pass them to the callback function
  std::string other_endpoint_id =
      boost::beast::buffers_to_string(handshake_response_buffer->data());
  handshake_response_buffer->consume(handshake_response_buffer->size());

  info.endpoint_id = other_endpoint_id;

  LOG_INFO("successfully connected to " << info.endpoint_id << " at "
                                        << info.hostname << " via web-sockets")

  // consume connection
  m_connection_consumer(info, std::move(*web_socket_stream));
  connection_promise.set_value(info);
}