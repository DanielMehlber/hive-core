#include "services/registry/impl/remote/RemoteServiceRegistry.h"
#include "common/assert/Assert.h"
#include "networking/peers/events/ConnectionEstablishedEvent.h"
#include "services/caller/impl/RoundRobinServiceCaller.h"
#include "services/messages/ServiceRegisteredNotification.h"
#include "services/messages/ServiceUnegisteredNotification.h"

using namespace services;
using namespace services::impl;
using namespace std::placeholders;

void broadcastServiceRegistration(
    const std::weak_ptr<IPeer> &sender, const SharedServiceExecutor &stub,
    const jobsystem::SharedJobManager &job_manager) {

  ASSERT(!sender.expired(), "sender should exist")
  ASSERT(stub->IsCallable(), "executor should be callable")

  SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [sender, stub](jobsystem::JobContext *context) {
        if (sender.expired()) {
          LOG_WARN("cannot broadcast local service "
                   << stub->GetServiceName()
                   << " because web-socket peer has been destroyed")
        } else {
          RemoteServiceRegistrationMessage registration;

          registration.SetServiceName(stub->GetServiceName());
          std::future<size_t> progress =
              sender.lock()->Broadcast(registration.GetMessage());

          context->GetJobManager()->WaitForCompletion(progress);
          try {
            size_t receivers = progress.get();
            LOG_DEBUG("broadcast local service " << stub->GetServiceName()
                                                 << " to " << receivers
                                                 << " other peers")
          } catch (std::exception &exception) {
            LOG_ERR("broadcasting local service "
                    << stub->GetServiceName()
                    << " failed due to internal error: " << exception.what())
          }
        }

        return JobContinuation::DISPOSE;
      });

  job_manager->KickJob(job);
}

void RemoteServiceRegistry::Register(const SharedServiceExecutor &stub) {

  ASSERT(stub->IsCallable(), "executor should be callable")

  auto job_manager =
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();
  auto web_socket_peer = m_subsystems.lock()->RequireSubsystem<IPeer>();

  std::string name = stub->GetServiceName();

  std::unique_lock lock(m_registered_callers_mutex);
  if (!m_registered_callers.contains(name)) {
    m_registered_callers[name] = std::make_shared<RoundRobinServiceCaller>();
  }

  if (stub->IsLocal()) {
    broadcastServiceRegistration(web_socket_peer, stub, job_manager);
  }

  m_registered_callers.at(name)->AddExecutor(stub);
  LOG_DEBUG("new service '" << name
                            << "' has been registered in web-socket registry")

  // send notification that new service has been registered
  if (m_subsystems.lock()->ProvidesSubsystem<events::IEventBroker>()) {
    auto broker = m_subsystems.lock()->RequireSubsystem<events::IEventBroker>();

    ServiceRegisteredNotification message;
    message.SetServiceName(name);
    broker->FireEvent(message.GetMessage());
  }
}

void RemoteServiceRegistry::Unregister(const std::string &name) {
  std::unique_lock lock(m_registered_callers_mutex);
  if (m_registered_callers.contains(name)) {
    m_registered_callers.erase(name);
    LOG_DEBUG("service '" << name
                          << "' has been unregistered from web-socket registry")
  }

  // send notification that new service has been registered
  if (m_subsystems.lock()->ProvidesSubsystem<events::IEventBroker>()) {
    auto broker = m_subsystems.lock()->RequireSubsystem<events::IEventBroker>();

    ServiceUnregisteredNotification message;
    message.SetServiceName(name);
    broker->FireEvent(message.GetMessage());
  }
}

std::future<std::optional<SharedServiceCaller>>
RemoteServiceRegistry::Find(const std::string &name, bool only_local) noexcept {
  std::promise<std::optional<SharedServiceCaller>> promise;
  std::future<std::optional<SharedServiceCaller>> future = promise.get_future();

  std::unique_lock lock(m_registered_callers_mutex);
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
                      "locally callable")
          promise.set_value({});
        }
      }
      promise.set_value(caller);
    } else {
      LOG_WARN("service '" << name
                           << "' was registered in web-socket registry, but is "
                              "not usable anymore")
      promise.set_value({});
    }
  } else {
    promise.set_value({});
  }

  return future;
}

