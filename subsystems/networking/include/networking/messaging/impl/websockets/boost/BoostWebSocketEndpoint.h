#pragma once

#include "BoostWebSocketConnection.h"
#include "BoostWebSocketConnectionEstablisher.h"
#include "BoostWebSocketConnectionListener.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/messaging/IMessageEndpoint.h"
#include <list>
#include <map>

namespace networking::messaging::websockets {

DECLARE_EXCEPTION(NoSuchPeerException);

/**
 * This implementation of IWebSocketServer uses WebSocket++ to provide
 * a web-socket communication peer.
 */
class BoostWebSocketEndpoint
    : public IMessageEndpoint,
      public common::memory::EnableBorrowFromThis<BoostWebSocketEndpoint> {
private:
  /**
   * Indicates if the web socket endpoint is currently running
   * @note The endpoint is not automatically initialized, except the
   * configuration says so.
   */
  bool m_running{false};
  mutable jobsystem::recursive_mutex m_running_mutex;

  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;
  common::config::SharedConfiguration m_config;

  /**
   * Used for clean-up jobs because they are kicked in the constructor, where
   * shared_from_this() is not available yet.
   */
  std::shared_ptr<BoostWebSocketEndpoint *> m_this_pointer;

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
  mutable jobsystem::recursive_mutex m_connections_mutex;

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
   * @param connection_info connection specification
   * @param stream web-socket stream
   * @param connection_promise promise to fulfill when the connection is
   * registered and ready to use.
   */
  void AddConnection(ConnectionInfo connection_info, stream_type &&stream);

  std::optional<SharedBoostWebSocketConnection>
  GetConnection(const std::string &connection_id);

  void ProcessReceivedMessage(std::string data,
                              SharedBoostWebSocketConnection over_connection);

  void InitAndStartConnectionListener();
  void InitConnectionEstablisher();

  void SetupCleanUpJob();

  void OnConnectionClose(const std::string &id);

public:
  BoostWebSocketEndpoint(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      const common::config::SharedConfiguration &config);
  virtual ~BoostWebSocketEndpoint();

  void AddMessageConsumer(std::weak_ptr<IMessageConsumer> consumer) override;

  std::list<SharedMessageConsumer>
  GetConsumersOfMessageType(const std::string &type_name) noexcept override;

  std::future<void> Send(const std::string &uri,
                         SharedMessage message) override;

  std::future<ConnectionInfo>
  EstablishConnectionTo(const std::string &uri) noexcept override;

  void CloseConnectionTo(const std::string &connection_id) noexcept override;

  std::future<size_t>
  IssueBroadcastAsJob(const SharedMessage &message) override;

  bool
  HasConnectionTo(const std::string &connection_id) const noexcept override;

  size_t GetActiveConnectionCount() const override;
};
} // namespace networking::messaging::websockets
