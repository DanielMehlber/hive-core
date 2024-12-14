#pragma once

#include "common/config/Configuration.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "jobsystem/synchronization/JobMutex.h"
#include "networking/messaging/ConnectionInfo.h"
#include "networking/messaging/Message.h"
#include "networking/messaging/consumer/IMessageConsumer.h"
#include "networking/messaging/converter/IMessageConverter.h"
#include "networking/messaging/endpoints/IMessageEndpoint.h"
#include "networking/messaging/endpoints/websockets/BoostWebSocketEndpoint.h"
#include <map>
#include <memory>

namespace hive::networking {

typedef messaging::websockets::BoostWebSocketEndpoint
    DefaultMessageEndpointImpl;

DECLARE_EXCEPTION(ProtocolNotSupportedException);
DECLARE_EXCEPTION(NoEndpointsException);

/**
 * Central networking manager that provides connectivity for higher level
 * subsystems.
 */
class NetworkingManager
    : public common::memory::EnableBorrowFromThis<NetworkingManager> {
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;
  common::config::SharedConfiguration m_config;

  /** maps message type names to their consumers */
  std::map<std::string, std::list<std::weak_ptr<messaging::IMessageConsumer>>>
      m_consumers;
  mutable jobsystem::mutex m_consumers_mutex;

  /** default protocol name */
  std::string m_default_endpoint_protocol;

  /** stores registered message endpoints according to their protocol */
  std::map<std::string,
           std::shared_ptr<common::memory::Owner<messaging::IMessageEndpoint>>>
      m_endpoints;
  mutable jobsystem::mutex m_endpoints_mutex;

  void StartDefaultEndpointImplementation();
  void ConfigureNode(const common::config::SharedConfiguration &config);
  void SetupConsumerCleanUpJob();

  /**
   * Consumers are stored as expiring weak-pointers. Once the actual consumer
   * has been destroyed (outside this component), its weak-pointer becomes
   * unusable and can be removed.
   * @param type message type name of consumers to clean up
   */
  void CleanUpConsumersOfMessageType(const std::string &type);

  /**
   * Consumers are stored as expiring weak-pointers. Once the actual consumer
   * has been destroyed (outside this component), its weak-pointer becomes
   * unusable and can be removed. This is done for all message types.
   */
  void CleanUpAllConsumers();

public:
  NetworkingManager(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      const common::config::SharedConfiguration &config);

  /**
   * Registers a consumer for a certain type of message. Incoming messages of
   * this type will be redirected to the consumer.
   * @attention Every type of message can only have a single consumer at a time,
   * so the type must be unique.
   * @param consumer consumer of specific type to add to the register
   * @throws DuplicateConsumerTypeException if there is already a consumer for
   * that type registered.
   */
  void AddMessageConsumer(std::weak_ptr<messaging::IMessageConsumer> consumer);

  /**
   * Tries to retrieve a valid consumer for the given message type (if
   * one is registered and has not expired yet)
   * @param type_name type name of the events that the consumer processes
   * @return a consumer if one has been found for the given type
   */
  std::list<messaging::SharedMessageConsumer>
  GetConsumersOfMessageType(const std::string &type_name);

  /**
   * Processes a message by forwarding it to the appropriate consumer.
   * @param message message to process
   * @param info connection information of the sender
   */
  void ProcessMessage(const messaging::SharedMessage &message,
                      const messaging::ConnectionInfo &info);

  /**
   * Installs a messaging endpoint implementation for a specific protocol.
   * @note The messaging endpoint will be initialized and started at
   * installation.
   * @attention If the protocol is already registered, the existing endpoint
   * will be shut down and replaced.
   * @param endpoint the messaging endpoint to install
   * @param is_default if true, this endpoint will be used as the default one.
   */
  void InstallMessageEndpoint(
      common::memory::Owner<messaging::IMessageEndpoint> &&endpoint,
      bool is_default = false);

  /**
   * Retrieves the messaging endpoint implementation for a specific protocol.
   * @param protocol short id or name of the underlying protocol.
   * @return the messaging endpoint implementation for the given protocol if a
   * matching one has been installed.
   */
  std::optional<common::memory::Borrower<messaging::IMessageEndpoint>>
  GetMessageEndpoint(const std::string &protocol) const;

  /**
   * Retrieves the default registered messaging endpoint implementation.
   * @return the default messaging endpoint implementation if one has been
   * installed.
   */
  std::optional<common::memory::Borrower<messaging::IMessageEndpoint>>
  GetDefaultMessageEndpoint() const;

  /**
   * Retrieves the default registered messaging endpoint implementation. If this
   * operation fails because there are no endpoints installed, an exception is
   * thrown.
   * @return the default messaging endpoint implementation.
   * @throws NoEndpointsException if no endpoints are currently installed.
   */
  common::memory::Borrower<messaging::IMessageEndpoint>
  RequireDefaultMessageEndpoint() const;

  /**
   * Sets the default messaging endpoint implementation for a specific protocol.
   * @param protocol short id or name of the underlying protocol.
   * @throws ProtocolNotSupportedException if no endpoint for the given protocol
   * is currently installed, it can't be set as default protocol.
   */
  void SetDefaultMessageEndpoint(const std::string &protocol);

  /**
   * Uninstalls a messaging endpoint implementation for a specific protocol
   * if it has been installed.
   * @note The messaging endpoint will be stopped and shut down at
   * uninstallation.
   * @param protocol short id or name of the underlying protocol.
   */
  void UninstallMessageEndpoint(const std::string &protocol);

  /**
   * Checks if a messaging endpoint implementation for a specific protocol is
   * currently installed and running.
   * @return true if the endpoint is installed and running.
   */
  bool SupportsMessagingProtocol(const std::string &protocol) const;

  /**
   * Retrieves a list of all supported messaging protocols.
   * @return list of supported messaging protocols.
   */
  std::vector<std::string> GetSupportedProtocols() const;

  /**
   * Checks if there are any messaging endpoints installed.
   * @return true if there are any messaging endpoints installed.
   */
  bool HasInstalledMessageEndpoints() const;

  /**
   * Retrieves some message endpoint which is connected to a certain node if one
   * exists.
   * @note default endpoint will be preferred if it is connected to the given
   * node. Otherwise, some other endpoint will be selected.
   * @return some message endpoint connected to the given node if one exists.
   */
  std::optional<common::memory::Borrower<messaging::IMessageEndpoint>>
  GetSomeMessageEndpointConnectedTo(const std::string &endpoint_id) const;
};

} // namespace hive::networking
