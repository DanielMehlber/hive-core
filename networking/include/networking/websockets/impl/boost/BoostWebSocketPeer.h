#ifndef BOOSTWEBSOCKETPEER_H
#define BOOSTWEBSOCKETPEER_H

#include "BoostWebSocketConnection.h"
#include "BoostWebSocketConnectionEstablisher.h"
#include "BoostWebSocketConnectionListener.h"
#include <atomic>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <jobsystem/JobSystem.h>
#include <list>
#include <map>
#include <networking/websockets/IWebSocketPeer.h>
#include <properties/PropertyProvider.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace networking::websockets {

typedef websocketpp::server<websocketpp::config::asio> server;

DECLARE_EXCEPTION(NoSuchPeerException);

/**
 * @brief This implementation of IWebSocketServer uses WebSocket++ to provide
 * a web-socket communication peer.
 */
class BoostWebSocketPeer : public IWebSocketPeer {
private:
  /**
   * @brief Indicates if the web socket peer is currently running
   */
  std::atomic_bool m_running{false};

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
   * @brief maps message type names to their consumers
   */
  std::map<std::string, std::list<std::weak_ptr<IWebSocketMessageConsumer>>>
      m_consumers;

  /**
   * @brief Acts as execution environment for asynchronous operations, such as
   * receiving messages
   */
  std::shared_ptr<boost::asio::io_context> m_execution_context;

  /**
   * @brief Maps host addresses to the connection established with the host.
   */
  std::map<std::string, SharedBoostWebSocketConnection> m_connections;
  std::mutex m_connections_mutex;

  std::shared_ptr<BoostWebSocketConnectionEstablisher> m_connection_establisher;
  std::shared_ptr<BoostWebSocketConnectionListener> m_connection_listener;

  /**
   * @brief Thread pool that executes asynchronous callbacks certain websocket
   * events
   */
  std::vector<std::thread> m_execution_threads;

  /**
   * @brief This is the local endpoint over which the peer should communicate
   * with others (receive & send messages).
   * @note This is important for the connection establishing and listening
   * process.
   */
  std::shared_ptr<boost::asio::ip::tcp::endpoint> m_local_endpoint;

  /**
   * @brief Consumers are stored as expireable weak-pointers. When the actual
   * referenced consumer is destroyed, the list of consumers holds expired
   * pointers that can be removed.
   * @param type message type name of consumers to clean up
   */
  void CleanUpConsumersOf(const std::string &type) noexcept;

  /**
   * @brief Constructs a new connection object from a stream
   * @param url id of connection
   * @param stream web-socket stream
   */
  void AddConnection(std::string url, stream_type &&stream);

  std::optional<SharedBoostWebSocketConnection>
  GetConnection(const std::string &uri);

  void ProcessReceivedMessage(std::string data,
                              SharedBoostWebSocketConnection over_connection);

  void InitAndStartConnectionListener();
  void InitConnectionEstablisher();

public:
  BoostWebSocketPeer(jobsystem::SharedJobManager job_manager,
                     props::SharedPropertyProvider properties);
  virtual ~BoostWebSocketPeer();

  virtual void
  AddConsumer(std::weak_ptr<IWebSocketMessageConsumer> consumer) override;

  virtual std::list<SharedWebSocketMessageConsumer>
  GetConsumersOfType(const std::string &type_name) noexcept override;

  virtual std::future<void>
  Send(const std::string &uri,
       SharedWebSocketMessage message) noexcept override;

  virtual std::future<void>
  EstablishConnectionTo(const std::string &uri) noexcept override;

  virtual void CloseConnectionTo(const std::string &uri) noexcept override;
};
} // namespace networking::websockets

#endif /* BOOSTWEBSOCKETPEER_H */
