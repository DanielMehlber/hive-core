#pragma once

#include "BoostWebSocketConnection.h"
#include "common/config/Configuration.h"
#include "common/exceptions/ExceptionsBase.h"
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <functional>
#include <memory>

namespace hive::networking::messaging::websockets {

DECLARE_EXCEPTION(WebSocketTcpServerException);

/**
 * Listens to incoming connections and upgrades them to the web-socket
 * protocol.
 * @note This is basically the server-role
 */
class BoostWebSocketConnectionListener
    : public std::enable_shared_from_this<BoostWebSocketConnectionListener> {
private:
  /**
   * This is used as a callback function: A successfully established
   * connection will be passed into this function for further use.
   */
  std::function<void(ConnectionInfo, stream_type &&)> m_connection_consumer;

  /**
   * Used to create new execution strands for asynchronous operations
   */
  std::shared_ptr<boost::asio::io_context> m_execution_context;

  /**
   * Configuration of this connection listener and possibly other subsystems
   */
  common::config::SharedConfiguration m_config;

  /**
   * Purpose of this component is listening and accepting incoming
   * connections
   */
  std::unique_ptr<boost::asio::ip::tcp::acceptor>
      m_incoming_tcp_connection_acceptor;

  /**
   * Defines the endpoint configuration of this host (port, address,
   * etc.)
   */
  const std::shared_ptr<boost::asio::ip::tcp::endpoint> m_local_endpoint;

  /** UUID of this node in the hive required for node handshake */
  const std::string m_this_node_uuid;

  /**
   * After a TCP connection has been established with a client, the
   * web-socket handshake must commence to upgrade the protocol.
   * @note This is step 2 of the connection process
   * @param error_code indicating success of establishing TCP connection
   * @param socket new TCP socket that will be used in further steps
   */
  void ProcessTcpConnection(boost::beast::error_code error_code,
                            boost::asio::ip::tcp::socket socket);

  /**
   * Performs web-socket handshake using an existing TCP connection
   * @param plain_tcp_stream established TCP connection
   */
  void PerformWebSocketHandshake(std::shared_ptr<stream_type> plain_tcp_stream);

  /**
   * After the web-socket handshake has been performed, the resulting
   * connection must be processed for further use.
   * @note This is step 3 of the connection process
   * @param web_socket_stream TCP connection over which the web-socket handshake
   * has been performed
   * @param ec error code indicating the handshake's success
   */
  void ProcessWebSocketHandshake(std::shared_ptr<stream_type> web_socket_stream,
                                 boost::beast::error_code ec);

  void ProcessNodeHandshakeRequest(
      std::shared_ptr<stream_type> web_socket_stream,
      ConnectionInfo connection_info,
      std::shared_ptr<boost::beast::flat_buffer> handhake_request_buffer,
      boost::beast::error_code ec, std::size_t bytes_transferred);

public:
  BoostWebSocketConnectionListener(
      std::string this_node_uuid,
      std::shared_ptr<boost::asio::io_context> execution_context,
      common::config::SharedConfiguration config,
      std::shared_ptr<boost::asio::ip::tcp::endpoint> local_endpoint,
      std::function<void(ConnectionInfo, stream_type &&)> connection_consumer);

  ~BoostWebSocketConnectionListener();

  /**
   * Initializes listener and further components which are necessary for
   * accepting TCP connections.
   * @attention After this call, this endpoint does provide an open port for TCP
   * connections, but it does not accept/process them yet.
   */
  void Init();

  /**
   * Asynchronously accept and wait for new TCP connection to establish
   * Web-socket connections.
   * @note This must be called in order to incoming connections to be processed
   * and eventually converted into web-socket connections.
   */
  void StartAcceptingAnotherConnection();

  /**
   * Shut down connection listener. No more connections can be accepted
   * after.
   */
  void ShutDown();
};
} // namespace hive::networking::messaging::websockets
