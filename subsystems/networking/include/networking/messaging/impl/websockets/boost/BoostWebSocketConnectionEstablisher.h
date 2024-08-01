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

  void ProcessResolvedHostnameOfServer(
      std::string uri, std::promise<ConnectionInfo> &&connection_promise,
      boost::beast::error_code error_code,
      boost::asio::ip::tcp::resolver::results_type results);

  void ProcessEstablishedTcpConnection(
      std::promise<ConnectionInfo> &&connection_promise, std::string uri,
      std::shared_ptr<stream_type> stream, boost::beast::error_code error_code,
      boost::asio::ip::tcp::resolver::results_type::endpoint_type
          endpoint_type);

  void ProcessWebSocketHandshake(
      std::promise<ConnectionInfo> &&connection_promise, std::string uri,
      std::shared_ptr<stream_type> stream, boost::beast::error_code error_code);

  void PerformNodeHandshake(std::promise<ConnectionInfo> &&connection_promise,
                            ConnectionInfo &&info,
                            std::shared_ptr<stream_type> stream);

  void ProcessNodeHandshakeResponse(
      std::promise<ConnectionInfo> &&connection_promise, ConnectionInfo &&info,
      std::shared_ptr<stream_type> stream,
      std::shared_ptr<boost::beast::flat_buffer> response_buffer,
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
