#ifndef BOOSTWEBSOCKETPEER_H
#define BOOSTWEBSOCKETPEER_H

#include "BoostWebSocketConnection.h"
#include "BoostWebSocketConnectionEstablisher.h"
#include "BoostWebSocketConnectionListener.h"
#include "jobsystem/JobSystemFactory.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/Networking.h"
#include "networking/websockets/IWebSocketPeer.h"
#include "properties/PropertyProvider.h"
#include <atomic>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <list>
#include <map>

namespace networking::websockets {

DECLARE_EXCEPTION(NoSuchPeerException);

/**
 * @brief This implementation of IWebSocketServer uses WebSocket++ to provide
 * a web-socket communication peer.
 */
class NETWORKING_API BoostWebSocketPeer : public IWebSocketPeer {
private:
  /**
   * @brief Indicates if the web socket peer is currently running
   */
  bool m_running{false};
  mutable std::mutex m_running_mutex;

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
  mutable std::mutex m_consumers_mutex;

  /**
   * @brief Acts as execution environment for asynchronous operations, such as
   * receiving messages
   */
  std::shared_ptr<boost::asio::io_context> m_execution_context;

  /**
   * @brief Maps host addresses to the connection established with the host.
   */
  std::map<std::string, SharedBoostWebSocketConnection> m_connections;
  mutable std::mutex m_connections_mutex;

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

  void AddConsumer(std::weak_ptr<IWebSocketMessageConsumer> consumer) override;

  std::list<SharedWebSocketMessageConsumer>
  GetConsumersOfType(const std::string &type_name) noexcept override;

  std::future<void> Send(const std::string &uri,
                         SharedWebSocketMessage message) override;

  std::future<void>
  EstablishConnectionTo(const std::string &uri) noexcept override;

  void CloseConnectionTo(const std::string &uri) noexcept override;

  bool HasConnectionTo(const std::string &uri) const noexcept;
};
} // namespace networking::websockets

#endif /* BOOSTWEBSOCKETPEER_H */
