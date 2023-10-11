#ifndef BOOSTWEBSOCKETCONNECTIONESTABLISHER_H
#define BOOSTWEBSOCKETCONNECTIONESTABLISHER_H

#include "BoostWebSocketConnection.h"
#include <boost/asio.hpp>
#include <common/exceptions/ExceptionsBase.h>
#include <future>
#include <properties/PropertyProvider.h>

namespace networking::websockets {

DECLARE_EXCEPTION(UrlMalformedException);
DECLARE_EXCEPTION(CannotResolveHostException);
DECLARE_EXCEPTION(ConnectionFailedException);

/**
 * @brief Tries to establish web-socket connections with servers or other
 * endpoints
 * @note This is basically the client-role
 */
class BoostWebSocketConnectionEstablisher
    : public std::enable_shared_from_this<BoostWebSocketConnectionEstablisher> {
private:
  std::shared_ptr<boost::asio::io_context> m_execution_context;

  /**
   * @brief Resolves IP addresses from given hostnames
   */
  boost::asio::ip::tcp::resolver m_resolver;

  /**
   * @brief This is used as a callback function: A successfully established
   * connection will be passed into this function for further use.
   */
  std::function<void(std::string uri, stream_type &&stream)>
      m_connection_consumer;

  void ProcessResolvedHostnameOfServer(
      std::string uri, std::promise<void> &&connection_promise,
      boost::beast::error_code error_code,
      boost::asio::ip::tcp::resolver::results_type results);

  void ProcessEstablishedConnectionToServer(
      std::promise<void> &&connection_promise, std::string uri,
      std::shared_ptr<stream_type> stream, boost::beast::error_code error_code,
      boost::asio::ip::tcp::resolver::results_type::endpoint_type
          endpoint_type);

  void ProcessWebSocketHandshake(std::promise<void> &&connection_promise,
                                 std::string uri,
                                 std::shared_ptr<stream_type> stream,
                                 boost::beast::error_code error_code);

public:
  BoostWebSocketConnectionEstablisher(
      std::shared_ptr<boost::asio::io_context> execution_context,
      props::SharedPropertyProvider provider,
      std::function<void(std::string, stream_type &&)> connection_consumer);

  /**
   * @brief Tries to establish connection to another endpoint host. This
   * triggers the overall connection process of the client.
   * @param uri URL of the remote endpoint
   * @return a future resolving when the overall connection-establishment
   * process has completed
   */
  std::future<void> EstablishConnectionTo(const std::string &uri);
};
} // namespace networking::websockets

#endif /* BOOSTWEBSOCKETCONNECTIONESTABLISHER_H */
