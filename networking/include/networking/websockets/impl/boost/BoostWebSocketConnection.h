#ifndef BOOSTWEBSOCKETCONNECTION_H
#define BOOSTWEBSOCKETCONNECTION_H

#include "networking/Networking.h"
#include "networking/websockets/WebSocketConnectionInfo.h"
#include "networking/websockets/WebSocketMessage.h"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <common/exceptions/ExceptionsBase.h>
#include <future>
#include <memory>

namespace networking::websockets {

typedef boost::beast::websocket::stream<boost::beast::tcp_stream> stream_type;

DECLARE_EXCEPTION(ConnectionClosedException);
DECLARE_EXCEPTION(MessageSendingException);

class NETWORKING_API BoostWebSocketConnection
    : public std::enable_shared_from_this<BoostWebSocketConnection> {
private:
  /**
   * @brief TCP stream that allows interaction with the communication partner.
   */
  stream_type m_socket;

  /**
   * @brief Buffer for received messages
   */
  boost::beast::flat_buffer m_buffer;

  /**
   * @brief Will be called to pass the received data on for further processing
   */
  std::function<void(const std::string &,
                     std::shared_ptr<BoostWebSocketConnection>)>
      m_on_message_received;

  /**
   * @brief Data describing the remote endpoint of this connection
   */
  boost::asio::ip::tcp::endpoint m_remote_endpoint_data;

  /**
   * @brief Waits for message asynchronously and processes it on arrival
   */
  void AsyncReceiveMessage();

  /**
   * @brief Callback function that will be called when a message has been
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
   * @param error_code status indicating the success of sending the message
   * @param bytes_transferred number of bytes that have been sent
   */
  void OnMessageSent(std::promise<void> &&promise,
                     SharedWebSocketMessage message,
                     boost::beast::error_code error_code,
                     [[maybe_unused]] std::size_t bytes_transferred);

public:
  /**
   * @brief Constructs a new web-socket connection handler
   * @param socket TCP stream which is used for the web-socket connection
   * @param on_message_received callback function on when a message has been
   * received
   * @attention The newly constructed connection won't automatically listen to
   * incoming messages. This can be started by calling StartReceivingMessages.
   */
  BoostWebSocketConnection(
      stream_type &&socket,
      std::function<void(const std::string &,
                         std::shared_ptr<BoostWebSocketConnection>)>
          on_message_received);

  /**
   * Shuts down the connection
   */
  ~BoostWebSocketConnection();

  /**
   * @brief Lets connection listen for incoming messages and processes them on
   * arrival
   * @attention Without this call, the connection won't be able to process
   * incoming messages
   */
  void StartReceivingMessages();

  /**
   * @brief Closes the web-socket connection and the underlying tcp stream
   */
  void Close();

  /**
   * @brief Sends a message to the remote peer through this connection.
   * @param message web-socket message that will be sent
   * @return future indicating that the message has been sent
   */
  std::future<void> Send(const SharedWebSocketMessage &message);

  /**
   * @return address of the connected remote endpoint
   */
  std::string GetRemoteHostAddress() const;

  /**
   * Checks if the connected endpoint is reachable, i.e. if the connection is
   * still standing.
   * @return true, if the connection can be used for sending/receiving messages
   */
  bool IsUsable() const;

  /**
   * Generates connection info object from current connection data
   * @return connection info object
   */
  WebSocketConnectionInfo GetConnectionInfo() const;
};

inline std::string BoostWebSocketConnection::GetRemoteHostAddress() const {
  return m_remote_endpoint_data.address().to_string() + ":" +
         std::to_string(m_remote_endpoint_data.port());
}

inline bool BoostWebSocketConnection::IsUsable() const {
  return m_socket.is_open();
}

typedef std::shared_ptr<BoostWebSocketConnection>
    SharedBoostWebSocketConnection;

} // namespace networking::websockets

#endif /* BOOSTWEBSOCKETCONNECTION_H */
