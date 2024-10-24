#pragma once

#include "common/config/Configuration.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "networking/messaging/IMessageConsumer.h"
#include "networking/messaging/IMessageEndpoint.h"
#include "networking/messaging/impl/websockets/boost/BoostWebSocketEndpoint.h"
#include <memory>

namespace hive::networking {

typedef messaging::websockets::BoostWebSocketEndpoint
    DefaultMessageEndpointImpl;

/**
 * Central networking manager that provides connectivity for higher level
 * subsystems.
 */
class NetworkingManager
    : public common::memory::EnableBorrowFromThis<NetworkingManager> {
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;
  common::config::SharedConfiguration m_config;

  /** Local message-oriented endpoint for communication with other nodes */
  common::memory::Reference<messaging::IMessageEndpoint> m_message_endpoint;

  /** maps message type names to their consumers */
  std::map<std::string, std::list<std::weak_ptr<messaging::IMessageConsumer>>>
      m_consumers;
  mutable jobsystem::mutex m_consumers_mutex;

  void StartMessageEndpointServer();
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
   */
  void ProcessMessage(const messaging::SharedMessage &message,
                      const messaging::ConnectionInfo &info);
};

} // namespace hive::networking
