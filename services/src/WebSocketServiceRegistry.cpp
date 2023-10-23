#include "services/registry/impl/websockets/WebSocketServiceRegistry.h"
#include "services/caller/impl/RoundRobinServiceCaller.h"

using namespace services;
using namespace services::impl;
using namespace std::placeholders;

void WebSocketServiceRegistry::Register(const SharedServiceStub &stub) {

  std::string name = stub->GetServiceName();

  if (!m_registered_callers.contains(name)) {
    m_registered_callers[name] = std::make_shared<RoundRobinServiceCaller>();
  }

  m_registered_callers.at(name)->AddServiceStub(stub);
  LOG_DEBUG("new service '" << name
                            << "' has been registered in web-socket registry");
}

void WebSocketServiceRegistry::Unregister(const std::string &name) {
  if (m_registered_callers.contains(name)) {
    m_registered_callers.erase(name);
    LOG_DEBUG(
        "service '" << name
                    << "' has been unregistered from web-socket registry");
  }
}

std::future<std::optional<SharedServiceCaller>>
WebSocketServiceRegistry::Find(const std::string &name,
                               bool only_local) noexcept {
  std::promise<std::optional<SharedServiceCaller>> promise;
  std::future<std::optional<SharedServiceCaller>> future = promise.get_future();

  if (m_registered_callers.contains(name)) {
    SharedServiceCaller caller = m_registered_callers.at(name);
    if (caller->IsCallable()) {
      if (!(only_local) && caller->ContainsLocallyCallable()) {
        promise.set_value(caller);
      } else {
        LOG_WARN("service '"
                 << name
                 << "' is registered in web-socket registry, but not "
                    "locally callable");
        promise.set_value({});
      }
    } else {
      LOG_WARN("service '" << name
                           << "' was registered in web-socket registry, but is "
                              "not usable anymore");
      promise.set_value({});
    }
  } else {
    promise.set_value({});
  }

  return future;
}

WebSocketServiceRegistry::WebSocketServiceRegistry(
    const SharedWebSocketPeer &web_socket_peer,
    const jobsystem::SharedJobManager &job_manager)
    : m_web_socket_peer(web_socket_peer) {
  m_registration_consumer =
      std::make_shared<WebSocketServiceRegistrationConsumer>(
          std::bind(&WebSocketServiceRegistry::Register, this, _1));
  m_response_consumer = std::make_shared<WebSocketServiceResponseConsumer>();
  m_request_consumer = std::make_shared<WebSocketServiceRequestConsumer>(
      job_manager, std::bind(&WebSocketServiceRegistry::Find, this, _1, _2),
      web_socket_peer);

  web_socket_peer->AddConsumer(m_registration_consumer);
  web_socket_peer->AddConsumer(m_response_consumer);
  web_socket_peer->AddConsumer(m_request_consumer);
}
