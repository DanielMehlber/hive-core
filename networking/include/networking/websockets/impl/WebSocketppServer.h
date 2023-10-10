#ifndef WEBSOCKETPPSERVER_H
#define WEBSOCKETPPSERVER_H

#include <jobsystem/JobSystem.h>
#include <map>
#include <networking/websockets/IWebSocketServer.h>
#include <properties/PropertyProvider.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace networking::websockets {

typedef websocketpp::server<websocketpp::config::asio> server;

/**
 * @brief This implementation of IWebSocketServer uses WebSocket++ to provide
 * web sockets
 */
class WebSocketppServer : public IWebSocketServer {
private:
  /**
   * @brief The job system is required to react to received messages
   */
  jobsystem::SharedJobManager m_job_manager;

  /**
   * @brief Provides property values and configuration that is required for the
   * setup of the server and socket connections
   */
  props::SharedPropertyProvider m_property_provider;

  /**
   * @brief maps type names to their consumers
   */
  std::map<std::string, std::weak_ptr<IWebSocketMessageConsumer>> m_consumers;

  /**
   * @brief server endpoint provided by WebSocket++
   */
  server m_server_endpoint;

  /**
   * @brief maps connected URIs to their connections handlers. A connection
   * handler is a weak_ptr to the actual connection and can also expire during
   * runtime.
   */
  std::map<std::string, websocketpp::connection_hdl> m_connections;

  // Event handlers
  void OnConnectionOpened(websocketpp::connection_hdl conn);
  void OnConnectionClosed(websocketpp::connection_hdl conn);
  void OnMessageReceived(websocketpp::connection_hdl conn,
                         server::message_ptr msg);

public:
  WebSocketppServer(jobsystem::SharedJobManager job_manager,
                    props::SharedPropertyProvider properties);

  virtual void
  AddConsumer(std::weak_ptr<IWebSocketMessageConsumer> consumer) override;

  virtual std::optional<SharedWebSocketMessageConsumer>
  GetConsumerForType(const std::string &type_name) noexcept override;

  virtual std::future<void>
  Send(const std::string &uri,
       SharedWebSocketMessage message) noexcept override;

  virtual std::future<void>
  EstablishConnectionTo(const std::string &uri) noexcept override;

  virtual void CloseConnectionTo(const std::string &uri) noexcept override;
};
} // namespace networking::websockets

#endif /* WEBSOCKETPPSERVER_H */
