#ifndef BOOSTWEBSOCKETPEER_H
#define BOOSTWEBSOCKETPEER_H

#include "BoostWebSocketConnection.h"
#include "BoostWebSocketConnectionEstablisher.h"
#include "BoostWebSocketConnectionListener.h"
#include "common/subsystems/SubsystemManager.h"
#include "jobsystem/JobSystemFactory.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/messaging/IMessageEndpoint.h"
#include "properties/PropertyProvider.h"
#include <atomic>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <list>
#include <map>

namespace networking::websockets {

DECLARE_EXCEPTION(NoSuchPeerException);

/**
 * This implementation of IWebSocketServer uses WebSocket++ to provide
 * a web-socket communication peer.
 */
class BoostWebSocketEndpoint
    : public IMessageEndpoint,
      public std::enable_shared_from_this<BoostWebSocketEndpoint> {
private:
  /**
   * Indicates if the web socket endpoint is currently running
   * @note The endpoint is not automatically initialized, except the
   * configuration says so.
   */
  bool m_running{false};
  mutable jobsystem::mutex m_running_mutex;

  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

  common::config::SharedConfiguration m_config;

  /**
   * maps message type names to their consumers
   */
  std::map<std::string, std::list<std::weak_ptr<IMessageConsumer>>> m_consumers;
  mutable jobsystem::mutex m_consumers_mutex;

  /**
   * Acts as execution environment for asynchronous operations, such as
   * receiving events
   */
  std::shared_ptr<boost::asio::io_context> m_execution_context;

  /**
   * Maps host addresses to the connection established with the host.
   */
  std::map<std::string, SharedBoostWebSocketConnection> m_connections;
  mutable jobsystem::mutex m_connections_mutex;

  std::shared_ptr<BoostWebSocketConnectionEstablisher> m_connection_establisher;
  std::shared_ptr<BoostWebSocketConnectionListener> m_connection_listener;

  /**
   * Thread pool that executes asynchronous callbacks certain websocket
   * events
   */
  std::vector<std::thread> m_execution_threads;

  /**
   * This is the local endpoint over which the peer should communicate
   * with others (receive & send events).
   * @note This is important for the connection establishing and listening
   * process.
   */
  std::shared_ptr<boost::asio::ip::tcp::endpoint> m_local_endpoint;

  /**
   * Consumers are stored as expireable weak-pointers. When the actual
   * referenced consumer is destroyed, the list of consumers holds expired
   * pointers that can be removed.
   * @param type message type name of consumers to clean up
   */
  void CleanUpConsumersOfMessageType(const std::string &type) noexcept;

  /**
   * Constructs a new connection object from a stream
   * @param url id of connection
   * @param stream web-socket stream
   */
  void AddConnection(const std::string &url, stream_type &&stream);

  std::optional<SharedBoostWebSocketConnection>
  GetConnection(const std::string &uri);

  void ProcessReceivedMessage(std::string data,
                              SharedBoostWebSocketConnection over_connection);

  void InitAndStartConnectionListener();
  void InitConnectionEstablisher();

  void SetupCleanUpJob();

  void OnConnectionClose(const std::string &id);

public:
  BoostWebSocketEndpoint(
      const common::subsystems::SharedSubsystemManager &subsystems,
      const common::config::SharedConfiguration &config);
  virtual ~BoostWebSocketEndpoint();

  void AddMessageConsumer(std::weak_ptr<IMessageConsumer> consumer) override;

  std::list<SharedMessageConsumer>
  GetConsumersOfMessageType(const std::string &type_name) noexcept override;

  std::future<void> Send(const std::string &uri,
                         SharedMessage message) override;

  std::future<void>
  EstablishConnectionTo(const std::string &uri) noexcept override;

  void CloseConnectionTo(const std::string &uri) noexcept override;

  std::future<size_t> Broadcast(const SharedMessage &message) override;

  bool HasConnectionTo(const std::string &uri) const noexcept override;

  size_t GetActiveConnectionCount() const override;
};
} // namespace networking::websockets

#endif /* BOOSTWEBSOCKETPEER_H */
