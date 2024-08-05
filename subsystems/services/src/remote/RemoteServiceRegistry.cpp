#include "services/registry/impl/remote/RemoteServiceRegistry.h"
#include "common/assert/Assert.h"
#include "networking/messaging/events/ConnectionEstablishedEvent.h"
#include "services/caller/impl/RoundRobinServiceCaller.h"
#include "services/messages/ServiceRegisteredNotification.h"
#include "services/messages/ServiceUnegisteredNotification.h"

using namespace hive;
using namespace hive::services;
using namespace hive::services::impl;
using namespace std::placeholders;

void broadcastServiceRegistration(
    common::memory::Reference<IMessageEndpoint> weak_sender,
    const SharedServiceExecutor &stub,
    common::memory::Borrower<jobsystem::JobManager> job_manager) {

  DEBUG_ASSERT(weak_sender.CanBorrow(), "sender should exist")
  DEBUG_ASSERT(stub->IsCallable(), "executor should be callable")
  DEBUG_ASSERT(stub->IsLocal(), "only local services can be offered to others")

  SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [weak_sender, stub](jobsystem::JobContext *context) mutable {
        if (auto maybe_sender = weak_sender.TryBorrow()) {
          auto sender = maybe_sender.value();
          RemoteServiceRegistrationMessage registration;
          registration.SetServiceName(stub->GetServiceName());
          registration.SetId(stub->GetId());

          std::future<size_t> progress =
              sender->IssueBroadcastAsJob(registration.GetMessage());

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
        } else {
          LOG_WARN("cannot broadcast local service "
                   << stub->GetServiceName()
                   << " because web-socket peer has been destroyed")
        }

        return JobContinuation::DISPOSE;
      },
      "broadcast-service-registration-" + stub->GetServiceName());

  job_manager->KickJob(job);
}

void RemoteServiceRegistry::Register(const SharedServiceExecutor &stub) {

  if (!stub->IsCallable()) {
    LOG_WARN("cannot register " << (stub->IsLocal() ? "local" : "remote")
                                << " service '" << stub->GetServiceName()
                                << "' because it is not callable (anymore)")
    return;
  }

  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();
    auto message_endpoint = subsystems->RequireSubsystem<IMessageEndpoint>();

    std::string name = stub->GetServiceName();

    std::unique_lock lock(m_registered_callers_mutex);
    if (!m_registered_callers.contains(name)) {
      m_registered_callers[name] = std::make_shared<RoundRobinServiceCaller>();
    }

    if (stub->IsLocal()) {
      broadcastServiceRegistration(message_endpoint.ToReference(), stub,
                                   job_manager);
    }

    m_registered_callers.at(name)->AddExecutor(stub);
    LOG_DEBUG("new service '" << name
                              << "' has been registered in web-socket registry")

    // send notification that new service has been registered
    if (subsystems->ProvidesSubsystem<events::IEventBroker>()) {
      auto broker = subsystems->RequireSubsystem<events::IEventBroker>();

      ServiceRegisteredNotification message;
      message.SetServiceName(name);
      broker->FireEvent(message.GetMessage());
    }
  } else {
    LOG_ERR("cannot perform registration in remote service registry because "
            "required subsystems are not available")
  }
}

void RemoteServiceRegistry::Unregister(const std::string &name) {
  std::unique_lock lock(m_registered_callers_mutex);
  if (m_registered_callers.contains(name)) {
    m_registered_callers.erase(name);
    LOG_DEBUG("service '" << name
                          << "' has been unregistered from web-socket registry")
  }

  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    // send notification that new service has been registered
    if (subsystems->ProvidesSubsystem<events::IEventBroker>()) {
      auto broker = subsystems->RequireSubsystem<events::IEventBroker>();

      ServiceUnregisteredNotification message;
      message.SetServiceName(name);
      broker->FireEvent(message.GetMessage());
    } else /* if event broker is not available */ {
      LOG_WARN("cannot send event notification about the un-registration of "
               "service '"
               << name
               << "' from the remote service registry: the event broker "
                  "subsystem is not available")
    }
  } else /* if subsystems in general are not available */ {
    LOG_WARN("cannot send event notification about the un-registration of "
             "service '"
             << name
             << "' from the remote service registry: required subsystems are "
                "not available or have been shut down")
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
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems)
    : m_subsystems(subsystems) {

  SetupMessageConsumers();
  SetupEventSubscribers();
}

