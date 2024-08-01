#pragma once

#include "BoostWebSocketConnection.h"
#include "common/exceptions/ExceptionsBase.h"
#include "properties/PropertyProvider.h"
#include <boost/asio.hpp>
#include <future>

namespace networking::messaging::websockets {

DECLARE_EXCEPTION(UrlMalformedException);
DECLARE_EXCEPTION(CannotResolveHostException);
DECLARE_EXCEPTION(ConnectionFailedException);

/**
 * Tries to establish web-socket connections with servers or other
 * endpoints
 * @note This is basically the client-role
 */
class BoostWebSocketConnectionEstablisher
    : public std::enable_shared_from_this<BoostWebSocketConnectionEstablisher> {
private:
  std::shared_ptr<boost::asio::io_context> m_execution_context;

  /** Unique ID of this node / endpoint required for their handshake */
  std::string m_this_node_uuid;

  /**
   * Resolves IP addresses from given hostnames
   */
  boost::asio::ip::tcp::resolver m_resolver;

  /**
   * This is used as a callback function: A successfully established
   * connection will be passed into this function for further use.
   */
  std::function<void(ConnectionInfo, stream_type &&)> m_connection_consumer;

  /**
   * At first the given uri or hostname has to be resolved to an IP address.
   * This function will be called after the resolution attempt and handle the
   * result.
   * @param uri URI of the remote endpoint
   * @param connection_promise promise to resolve when the connection process
   * has completed. This will be passed through multiple functions until the end
   * of the process.
   * @param error_code result of the resolution attempt
   * @param results resolved IP addresses of the given hostname
   */
  void ProcessResolvedHostnameOfServer(
      std::string uri, std::promise<ConnectionInfo> &&connection_promise,
      boost::beast::error_code error_code,
      boost::asio::ip::tcp::resolver::results_type results);

  /**
   * A web-socket stream is upgraded from a plain TCP stream which has to be
   * established first. This function will be called after the attempt to
   * establish a stable TCP connection.
   * @param connection_promise promise to resolve when the connection process
   * has completed. This will be passed through multiple functions until the end
   * of the process.
   * @param uri URI of the remote endpoint
   * @param plain_tcp_stream possibly established TCP stream
   * @param error_code result of the TCP connection attempt
   * @param endpoint_type resolved IP address of the remote endpoint
   */
  void ProcessEstablishedTcpConnection(
      std::promise<ConnectionInfo> &&connection_promise, std::string uri,
      std::shared_ptr<stream_type> plain_tcp_stream,
      boost::beast::error_code error_code,
      boost::asio::ip::tcp::resolver::results_type::endpoint_type
          endpoint_type);

  /**
   * After the TCP connection has been established, a web-socket handshake is
   * performed to upgrade the connection.
   * @param connection_promise promise to resolve when the connection process
   * has completed. This will be passed through multiple functions until the end
   * of the process.
   * @param uri URI of the remote endpoint
   * @param web_socket_stream upgraded web-socket stream
   * @param error_code result of the web-socket handshake attempt
   */
  void
  ProcessWebSocketHandshake(std::promise<ConnectionInfo> &&connection_promise,
                            std::string uri,
                            std::shared_ptr<stream_type> web_socket_stream,
                            boost::beast::error_code error_code);

  /**
   * After the web-socket handshake has been performed and a stable stream
   * between both nodes has been established, a handshake is performed to
   * exchange additional information (like identification codes). This is
   * proprietary and not part of the web-socket protocol.
   * @param connection_promise promise to resolve when the connection process
   * has completed. This will be passed through multiple functions until the end
   * of the process.
   * @param info connection information
   * @param web_socket_stream established web-socket stream
   */
  void PerformNodeHandshake(std::promise<ConnectionInfo> &&connection_promise,
                            ConnectionInfo &&info,
                            std::shared_ptr<stream_type> web_socket_stream);

  void ProcessNodeHandshakeResponse(
      std::promise<ConnectionInfo> &&connection_promise, ConnectionInfo &&info,
      std::shared_ptr<stream_type> web_socket_stream,
      std::shared_ptr<boost::beast::flat_buffer> handshake_response_buffer,
      boost::beast::error_code error_code, std::size_t bytes_transferred);

public:
  BoostWebSocketConnectionEstablisher(
      std::string this_node_uuid,
      std::shared_ptr<boost::asio::io_context> execution_context,
      std::function<void(ConnectionInfo, stream_type &&)> connection_consumer);

  /**
   * Tries to establish connection to another endpoint host. This
   * triggers the overall connection process of the client.
   * @param uri URL of the remote endpoint
   * @return a future resolving when the overall connection-establishment
   * process has completed
   */
  std::future<ConnectionInfo> EstablishConnectionTo(const std::string &uri);
};
} // namespace networking::messaging::websockets
