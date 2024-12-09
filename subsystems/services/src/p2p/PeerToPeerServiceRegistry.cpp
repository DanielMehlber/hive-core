#include "services/registry/impl/p2p/PeerToPeerServiceRegistry.h"
#include "common/assert/Assert.h"
#include "events/broker/IEventBroker.h"
#include "logging/LogManager.h"
#include "networking/NetworkingManager.h"
#include "networking/messaging/events/ConnectionEstablishedEvent.h"
#include "services/caller/impl/RoundRobinServiceCaller.h"
#include "services/messages/ServiceRegisteredNotification.h"
#include "services/messages/ServiceUnegisteredNotification.h"

using namespace hive;
using namespace hive::services;
using namespace hive::services::impl;
using namespace std::placeholders;
using namespace hive::networking;
using namespace hive::networking::messaging;
using namespace hive::jobsystem;

void broadcastServiceRegistration(
    common::memory::Reference<IMessageEndpoint> weak_sender,
    const SharedServiceExecutor &executor,
    common::memory::Borrower<jobsystem::JobManager> job_manager) {

  DEBUG_ASSERT(weak_sender.CanBorrow(), "sender should exist")
  DEBUG_ASSERT(!executor->GetId().empty(), "executor id should not be empty")
  DEBUG_ASSERT(!executor->GetServiceName().empty(),
               "executor service name should not be empty")
  DEBUG_ASSERT(executor->IsCallable(), "executor should be callable")
  DEBUG_ASSERT(executor->IsLocal(),
               "only local services can be offered to others")

  SharedJob job = std::make_shared<Job>(
      [weak_sender, executor](jobsystem::JobContext *context) mutable {
        if (auto maybe_sender = weak_sender.TryBorrow()) {
          auto sender = maybe_sender.value();

          RemoteServiceRegistrationMessage registration;
          registration.SetServiceName(executor->GetServiceName());
          registration.SetId(executor->GetId());
          registration.SetCapacity(executor->GetCapacity());

          std::future<size_t> progress =
              sender->IssueBroadcastAsJob(registration.GetMessage());

          context->GetJobManager()->Await(progress);

          try {
            size_t receivers = progress.get();
            LOG_DEBUG("broadcast local service " << executor->GetServiceName()
                                                 << " to " << receivers
                                                 << " other endpoints")
          } catch (std::exception &exception) {
            LOG_ERR("broadcasting local service "
                    << executor->GetServiceName()
                    << " failed due to internal error: " << exception.what())
          }
        } else {
          LOG_WARN("cannot broadcast local service "
                   << executor->GetServiceName()
                   << " because web-socket endpoint has been destroyed")
        }

        return JobContinuation::DISPOSE;
      },
      "broadcast-service-registration-" + executor->GetServiceName());

  job_manager->KickJob(job);
}

