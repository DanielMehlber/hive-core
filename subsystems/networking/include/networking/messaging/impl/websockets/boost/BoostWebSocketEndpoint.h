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

namespace hive::networking::messaging::websockets {

DECLARE_EXCEPTION(NoSuchEndpointException);

/**
 * This implementation of IWebSocketServer uses WebSocket++ to provide
 * a web-socket communication peer.
 */
class BoostWebSocketEndpoint
    : public IMessageEndpoint,
      public common::memory::EnableBorrowFromThis<BoostWebSocketEndpoint> {
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
   * Constructs a new connection object from a stream
   * @param connection_info connection specification
   * @param stream web-socket stream
   * registered and ready to use.
   */
  void AddConnection(const ConnectionInfo &connection_info,
                     stream_type &&stream);

  std::optional<SharedBoostWebSocketConnection>
  GetConnection(const std::string &connection_id);

  void
  ProcessReceivedMessage(const std::string &data,
                         const SharedBoostWebSocketConnection &over_connection);

  void InitAndStartConnectionListener();
  void InitConnectionEstablisher();

  void SetupCleanUpJob();

  void OnConnectionClose(const std::string &id);

public:
  BoostWebSocketEndpoint(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      common::config::SharedConfiguration config);

  ~BoostWebSocketEndpoint() override;

  void Startup() override;
  void Shutdown() override;
  std::string GetProtocol() const override;

  std::future<void> Send(const std::string &node_id,
                         SharedMessage message) override;

  std::future<ConnectionInfo>
  EstablishConnectionTo(const std::string &uri) override;

  void CloseConnectionTo(const std::string &node_id) override;

  std::future<size_t>
  IssueBroadcastAsJob(const SharedMessage &message) override;

  bool HasConnectionTo(const std::string &node_id) const override;

  size_t GetActiveConnectionCount() const override;
};
} // namespace hive::networking::messaging::websockets
