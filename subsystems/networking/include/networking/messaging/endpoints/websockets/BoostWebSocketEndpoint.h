#pragma once

#include "common/config/Configuration.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "networking/messaging/endpoints/IMessageEndpoint.h"

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
   * Pointer to implementation (Pimpl) in source file. This is necessary to
   * constrain Boost's implementation details in the source file and not expose
   * them to the rest of the application.
   * @note Indispensable for ABI stability and to use static-linked Boost.
   */
  struct Impl;
  std::unique_ptr<Impl> m_impl;

  void InitAndStartConnectionListener();
  void InitConnectionEstablisher();

  void SetupCleanUpJob();

public:
  BoostWebSocketEndpoint(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      const common::config::SharedConfiguration &config);

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
