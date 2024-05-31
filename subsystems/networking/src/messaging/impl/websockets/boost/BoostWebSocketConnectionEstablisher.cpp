#include "networking/messaging/impl/websockets/boost/BoostWebSocketConnectionEstablisher.h"
#include "logging/LogManager.h"
#include "networking/util/UrlParser.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <regex>
#include <string>

using namespace networking::messaging::websockets;
using namespace networking::messaging;
using namespace networking;

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;           // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

BoostWebSocketConnectionEstablisher::BoostWebSocketConnectionEstablisher(
    std::shared_ptr<boost::asio::io_context> execution_context,
    std::function<void(std::string, stream_type &&)> connection_consumer)
    : m_resolver{asio::make_strand(*execution_context)},
      m_execution_context{execution_context},
      m_connection_consumer{std::move(connection_consumer)} {}

std::future<void> BoostWebSocketConnectionEstablisher::EstablishConnectionTo(
    const std::string &uri) {

  std::promise<void> connection_promise;

  auto optional_uri = util::UrlParser::parse(uri);
  if (!optional_uri.has_value()) {
    LOG_WARN("failed to establish web-socket connection due to malformed URL '"
             << uri << "'")
    THROW_EXCEPTION(UrlMalformedException, "URL is malformed")
  }

  LOG_DEBUG("connection attempt to host " << uri << " started")

  util::ParsedUrl url = optional_uri.value();

  std::future<void> connection_future = connection_promise.get_future();

  m_resolver.async_resolve(
      url.host, url.port,
      beast::bind_front_handler(
          &BoostWebSocketConnectionEstablisher::ProcessResolvedHostnameOfServer,
          shared_from_this(), uri, std::move(connection_promise)));

  return connection_future;
}

void BoostWebSocketConnectionEstablisher::ProcessResolvedHostnameOfServer(
    std::string uri, std::promise<void> &&connection_promise,
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

  auto stream =
      std::make_shared<stream_type>(asio::make_strand(*m_execution_context));
  // create new stream that will be processed in further asynchronous steps

  // Set the timeout for the operation
  beast::get_lowest_layer(*stream).expires_after(std::chrono::seconds(30));

  // Make the connection on the IP address we get from a lookup
  beast::get_lowest_layer(*stream).async_connect(
      results,
      beast::bind_front_handler(
          &BoostWebSocketConnectionEstablisher::ProcessEstablishedTcpConnection,
          shared_from_this(), std::move(connection_promise), uri, stream));
}

void BoostWebSocketConnectionEstablisher::ProcessEstablishedTcpConnection(
    std::promise<void> &&connection_promise, std::string uri,
    std::shared_ptr<stream_type> stream, beast::error_code error_code,
    tcp::resolver::results_type::endpoint_type endpoint_type) {

  if (error_code) {
    LOG_WARN("cannot establish TCP connection to "
             << endpoint_type.address() << " (" << uri << ") " << ": "
             << error_code.message())

    auto exception = BUILD_EXCEPTION(ConnectionFailedException,
                                     "cannot establish TCP connection to "
                                         << endpoint_type.address() << ": "
                                         << error_code.message());
    connection_promise.set_exception(std::make_exception_ptr(exception));
    return;
  }

  LOG_DEBUG("established TCP connection to "
            << endpoint_type.address().to_string())

  // Turn off the timeout on the tcp_stream, because
  // the websocket stream has its own timeout system.
  beast::get_lowest_layer(*stream).expires_never();

  // Set suggested timeout settings for the websocket
  stream->set_option(
      websocket::stream_base::timeout::suggested(beast::role_type::client));

  // Set a decorator to change the User-Agent of the handshake
  stream->set_option(
      websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-async");
      }));

  auto host = endpoint_type.address().to_string() + ":" +
              std::to_string(endpoint_type.port());

  // Perform the websocket handshake
  stream->async_handshake(
      host, "/",
      beast::bind_front_handler(
          &BoostWebSocketConnectionEstablisher::ProcessWebSocketHandshake,
          shared_from_this(), std::move(connection_promise), uri, stream));
}

void BoostWebSocketConnectionEstablisher::ProcessWebSocketHandshake(
    std::promise<void> &&connection_promise, std::string uri,
    std::shared_ptr<stream_type> stream, beast::error_code error_code) {

  auto remote_address =
      stream->next_layer().socket().remote_endpoint().address();
  auto remote_port = stream->next_layer().socket().remote_endpoint().port();
  auto local_address = stream->next_layer().socket().local_endpoint().address();
  auto local_port = stream->next_layer().socket().local_endpoint().port();
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

  LOG_INFO("successfully established outgoing web-socket connection "
           << local_address.to_string() << ":" << local_port << "->"
           << remote_address.to_string() << ":" << remote_port);

  // consume connection
  m_connection_consumer(uri, std::move(*stream));

  // notify future about resolution of promise
  connection_promise.set_value();
}