RemoteServiceRegistry::RemoteServiceRegistry(
    const common::subsystems::SharedSubsystemManager &subsystems)
    : m_subsystems(subsystems) {

  SetupMessageConsumers();
  SetupEventSubscribers();
}

void RemoteServiceRegistry::SetupEventSubscribers() {

  ASSERT(!m_subsystems.expired(), "subsystems should still exist")
  ASSERT(m_subsystems.lock()->ProvidesSubsystem<events::IEventBroker>(),
         "event broker subsystem should exist")

  auto event_system =
      m_subsystems.lock()->RequireSubsystem<events::IEventBroker>();

  m_new_peer_connection_listener =
      std::make_shared<events::FunctionalEventListener>(
          [this](const events::SharedEvent event) {
            networking::ConnectionEstablishedEvent established_event(event);
            auto peer_id = established_event.GetPeerId();
            SendServicePortfolioToPeer(peer_id);
          });

  // event_system->RegisterListener(m_new_peer_connection_listener,
  //                                "connection-established");
}

void RemoteServiceRegistry::SetupMessageConsumers() {

  ASSERT(!m_subsystems.expired(), "subsystems should still exist")
  ASSERT(m_subsystems.lock()->ProvidesSubsystem<IPeer>(),
         "peer networking subsystem should exist")

  auto web_socket_peer = m_subsystems.lock()->RequireSubsystem<IPeer>();

  auto response_consumer = std::make_shared<RemoteServiceResponseConsumer>();
  m_response_consumer = response_consumer;
  m_registration_consumer = std::make_shared<RemoteServiceRegistrationConsumer>(
      std::bind(&RemoteServiceRegistry::Register, this, _1), response_consumer,
      web_socket_peer);
  m_request_consumer = std::make_shared<RemoteServiceRequestConsumer>(
      m_subsystems.lock(),
      std::bind(&RemoteServiceRegistry::Find, this, _1, _2), web_socket_peer);

  web_socket_peer->AddConsumer(m_registration_consumer);
  web_socket_peer->AddConsumer(m_response_consumer);
  web_socket_peer->AddConsumer(m_request_consumer);
}

void RemoteServiceRegistry::SendServicePortfolioToPeer(
    const std::string &peer_id) const {

  ASSERT(!m_subsystems.expired(), "subsystems should still exist")
  ASSERT(m_subsystems.lock()->ProvidesSubsystem<IPeer>(),
         "peer networking subsystem should exist")
  ASSERT(m_subsystems.lock()->ProvidesSubsystem<jobsystem::JobManager>(),
         "job system should exist")

  if (!m_subsystems.lock()
           ->ProvidesSubsystem<networking::websockets::IPeer>()) {
    LOG_ERR("Cannot transmit service portfolio to remote peer "
            << peer_id << ": this peer cannot be found as subsystem");
    return;
  }

  std::weak_ptr<networking::websockets::IPeer> this_peer_weak =
      m_subsystems.lock()->RequireSubsystem<networking::websockets::IPeer>();

  auto job_manager =
      m_subsystems.lock()->RequireSubsystem<jobsystem::JobManager>();

  // create a registration message job for each registered service name
  std::unique_lock callers_lock(m_registered_callers_mutex);
  for (const auto &[service_name, caller] : m_registered_callers) {
    auto job = CreateRemoteServiceRegistrationJob(peer_id, service_name,
                                                  this_peer_weak);
    job_manager->KickJob(job);
  }
}

SharedJob RemoteServiceRegistry::CreateRemoteServiceRegistrationJob(
    const std::string &peer_id, const std::string &service_name,
    const std::weak_ptr<networking::websockets::IPeer> &sending_peer) {

  ASSERT(!sending_peer.expired(), "sending peer should exist")

  auto job = std::make_shared<Job>(
      [sending_peer, peer_id, service_name](jobsystem::JobContext *context) {
        if (sending_peer.expired()) {
          LOG_WARN("Cannot transmit service "
                   << service_name << " to remote peer " << peer_id
                   << " because peer subsystem has expired");
          return DISPOSE;
        }

        auto this_peer = sending_peer.lock();

        RemoteServiceRegistrationMessage registration;
        registration.SetServiceName(service_name);

        this_peer->Send(peer_id, registration.GetMessage());

        return DISPOSE;
      },
      "register-service-{" + service_name + "}-at-peer-{" + peer_id + "}",
      MAIN);
  return job;
}
