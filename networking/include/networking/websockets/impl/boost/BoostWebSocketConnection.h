#ifndef BOOSTWEBSOCKETCONNECTION_H
#define BOOSTWEBSOCKETCONNECTION_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <memory>

namespace networking::websockets {

typedef boost::beast::websocket::stream<boost::beast::tcp_stream> stream_type;

class BoostWebSocketConnection
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
                         std::size_t bytes_transferred);

  bool IsUsable() const;

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

  std::string GetRemoteHostAddress() const;
};

inline std::string BoostWebSocketConnection::GetRemoteHostAddress() const {
  return m_remote_endpoint_data.address().to_string();
}

inline bool BoostWebSocketConnection::IsUsable() const {
  return m_socket.is_open();
}

typedef std::shared_ptr<BoostWebSocketConnection>
    SharedBoostWebSocketConnection;

} // namespace networking::websockets

#endif /* BOOSTWEBSOCKETCONNECTION_H */
