#ifndef BOOSTWEBSOCKETCONNECTION_H
#define BOOSTWEBSOCKETCONNECTION_H

#include "common/exceptions/ExceptionsBase.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/messaging/ConnectionInfo.h"
#include "networking/messaging/Message.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <future>
#include <memory>

namespace networking::websockets {

typedef boost::beast::websocket::stream<boost::beast::tcp_stream> stream_type;

DECLARE_EXCEPTION(ConnectionClosedException);
DECLARE_EXCEPTION(MessageSendingException);

/**
 * A connection between two endpoints realized using Boost.Beast websockets.
 */
class BoostWebSocketConnection
    : public std::enable_shared_from_this<BoostWebSocketConnection> {
private:
  /**
   * TCP stream that allows interaction with the communication partner.
   */
  stream_type m_socket;
  mutable jobsystem::mutex m_socket_mutex;

  /**
   * Buffer for received messages
   */
  boost::beast::flat_buffer m_buffer;

  /**
   * Will be called to pass the received data on for further processing
   */
  const std::function<void(const std::string &,
                           std::shared_ptr<BoostWebSocketConnection>)>
      m_on_message_received;

  /**
   * Will be called when the connection was closed by this or the other
   * peer
   */
  const std::function<void(const std::string &)> m_on_connection_closed;

  /**
   * Data describing the remote endpoint of this connection
   */
  boost::asio::ip::tcp::endpoint m_remote_endpoint_data;

  /**
   * Waits for message asynchronously and processes it on arrival
   */
  void AsyncReceiveMessage();

  /**
   * Callback function that will be called when a message has been
   * received from the remote host.
   * @param error_code indicating if the message was received successfully
   * @param bytes_transferred count of received bytes from the remote host
   */
  void OnMessageReceived(boost::beast::error_code error_code,
                         [[maybe_unused]] std::size_t bytes_transferred);

  /**
   * Callback function that will be called when a message has been sent to some
   * remote host (or if that operation has failed)
   * @param promise promise that must be resolved
   * @param message message that was sent (or maybe not)
   * @param sent_data data that has been sent to the other peer.
   * @param lock locks the async_write call and must be unlocked
   * @param error_code status indicating the success of sending the message
   * @param bytes_transferred number of bytes that have been sent
   */
  void OnMessageSent(std::promise<void> &&promise, SharedMessage message,
                     std::shared_ptr<std::string> sent_data,
                     std::unique_lock<jobsystem::mutex> lock,
                     boost::beast::error_code error_code,
                     [[maybe_unused]] std::size_t bytes_transferred);

public:
  /**
   * Constructs a new web-socket connection handler
   * @param socket TCP stream which is used for the web-socket connection
   * @param on_message_received callback function on when a message has been
   * received
   * @attention The newly constructed connection won't automatically listen to
   * incoming events. This can be started by calling StartReceivingMessages.
   */
  BoostWebSocketConnection(
      stream_type &&socket,
      std::function<void(const std::string &,
                         std::shared_ptr<BoostWebSocketConnection>)>
          on_message_received,
      std::function<void(const std::string &)> on_connection_closed);

  /**
   * Shuts down the connection
   */
  ~BoostWebSocketConnection();

  /**
   * Lets connection listen for incoming events and processes them on
   * arrival
   * @attention Without this call, the connection won't be able to process
   * incoming events
   */
  void StartReceivingMessages();

  /**
   * Closes the web-socket connection and the underlying tcp stream
   */
  void Close();

  /**
   * Sends a message to the remote peer through this connection.
   * @param message web-socket message that will be sent
   * @return future indicating that the message has been sent
   */
  std::future<void> Send(const SharedMessage &message);

  /**
   * @return address of the connected remote endpoint
   */
  std::string GetRemoteHostAddress() const;

  /**
   * Checks if the connected endpoint is reachable, i.e. if the connection is
   * still standing.
   * @return true, if the connection can be used for sending/receiving events
   */
  bool IsUsable() const;

  /**
   * Generates connection info object from current connection data
   * @return connection info object
   */
  ConnectionInfo GetConnectionInfo() const;
};

inline std::string BoostWebSocketConnection::GetRemoteHostAddress() const {
  return m_remote_endpoint_data.address().to_string() + ":" +
         std::to_string(m_remote_endpoint_data.port());
}

inline bool BoostWebSocketConnection::IsUsable() const {
  std::unique_lock lock(m_socket_mutex);
  return m_socket.is_open();
}

typedef std::shared_ptr<BoostWebSocketConnection>
    SharedBoostWebSocketConnection;

} // namespace networking::websockets

#endif /* BOOSTWEBSOCKETCONNECTION_H */
