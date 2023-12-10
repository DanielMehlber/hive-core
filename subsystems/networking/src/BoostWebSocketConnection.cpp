#include "networking/peers/impl/websockets/boost/BoostWebSocketConnection.h"
#include "logging/LogManager.h"
#include "networking/peers/PeerMessageConverter.h"
#include <boost/asio.hpp>
#include <utility>

using namespace networking::websockets;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;           // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

BoostWebSocketConnection::BoostWebSocketConnection(
    stream_type &&socket,
    std::function<void(const std::string &, SharedBoostWebSocketConnection)>
        on_message_received,
    std::function<void(const std::string &)> on_connection_closed)
    : m_socket(std::move(socket)),
      m_on_message_received{std::move(on_message_received)},
      m_on_connection_closed{std::move(on_connection_closed)} {

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
    beast::error_code error_code,
    [[maybe_unused]] std::size_t bytes_transferred) {

  // check for closing message
  if (error_code == websocket::error::closed ||
      error_code == http::error::end_of_stream ||
      error_code == boost::asio::error::connection_reset) {
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

  m_on_connection_closed(GetConnectionInfo().GetHostname());
}

BoostWebSocketConnection::~BoostWebSocketConnection() { Close(); }

std::future<void>
BoostWebSocketConnection::Send(const SharedWebSocketMessage &message) {
  std::promise<void> sending_promise;
  std::future<void> sending_future = sending_promise.get_future();

  std::shared_ptr<std::string> payload = std::make_shared<std::string>(
      networking::websockets::PeerMessageConverter::ToMultipartFormData(
          message));

  /*
   * The async_write must finish before another one can be called. This lock
   * will be released in the callback method.
   */
  std::unique_lock lock(m_socket_mutex);

  if (!m_socket.is_open()) {
    LOG_WARN("Cannot sent message via web-socket to remote host "
             << m_remote_endpoint_data.address().to_string() << ":"
             << m_remote_endpoint_data.port() << " because socket is closed")

    THROW_EXCEPTION(ConnectionClosedException, "connection is not open")
  }

  m_socket.binary(true);

  /*
   * TODO: Make this async to avoid blocking
   * Warning: The async_write call must be synchronized with some sort of lock
   * or condition variable. The async call must finish before another can be
   * started.
   */

  //  m_socket.async_write(asio::buffer(*payload),
  //                       boost::beast::bind_front_handler(
  //                           &BoostWebSocketConnection::OnMessageSent,
  //                           shared_from_this(), std::move(sending_promise),
  //                           message, payload, std::move(lock)));

  boost::beast::error_code error_code;
  std::size_t bytes_transferred =
      m_socket.write(asio::buffer(*payload), error_code);

  OnMessageSent(std::move(sending_promise), message, payload, std::move(lock),
                error_code, bytes_transferred);

  return std::move(sending_future);
}

void BoostWebSocketConnection::OnMessageSent(
    std::promise<void> &&promise, SharedWebSocketMessage message,
    std::shared_ptr<std::string> sent_data, std::unique_lock<std::mutex> lock,
    boost::beast::error_code error_code,
    [[maybe_unused]] std::size_t bytes_transferred) {

  // allow others to send messages now
  lock.unlock();

  if (error_code) {
    LOG_WARN("Sending message via web-socket to remote host "
             << m_remote_endpoint_data.address().to_string() << ":"
             << m_remote_endpoint_data.port()
             << " failed: " << error_code.message())
    auto exception =
        BUILD_EXCEPTION(MessageSendingException,
                        "Sending message via web-socket to remote host "
                            << m_remote_endpoint_data.address().to_string()
                            << ":" << m_remote_endpoint_data.port()
                            << " failed: " << error_code.message());
    promise.set_exception(std::make_exception_ptr(exception));
    return;
  }

  promise.set_value();
  LOG_DEBUG("Sent message of type "
            << message->GetType() << " (" << sent_data->size()
            << " bytes) via web-socket to host "
            << m_remote_endpoint_data.address().to_string() << ":"
            << m_remote_endpoint_data.port())
}

PeerConnectionInfo BoostWebSocketConnection::GetConnectionInfo() const {
  PeerConnectionInfo info;
  info.SetHostname(GetRemoteHostAddress());
  return info;
}
