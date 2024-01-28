#include "services/registry/impl/remote/RemoteServiceRegistrationConsumer.h"
#include "logging/LogManager.h"
#include "networking/peers/ConnectionInfo.h"
#include "services/executor/impl/RemoteServiceExecutor.h"

using namespace services::impl;

RemoteServiceRegistrationConsumer::RemoteServiceRegistrationConsumer(
    std::function<void(SharedServiceExecutor)> consumer,
    std::weak_ptr<RemoteServiceResponseConsumer> response_consumer,
    std::weak_ptr<IMessageEndpoint> web_socket_peer)
    : m_consumer(std::move(consumer)),
      m_response_consumer(std::move(response_consumer)),
      m_web_socket_peer(std::move(web_socket_peer)) {}

void RemoteServiceRegistrationConsumer::ProcessReceivedMessage(
    SharedMessage received_message, ConnectionInfo connection_info) noexcept {

  RemoteServiceRegistrationMessage registration_message(received_message);
  LOG_INFO("received web-socket service registration of service '"
           << registration_message.GetServiceName() << "' from host "
           << connection_info.GetHostname());

  SharedServiceExecutor stub = std::make_shared<RemoteServiceExecutor>(
      registration_message.GetServiceName(), m_web_socket_peer,
      connection_info.GetHostname(), m_response_consumer);

  m_consumer(stub);
}
