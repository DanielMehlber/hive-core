#include "networking/messaging/impl/websockets/boost/BoostWebSocketConnection.h"
#include "logging/LogManager.h"
#include "networking/messaging/MessageConverter.h"
#include <boost/asio.hpp>
#include <utility>

using namespace networking::messaging;
using namespace networking::messaging::websockets;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;           // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

using namespace std::chrono_literals;

#define HANDSHAKE_TIMEOUT 5s
#define IDLE_TIMEOUT 5s

BoostWebSocketConnection::BoostWebSocketConnection(
    ConnectionInfo info, stream_type &&socket,
    std::function<void(const std::string &, SharedBoostWebSocketConnection)>
        on_message_received,
    std::function<void(const std::string &)> on_connection_closed)
    : m_socket(std::move(socket)),
      m_on_message_received{std::move(on_message_received)},
      m_on_connection_closed{std::move(on_connection_closed)},
      m_connection_info(std::move(info)) {

  m_remote_endpoint_data = m_socket.next_layer().socket().remote_endpoint();

  // activates timeouts and liveliness probes. Timeouts will be handled by
  // outstanding async operations (async_read, write, ...).
  beast::websocket::stream_base::timeout timeout_settings{
      HANDSHAKE_TIMEOUT, IDLE_TIMEOUT,
      true // liveliness probes at half-time of idle timeout
  };
  m_socket.set_option(timeout_settings);
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
    beast::error_code error_code,
    [[maybe_unused]] std::size_t bytes_transferred) {

  // check for closing message
  if (error_code == websocket::error::closed ||
      error_code == http::error::end_of_stream ||
      error_code == boost::asio::error::connection_reset ||
      error_code == beast::error::timeout) {
    LOG_WARN("remote host " << m_remote_endpoint_data.address().to_string()
                            << ":" << m_remote_endpoint_data.port()
                            << " has closed the web-socket connection")

    Close();
    return;
  } else if (error_code == asio::error::operation_aborted) {
    LOG_DEBUG("local host has cancelled listening to web-socket message from "
              << m_remote_endpoint_data.address().to_string() << ":"
              << m_remote_endpoint_data.port())
    return;
  } else if (error_code) {
    LOG_ERR("failed to receive web-socket message from host "
            << m_remote_endpoint_data.address() << ": " << error_code.message())
    return;
  }

  // extract sent bytes and pass them to the callback function
  std::string received_data = boost::beast::buffers_to_string(m_buffer.data());
  m_buffer.consume(m_buffer.size());

  // read next message asynchronously
  if (IsUsable()) {
    AsyncReceiveMessage();
  }

  m_on_message_received(received_data, shared_from_this());
}

void BoostWebSocketConnection::AsyncReceiveMessage() {
  m_socket.async_read(
      m_buffer,
      boost::beast::bind_front_handler(
          &BoostWebSocketConnection::OnMessageReceived, shared_from_this()));
}

void BoostWebSocketConnection::Close() {
  std::unique_lock lock(m_socket_mutex);
  if (m_socket.is_open()) {
    beast::error_code error_code;
    m_socket.close(beast::websocket::close_code::normal, error_code);
    // shut down the TCP connection as well
    if (m_socket.next_layer().socket().is_open()) {
      m_socket.next_layer().socket().shutdown(tcp::socket::shutdown_send,
                                              error_code);
    }

    LOG_INFO("closed web-socket connection to "
             << m_remote_endpoint_data.address().to_string() << ":"
             << m_remote_endpoint_data.port())
  }

  m_on_connection_closed(m_connection_info.endpoint_id);

  DEBUG_ASSERT(!m_socket.is_open(), "socket should be closed now")
}

BoostWebSocketConnection::~BoostWebSocketConnection() { Close(); }

std::future<void> BoostWebSocketConnection::Send(const SharedMessage &message) {
  std::promise<void> sending_promise;
  std::future<void> sending_future = sending_promise.get_future();

  std::shared_ptr<std::string> payload = std::make_shared<std::string>(
      networking::messaging::MessageConverter::ToMultipartFormData(message));

  /*
   * The async_write must finish before another one can be called. This lock
   * will be released in the callback method.
   */
  std::unique_lock lock(m_socket_mutex);

  if (!m_socket.is_open()) {
    LOG_WARN("cannot sent message via web-socket to remote host "
             << m_remote_endpoint_data.address().to_string() << ":"
             << m_remote_endpoint_data.port() << " because socket is closed")

    THROW_EXCEPTION(ConnectionClosedException, "connection is not open")
  }

  m_socket.binary(true);

  /* THIS IS INTENTIONALLY SYNCHRONOUS
   * Warning: The async_write call must be synchronized with some sort of lock
   * or condition variable. The async call must finish before another can be
   * started. The easiest way is to make it synchronous in the first place.
   */
  beast::error_code error_code;
  std::size_t bytes_transferred =
      m_socket.write(asio::buffer(*payload), error_code);

  OnMessageSent(std::move(sending_promise), message, payload, std::move(lock),
                error_code, bytes_transferred);

  return std::move(sending_future);
}

void BoostWebSocketConnection::OnMessageSent(
    std::promise<void> &&promise, SharedMessage message,
    std::shared_ptr<std::string> sent_data,
    std::unique_lock<jobsystem::recursive_mutex> lock,
    boost::beast::error_code error_code,
    [[maybe_unused]] std::size_t bytes_transferred) {

  // allow others to send messages now
  lock.unlock();

  if (error_code) {
    LOG_WARN("sending message via web-socket to remote host "
             << m_remote_endpoint_data.address().to_string() << ":"
             << m_remote_endpoint_data.port()
             << " failed: " << error_code.message())
    auto exception =
        BUILD_EXCEPTION(MessageSendingException,
                        "sending message via web-socket to remote host "
                            << m_remote_endpoint_data.address().to_string()
                            << ":" << m_remote_endpoint_data.port()
                            << " failed: " << error_code.message());
    promise.set_exception(std::make_exception_ptr(exception));

    // if the connection timed out, it must be cleaned up.
    if (error_code == beast::error::timeout) {
      Close();
    }

    return;
  }

  promise.set_value();
  LOG_DEBUG("sent message of type "
            << message->GetType() << " (" << sent_data->size()
            << " bytes) via web-socket to host "
            << m_remote_endpoint_data.address().to_string() << ":"
            << m_remote_endpoint_data.port())
}
