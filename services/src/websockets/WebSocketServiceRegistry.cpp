#include "services/registry/impl/websockets/WebSocketServiceRegistry.h"
#include "services/caller/impl/RoundRobinServiceCaller.h"

using namespace services;
using namespace services::impl;
using namespace std::placeholders;

void broadcastServiceRegistration(std::weak_ptr<IWebSocketPeer> sender,
                                  SharedServiceStub stub,
                                  jobsystem::SharedJobManager job_manager) {
  SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [sender, stub](jobsystem::JobContext *context) {
        if (sender.expired()) {
          LOG_WARN("cannot broadcast local service "
                   << stub->GetServiceName()
                   << " because web-socket peer has been destroyed");
        } else {
          auto message = std::make_shared<WebSocketMessage>(
              MESSAGE_TYPE_WS_SERVICE_REGISTRATION);
          auto registration =
              std::make_shared<WebSocketServiceRegistrationMessage>(message);

          registration->SetServiceName(stub->GetServiceName());
          std::future<size_t> progress = sender.lock()->Broadcast(message);

          context->GetJobManager()->WaitForCompletion(progress);
          try {
            size_t receivers = progress.get();
            LOG_DEBUG("broadcast local service " << stub->GetServiceName()
                                                 << " to " << receivers
                                                 << " other peers");
          } catch (std::exception &exception) {
            LOG_ERR("broadcasting local service "
                    << stub->GetServiceName()
                    << " failed due to internal error: " << exception.what());
          }
        }

        return JobContinuation::DISPOSE;
      });

  job_manager->KickJob(job);
}

void WebSocketServiceRegistry::Register(const SharedServiceStub &stub) {

  std::string name = stub->GetServiceName();

  if (!m_registered_callers.contains(name)) {
    m_registered_callers[name] = std::make_shared<RoundRobinServiceCaller>();
  }

  if (stub->IsLocal()) {
    broadcastServiceRegistration(m_web_socket_peer, stub, m_job_manager);
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

    // check if this is callable at all
    if (caller->IsCallable()) {

      // if a local stub is required, check if the caller can provide one
      if (only_local) {
        if (!caller->ContainsLocallyCallable()) {
          LOG_WARN("service '"
                   << name
                   << "' is registered in web-socket registry, but not "
                      "locally callable");
          promise.set_value({});
        }
      }
      promise.set_value(caller);
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
    : m_web_socket_peer(web_socket_peer), m_job_manager(job_manager) {
  auto response_consumer = std::make_shared<WebSocketServiceResponseConsumer>();
  m_response_consumer = response_consumer;
  m_registration_consumer =
      std::make_shared<WebSocketServiceRegistrationConsumer>(
          std::bind(&WebSocketServiceRegistry::Register, this, _1),
          response_consumer, web_socket_peer);
  m_request_consumer = std::make_shared<WebSocketServiceRequestConsumer>(
      job_manager, std::bind(&WebSocketServiceRegistry::Find, this, _1, _2),
      web_socket_peer);

  web_socket_peer->AddConsumer(m_registration_consumer);
  web_socket_peer->AddConsumer(m_response_consumer);
  web_socket_peer->AddConsumer(m_request_consumer);
}
