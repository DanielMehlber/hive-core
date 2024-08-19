#pragma once

#include "networking/messaging/IMessageConsumer.h"
#include "networking/messaging/IMessageEndpoint.h"
#include "services/registry/IServiceRegistry.h"
#include "services/registry/impl/remote/RemoteServiceRegistrationMessage.h"
#include "services/registry/impl/remote/RemoteServiceResponseConsumer.h"

namespace hive::services::impl {

/**
 * Consumer for web-socket events that register remote services on the current
 * host.
 */
class RemoteServiceRegistrationConsumer
    : public networking::messaging::IMessageConsumer {
private:
  std::function<void(SharedServiceExecutor)> m_consumer;
  std::weak_ptr<RemoteServiceResponseConsumer> m_response_consumer;
  common::memory::Reference<networking::messaging::IMessageEndpoint>
      m_message_endpoint;

public:
  RemoteServiceRegistrationConsumer() = delete;
  explicit RemoteServiceRegistrationConsumer(
      std::function<void(SharedServiceExecutor)> consumer,
      std::weak_ptr<RemoteServiceResponseConsumer> response_consumer,
      common::memory::Reference<networking::messaging::IMessageEndpoint>
          messaging_endpoint);

  virtual ~RemoteServiceRegistrationConsumer() = default;

  std::string GetMessageType() const override;

  void ProcessReceivedMessage(
      networking::messaging::SharedMessage received_message,
      networking::messaging::ConnectionInfo connection_info) override;
};

inline std::string RemoteServiceRegistrationConsumer::GetMessageType() const {
  return MESSAGE_TYPE_REMOTE_SERVICE_REGISTRATION;
}
} // namespace hive::services::impl
