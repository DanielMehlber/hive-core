#ifndef WEBSOCKETSERVICEREGISTRY_H
#define WEBSOCKETSERVICEREGISTRY_H

#include "networking/websockets/IWebSocketPeer.h"
#include "services/registry/IServiceRegistry.h"
#include "services/registry/impl/websockets/WebSocketServiceRegistrationConsumer.h"
#include "services/registry/impl/websockets/WebSocketServiceRequestConsumer.h"
#include "services/registry/impl/websockets/WebSocketServiceResponseConsumer.h"

namespace services::impl {

class WebSocketServiceRegistry : public IServiceRegistry {
protected:
  mutable std::mutex m_registered_callers_mutex;
  std::map<std::string, SharedServiceCaller> m_registered_callers;

  std::weak_ptr<IWebSocketPeer> m_web_socket_peer;
  jobsystem::SharedJobManager m_job_manager;

  SharedWebSocketMessageConsumer m_registration_consumer;
  SharedWebSocketMessageConsumer m_response_consumer;
  SharedWebSocketMessageConsumer m_request_consumer;

public:
  explicit WebSocketServiceRegistry(
      const SharedWebSocketPeer &web_socket_peer,
      const jobsystem::SharedJobManager &job_manager);

  void Register(const SharedServiceStub &stub) override;

  void Unregister(const std::string &name) override;

  std::future<std::optional<SharedServiceCaller>>
  Find(const std::string &name, bool only_local) noexcept override;
};

} // namespace services::impl

#endif // WEBSOCKETSERVICEREGISTRY_H