void RemoteServiceRegistry::SetupEventSubscribers() {
  DEBUG_ASSERT(m_subsystems.CanBorrow(), "subsystems should still exist")
  auto subsystems = m_subsystems.Borrow();
  DEBUG_ASSERT(subsystems->ProvidesSubsystem<events::IEventBroker>(),
               "event broker subsystem should exist")

  auto event_system = subsystems->RequireSubsystem<events::IEventBroker>();

  m_new_peer_connection_listener =
      std::make_shared<events::FunctionalEventListener>(
          [this](const events::SharedEvent event) {
            networking::ConnectionEstablishedEvent established_event(event);
            auto peer_id = established_event.GetEndpointId();
            SendServicePortfolioToPeer(peer_id);
          });

  event_system->RegisterListener(m_new_peer_connection_listener,
                                 "connection-established");
}

void RemoteServiceRegistry::SetupMessageConsumers() {
  DEBUG_ASSERT(m_subsystems.CanBorrow(), "subsystems should still exist")
  DEBUG_ASSERT(m_subsystems.Borrow()->ProvidesSubsystem<IMessageEndpoint>(),
               "peer networking subsystem should exist")

  auto subsystems = m_subsystems.Borrow();
  auto web_socket_peer = subsystems->RequireSubsystem<IMessageEndpoint>();

  auto response_consumer =
      std::make_shared<RemoteServiceResponseConsumer>(m_subsystems);

  m_response_consumer = response_consumer;
  m_registration_consumer = std::make_shared<RemoteServiceRegistrationConsumer>(
      std::bind(&RemoteServiceRegistry::Register, this, _1), response_consumer,
      web_socket_peer.ToReference());
  m_request_consumer = std::make_shared<RemoteServiceRequestConsumer>(
      subsystems.ToReference(),
      std::bind(&RemoteServiceRegistry::Find, this, _1, _2),
      web_socket_peer.ToReference());

  web_socket_peer->AddMessageConsumer(m_registration_consumer);
  web_socket_peer->AddMessageConsumer(m_response_consumer);
  web_socket_peer->AddMessageConsumer(m_request_consumer);
}

void RemoteServiceRegistry::SendServicePortfolioToPeer(
    const std::string &peer_id) {

  DEBUG_ASSERT(m_subsystems.CanBorrow(), "subsystems should still exist")

  auto subsystems = m_subsystems.Borrow();
  DEBUG_ASSERT(subsystems->ProvidesSubsystem<IMessageEndpoint>(),
               "peer networking subsystem should exist")
  DEBUG_ASSERT(subsystems->ProvidesSubsystem<jobsystem::JobManager>(),
               "job system should exist")

  if (!subsystems
           ->ProvidesSubsystem<networking::messaging::IMessageEndpoint>()) {
    LOG_ERR("Cannot transmit service portfolio to remote peer "
            << peer_id << ": this peer cannot be found as subsystem")
    return;
  }

  auto this_message_endpoint =
      subsystems->RequireSubsystem<networking::messaging::IMessageEndpoint>();

  auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();

  // create a registration message job for each registered service name
  std::unique_lock callers_lock(m_registered_callers_mutex);
  for (const auto &[service_name, caller] : m_registered_callers) {

    // only offer locally provided services to other nodes.
    if (caller->ContainsLocallyCallable()) {
      auto job = CreateRemoteServiceRegistrationJob(
          peer_id, service_name, this_message_endpoint.ToReference());
      job_manager->KickJob(job);
    }
  }
}

SharedJob RemoteServiceRegistry::CreateRemoteServiceRegistrationJob(
    const std::string &peer_id, const std::string &service_name,
    common::memory::Reference<networking::messaging::IMessageEndpoint>
        weak_endpoint) {

  DEBUG_ASSERT(weak_endpoint.CanBorrow(), "sending endpoint should exist")

  auto job = std::make_shared<Job>(
      [weak_endpoint, peer_id,
       service_name](jobsystem::JobContext *context) mutable {
        if (auto maybe_endpoint = weak_endpoint.TryBorrow()) {
          auto this_peer = maybe_endpoint.value();

          RemoteServiceRegistrationMessage registration;
          registration.SetServiceName(service_name);

          this_peer->Send(peer_id, registration.GetMessage());

          return DISPOSE;
        } else {
          LOG_WARN("Cannot transmit service "
                   << service_name << " to remote peer " << peer_id
                   << " because peer subsystem has expired")
          return DISPOSE;
        }
      },
      "register-service-{" + service_name + "}-at-peer-{" + peer_id + "}",
      MAIN);
  return job;
}
