#include "services/registry/impl/websockets/WebSocketServiceRegistrationConsumer.h"
#include "logging/LogManager.h"
#include "networking/websockets/WebSocketConnectionInfo.h"
#include "services/stub/impl/WebSocketServiceStub.h"

using namespace services::impl;

WebSocketServiceRegistrationConsumer::WebSocketServiceRegistrationConsumer(
    std::function<void(SharedServiceStub)> consumer,
    std::weak_ptr<WebSocketServiceResponseConsumer> response_consumer,
    std::weak_ptr<IWebSocketPeer> web_socket_peer)
    : m_consumer(std::move(consumer)),
      m_response_consumer(std::move(response_consumer)),
      m_web_socket_peer(std::move(web_socket_peer)) {}

void WebSocketServiceRegistrationConsumer::ProcessReceivedMessage(
    SharedWebSocketMessage received_message,
    WebSocketConnectionInfo connection_info) noexcept {

  WebSocketServiceRegistrationMessage registration_message(received_message);
  LOG_INFO("received web-socket service registration of service '"
           << registration_message.GetServiceName() << "' from host "
           << connection_info.GetHostname());

  SharedServiceStub stub = std::make_shared<WebSocketServiceStub>(
      registration_message.GetServiceName(), m_web_socket_peer,
      connection_info.GetHostname(), m_response_consumer);

  m_consumer(stub);
}