void PeerToPeerServiceRegistry::Register(
    const SharedServiceExecutor &executor) {

  DEBUG_ASSERT(!executor->GetId().empty(), "executor id should not be empty")
  DEBUG_ASSERT(!executor->GetServiceName().empty(),
               "executor service name should not be empty")

  if (!executor->IsCallable()) {
    LOG_WARN("cannot register " << (executor->IsLocal() ? "local" : "remote")
                                << " service '" << executor->GetServiceName()
                                << "' because it is not callable (anymore)")
    return;
  }

  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();
    auto networking_manager =
        subsystems->RequireSubsystem<networking::NetworkingManager>();
    auto message_endpoint =
        networking_manager->GetDefaultMessageEndpoint().value();

    std::string name = executor->GetServiceName();

    std::unique_lock lock(m_registered_callers_mutex);
    if (!m_registered_callers.contains(name)) {
      m_registered_callers[name] =
          std::make_shared<RoundRobinServiceCaller>(name);
    }

    if (executor->IsLocal()) {
      broadcastServiceRegistration(message_endpoint.ToReference(), executor,
                                   job_manager);
    }

    m_registered_callers.at(name)->AddExecutor(executor);
    LOG_DEBUG("new " << (executor->IsLocal() ? "local" : "remote")
                     << " service '" << name
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

void PeerToPeerServiceRegistry::UnregisterAll(const std::string &name) {
  DEBUG_ASSERT(!name.empty(), "name should not be empty")

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

void PeerToPeerServiceRegistry::Unregister(const std::string &executor_id) {
  DEBUG_ASSERT(!executor_id.empty(), "executor id should not be empty")

  std::unique_lock lock(m_registered_callers_mutex);
  for (auto &[service_name, caller] : m_registered_callers) {
    caller->RemoveExecutor(executor_id);
  }
}

std::future<std::optional<SharedServiceCaller>>
PeerToPeerServiceRegistry::Find(const std::string &service_name,
                                bool only_local) {
  DEBUG_ASSERT(!service_name.empty(), "name should not be empty")

  std::promise<std::optional<SharedServiceCaller>> promise;
  std::future<std::optional<SharedServiceCaller>> future = promise.get_future();

  std::unique_lock lock(m_registered_callers_mutex);
  if (m_registered_callers.contains(service_name)) {
    SharedServiceCaller caller = m_registered_callers.at(service_name);

    // check if this is callable at all
    if (caller->IsCallable()) {

      // if a local stub is required, check if the caller can provide one
      if (only_local) {
        if (!caller->ContainsLocallyCallable()) {
          LOG_WARN("service '"
                   << service_name
                   << "' is registered in web-socket registry, but not "
                      "locally callable")
          promise.set_value({});
        }
      }
      promise.set_value(caller);
    } else {
      LOG_WARN("service '" << service_name
                           << "' was registered in web-socket registry, but is "
                              "not usable anymore")
      promise.set_value({});
    }
  } else {
    promise.set_value({});
  }

  return future;
}

PeerToPeerServiceRegistry::PeerToPeerServiceRegistry(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems)
    : m_subsystems(subsystems) {

  SetupMessageConsumers();
  SetupEventSubscribers();
}

void PeerToPeerServiceRegistry::SetupEventSubscribers() {
  DEBUG_ASSERT(m_subsystems.CanBorrow(), "subsystems should still exist")
  auto subsystems = m_subsystems.Borrow();
  DEBUG_ASSERT(subsystems->ProvidesSubsystem<events::IEventBroker>(),
               "event broker subsystem should exist")

  auto event_system = subsystems->RequireSubsystem<events::IEventBroker>();

  m_new_endpoint_connection_listener =
      std::make_shared<events::FunctionalEventListener>(
          [this](const events::SharedEvent event) {
            networking::ConnectionEstablishedEvent established_event(event);
            auto endpoint_id = established_event.GetEndpointId();
            SendServicePortfolioToEndpoint(endpoint_id);
          });

  event_system->RegisterListener(m_new_endpoint_connection_listener,
                                 "connection-established");
}

void PeerToPeerServiceRegistry::SetupMessageConsumers() {
  DEBUG_ASSERT(m_subsystems.CanBorrow(), "subsystems should still exist")
  DEBUG_ASSERT(m_subsystems.Borrow()->ProvidesSubsystem<NetworkingManager>(),
               "peer networking subsystem should exist")

  auto subsystems = m_subsystems.Borrow();
  auto networking_manager =
      subsystems->RequireSubsystem<networking::NetworkingManager>();

  if (!networking_manager->HasInstalledMessageEndpoints()) {
    LOG_WARN("no message endpoints installed in networking manager; calling "
             "remote services is possible until at least one endpoint "
             "is installed")
  }

  auto response_consumer =
      std::make_shared<RemoteServiceResponseConsumer>(m_subsystems);

  m_response_consumer = response_consumer;
  m_registration_consumer = std::make_shared<RemoteServiceRegistrationConsumer>(
      std::bind(&PeerToPeerServiceRegistry::Register, this, _1),
      response_consumer, networking_manager.ToReference());
  m_request_consumer = std::make_shared<RemoteServiceRequestConsumer>(
      subsystems.ToReference(),
      std::bind(&PeerToPeerServiceRegistry::Find, this, _1, _2),
      networking_manager.ToReference());

  networking_manager->AddMessageConsumer(m_registration_consumer);
  networking_manager->AddMessageConsumer(m_response_consumer);
  networking_manager->AddMessageConsumer(m_request_consumer);
}

void PeerToPeerServiceRegistry::Unregister(
    const SharedServiceExecutor &executor) {
  DEBUG_ASSERT(executor != nullptr, "executor should not be null")
  Unregister(executor->GetId());
}

void PeerToPeerServiceRegistry::SendServicePortfolioToEndpoint(
    const std::string &endpoint_id) {

  DEBUG_ASSERT(m_subsystems.CanBorrow(), "subsystems should still exist")

  auto subsystems = m_subsystems.Borrow();
  DEBUG_ASSERT(subsystems->ProvidesSubsystem<NetworkingManager>(),
               "networking subsystem should exist")
  DEBUG_ASSERT(subsystems->ProvidesSubsystem<jobsystem::JobManager>(),
               "job system should exist")

  auto networking_manager = subsystems->RequireSubsystem<NetworkingManager>();
  auto endpoint = networking_manager->RequireDefaultMessageEndpoint();

  auto job_manager = subsystems->RequireSubsystem<JobManager>();

  // create a registration message job for each registered service name
  std::unique_lock callers_lock(m_registered_callers_mutex);
  for (const auto &[service_name, caller] : m_registered_callers) {

    std::vector<SharedServiceExecutor> executors =
        caller->GetCallableServiceExecutors(true);

    for (const auto &executor : executors) {
      // only offer locally provided services to other nodes.
      std::string service_id = executor->GetId();
      auto job = CreateRemoteServiceRegistrationJob(
          endpoint_id, service_name, service_id, executor->GetCapacity(),
          endpoint.ToReference());
      job_manager->KickJob(job);
    }
  }
}

SharedJob PeerToPeerServiceRegistry::CreateRemoteServiceRegistrationJob(
    const std::string &endpoint_id, const std::string &service_name,
    const std::string &service_id, size_t capacity,
    common::memory::Reference<IMessageEndpoint> weak_endpoint) {

  DEBUG_ASSERT(weak_endpoint.CanBorrow(), "sending endpoint should exist")
  DEBUG_ASSERT(!endpoint_id.empty(), "endpoint id should not be empty")
  DEBUG_ASSERT(!service_name.empty(), "service name should not be empty")
  DEBUG_ASSERT(!service_id.empty(), "service id should not be empty")

  auto job = std::make_shared<Job>(
      [weak_endpoint, endpoint_id, service_name, service_id,
       capacity](jobsystem::JobContext *context) mutable {
        if (auto maybe_endpoint = weak_endpoint.TryBorrow()) {
          auto this_endpoint = maybe_endpoint.value();

          RemoteServiceRegistrationMessage registration;
          registration.SetServiceName(service_name);
          registration.SetId(service_id);
          registration.SetCapacity(capacity);

          this_endpoint->Send(endpoint_id, registration.GetMessage());

          return DISPOSE;
        } else {
          LOG_WARN("Cannot transmit service "
                   << service_name << " to remote endpoint " << endpoint_id
                   << " because endpoint subsystem has expired")
          return DISPOSE;
        }
      },
      "register-service-{" + service_name + "}-at-endpoint-{" + endpoint_id +
          "}",
      MAIN);
  return job;
}
