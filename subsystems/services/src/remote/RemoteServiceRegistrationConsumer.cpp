#include "services/registry/impl/remote/RemoteServiceRegistrationConsumer.h"
#include "logging/LogManager.h"
#include "networking/messaging/ConnectionInfo.h"
#include "services/executor/impl/RemoteServiceExecutor.h"

using namespace hive::services::impl;
using namespace hive::networking::messaging;

RemoteServiceRegistrationConsumer::RemoteServiceRegistrationConsumer(
    std::function<void(SharedServiceExecutor)> consumer,
    std::weak_ptr<RemoteServiceResponseConsumer> response_consumer,
    common::memory::Reference<IMessageEndpoint> messaging_endpoint)
    : m_consumer(std::move(consumer)),
      m_response_consumer(std::move(response_consumer)),
      m_message_endpoint(std::move(messaging_endpoint)) {}

void RemoteServiceRegistrationConsumer::ProcessReceivedMessage(
    SharedMessage received_message, ConnectionInfo connection_info) {

  DEBUG_ASSERT(received_message != nullptr,
               "received message should not be null")

  RemoteServiceRegistrationMessage registration_message(received_message);
  auto service_id = registration_message.GetId();
  auto service_name = registration_message.GetServiceName();
  auto capacity = registration_message.GetCapacity();

  DEBUG_ASSERT(!service_id.empty(), "remote service id should not be empty")
  DEBUG_ASSERT(!service_name.empty(), "remote service name should not be empty")

  LOG_INFO("received web-socket service registration of service '"
           << service_name << "' (" << service_id << ") from host "
           << connection_info.endpoint_id)

  SharedServiceExecutor stub = std::make_shared<RemoteServiceExecutor>(
      service_name, m_message_endpoint, connection_info, service_id,
      m_response_consumer, capacity);

  m_consumer(stub);
}
