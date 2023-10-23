#include "services/registry/impl/websockets/WebSocketServiceRegistrationConsumer.h"
#include "logging/LogManager.h"
#include "networking/websockets/WebSocketConnectionInfo.h"

using namespace services::impl;

WebSocketServiceRegistrationConsumer::WebSocketServiceRegistrationConsumer(
    std::function<void(SharedServiceStub)> consumer)
    : m_consumer(std::move(consumer)) {}

void WebSocketServiceRegistrationConsumer::ProcessReceivedMessage(
    SharedWebSocketMessage received_message,
    WebSocketConnectionInfo connection_info) noexcept {

  WebSocketServiceRegistrationMessage registration_message(received_message);
  LOG_INFO("received web-socket service registration of service '"
           << registration_message.GetServiceName() << "' on host "
           << connection_info.GetHostname());
}
