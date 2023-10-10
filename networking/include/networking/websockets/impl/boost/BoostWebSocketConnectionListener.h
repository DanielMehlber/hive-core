#ifndef BOOSTWEBSOCKETCONNECTIONLISTENER_H
#define BOOSTWEBSOCKETCONNECTIONLISTENER_H

#include "BoostWebSocketConnection.h"
#include <boost/beast.hpp>
#include <common/exceptions/ExceptionsBase.h>
#include <functional>
#include <memory>
#include <properties/PropertyProvider.h>

namespace networking::websockets {

DECLARE_EXCEPTION(WebSocketTcpServerException);

/**
 * @brief Listens to incoming connections and upgrades them to the web-socket
 * protocol.
 * @note This is basically the server-role
 */
class BoostWebSocketConnectionListener
    : public std::enable_shared_from_this<BoostWebSocketConnectionListener> {
private:
  /**
   * @brief This is used as a callback function: A successfully established
   * connection will be passed into this function for further use.
   */
  std::function<void(std::string, stream_type &&)> m_connection_consumer;

  /**
   * @brief Used to create new execution strands for asynchronous operations
   */
  std::shared_ptr<boost::asio::io_context> m_execution_context;

  props::SharedPropertyProvider m_properties;

  /**
   * @brief Purpose of this component is listening and accepting incoming
   * connections
   */
  std::unique_ptr<boost::asio::ip::tcp::acceptor>
      m_incoming_connection_acceptor;

  /**
   * @brief Defines the endpoint configuration of this host (port, address,
   * etc.)
   */
  boost::asio::ip::tcp::endpoint m_this_host_endpoint;

  /**
   * @brief After a TCP connection has been established with a client, the
   * web-socket handshake must commence to upgrade the protocol.
   * @note This is step 2 of the connection process
   * @param error_code indicating success of establishing TCP connection
   * @param socket new TCP socket that will be used in further steps
   */
  void ProcessTcpConnection(boost::beast::error_code error_code,
                            boost::asio::ip::tcp::socket socket);

  /**
   * @brief Performs web-socket handshake using an existing TCP connection
   * @param current_stream established TCP connection
   */
  void PerformWebSocketHandshake(std::shared_ptr<stream_type> current_stream);

  /**
   * @brief After the web-socket handshake has been performed, the resulting
   * connection must be processed for further use.
   * @note This is step 3 of the connection process
   * @param current_stream TCP connection over which the web-socket handshake
   * has been performed
   * @param ec error code indicating the handshake's success
   */
  void ProcessWebSocketHandshake(std::shared_ptr<stream_type> current_stream,
                                 boost::beast::error_code ec);

public:
  BoostWebSocketConnectionListener(
      std::shared_ptr<boost::asio::io_context> execution_context,
      props::SharedPropertyProvider property_provider,
      std::function<void(std::string, stream_type &&)> connection_consumer);

  ~BoostWebSocketConnectionListener();

  /**
   * @brief Initializes listener and further components which are necessary for
   * accepting TCP connections.
   * @attention After this call, this endpoint does provide an open port for TCP
   * connections, but it does not accept/process them yet.
   */
  void Init();

  /**
   * @brief Asynchronously accept and wait for new TCP connection to establish
   * Web-socket connections.
   * @note This must be called in order to incoming connections to be processed
   * and eventually converted into web-socket connections.
   */
  void StartAcceptingAnotherConnection();

  /**
   * @brief Shut down connection listener. No more connections can be accepted
   * after.
   */
  void ShutDown();
};
} // namespace networking::websockets

#endif /* BOOSTWEBSOCKETCONNECTIONLISTENER_H */
