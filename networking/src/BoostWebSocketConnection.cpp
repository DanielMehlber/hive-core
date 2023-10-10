#include "networking/websockets/impl/boost/BoostWebSocketConnection.h"
#include "logging/Logging.h"
#include <boost/asio.hpp>

using namespace networking::websockets;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;           // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

BoostWebSocketConnection::BoostWebSocketConnection(
    stream_type &&socket,
    std::function<void(const std::string &, SharedBoostWebSocketConnection)>
        on_message_received)
    : m_socket(std::move(socket)), m_on_message_received{on_message_received} {

  m_remote_endpoint_data = m_socket.next_layer().socket().remote_endpoint();
}

void BoostWebSocketConnection::StartReceivingMessages() {
  // We need to be executing within an asio strand to perform async operations
  // on the I/O objects in this session. Although not strictly necessary
  // for single-threaded contexts, this example code is written to be
  // thread-safe by default.
  asio::dispatch(
      m_socket.get_executor(),
      beast::bind_front_handler(&BoostWebSocketConnection::AsyncReceiveMessage,
                                shared_from_this()));
}

void BoostWebSocketConnection::OnMessageReceived(
    beast::error_code error_code, std::size_t bytes_transferred) {

  // check for closing message
  if (error_code == websocket::error::closed ||
      error_code == http::error::end_of_stream) {
    LOG_WARN("remote host " << m_remote_endpoint_data.address().to_string()
                            << ":" << m_remote_endpoint_data.port()
                            << " has closed the web-socket connection");

    // shut down the TCP connection as well
    m_socket.next_layer().socket().shutdown(tcp::socket::shutdown_send,
                                            error_code);
    return;
  } else if (error_code) {
    LOG_ERR("failed to receive web-socket message from host "
            << m_remote_endpoint_data.address() << ": "
            << error_code.message());
    return;
  }

  // read next message asynchronously
  if (IsUsable()) {
    AsyncReceiveMessage();
  }

  // extract sent bytes and pass them to the callback function
  const auto received_data =
      boost::asio::buffer_cast<const char *>(m_buffer.data());
  std::string content = received_data;
  m_on_message_received(received_data, shared_from_this());
}

void BoostWebSocketConnection::AsyncReceiveMessage() {
  m_socket.async_read(
      m_buffer,
      boost::beast::bind_front_handler(
          &BoostWebSocketConnection::OnMessageReceived, shared_from_this()));
}

void BoostWebSocketConnection::Close() {
  if (m_socket.is_open()) {
    m_socket.async_close(
        beast::websocket::close_code::normal, [this](beast::error_code ec) {
          if (ec) {
            LOG_WARN("closing web-socket connection to "
                     << m_remote_endpoint_data.address().to_string() << ":"
                     << m_remote_endpoint_data.port()
                     << "failed: " << ec.message());
          } else {
            LOG_INFO("closed web-socket connection to "
                     << m_remote_endpoint_data.address().to_string() << ":"
                     << m_remote_endpoint_data.port());
          }
        });
  }
}

BoostWebSocketConnection::~BoostWebSocketConnection() { Close(); }