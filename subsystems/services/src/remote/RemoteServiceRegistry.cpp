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
    const SharedServiceExecutor &executor,
    common::memory::Borrower<jobsystem::JobManager> job_manager) {

  DEBUG_ASSERT(weak_sender.CanBorrow(), "sender should exist")
  DEBUG_ASSERT(!executor->GetId().empty(), "executor id should not be empty")
  DEBUG_ASSERT(!executor->GetServiceName().empty(),
               "executor service name should not be empty")
  DEBUG_ASSERT(executor->IsCallable(), "executor should be callable")
  DEBUG_ASSERT(executor->IsLocal(),
               "only local services can be offered to others")

  SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [weak_sender, executor](jobsystem::JobContext *context) mutable {
        if (auto maybe_sender = weak_sender.TryBorrow()) {
          auto sender = maybe_sender.value();
          RemoteServiceRegistrationMessage registration;
          registration.SetServiceName(executor->GetServiceName());
          registration.SetId(executor->GetId());

          std::future<size_t> progress =
              sender->IssueBroadcastAsJob(registration.GetMessage());

          context->GetJobManager()->WaitForCompletion(progress);

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

void RemoteServiceRegistry::Register(const SharedServiceExecutor &executor) {

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
    auto message_endpoint = subsystems->RequireSubsystem<IMessageEndpoint>();

    std::string name = executor->GetServiceName();

    std::unique_lock lock(m_registered_callers_mutex);
    if (!m_registered_callers.contains(name)) {
      m_registered_callers[name] = std::make_shared<RoundRobinServiceCaller>();
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

void RemoteServiceRegistry::Unregister(const std::string &name) {
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

std::future<std::optional<SharedServiceCaller>>
RemoteServiceRegistry::Find(const std::string &service_name, bool only_local) {
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

void RemoteServiceRegistry::SetupMessageConsumers() {
  DEBUG_ASSERT(m_subsystems.CanBorrow(), "subsystems should still exist")
  DEBUG_ASSERT(m_subsystems.Borrow()->ProvidesSubsystem<IMessageEndpoint>(),
               "peer networking subsystem should exist")

  auto subsystems = m_subsystems.Borrow();
  auto web_socket_endpoint = subsystems->RequireSubsystem<IMessageEndpoint>();

  auto response_consumer =
      std::make_shared<RemoteServiceResponseConsumer>(m_subsystems);

  m_response_consumer = response_consumer;
  m_registration_consumer = std::make_shared<RemoteServiceRegistrationConsumer>(
      std::bind(&RemoteServiceRegistry::Register, this, _1), response_consumer,
      web_socket_endpoint.ToReference());
  m_request_consumer = std::make_shared<RemoteServiceRequestConsumer>(
      subsystems.ToReference(),
      std::bind(&RemoteServiceRegistry::Find, this, _1, _2),
      web_socket_endpoint.ToReference());

  web_socket_endpoint->AddMessageConsumer(m_registration_consumer);
  web_socket_endpoint->AddMessageConsumer(m_response_consumer);
  web_socket_endpoint->AddMessageConsumer(m_request_consumer);
}

void RemoteServiceRegistry::SendServicePortfolioToEndpoint(
    const std::string &endpoint_id) {

  DEBUG_ASSERT(m_subsystems.CanBorrow(), "subsystems should still exist")

  auto subsystems = m_subsystems.Borrow();
  DEBUG_ASSERT(subsystems->ProvidesSubsystem<IMessageEndpoint>(),
               "networking subsystem should exist")
  DEBUG_ASSERT(subsystems->ProvidesSubsystem<jobsystem::JobManager>(),
               "job system should exist")

  if (!subsystems
           ->ProvidesSubsystem<networking::messaging::IMessageEndpoint>()) {
    LOG_ERR("Cannot transmit service portfolio to remote endpoint "
            << endpoint_id << ": this peer cannot be found as subsystem")
    return;
  }

  auto this_message_endpoint =
      subsystems->RequireSubsystem<networking::messaging::IMessageEndpoint>();

  auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();

  // create a registration message job for each registered service name
  std::unique_lock callers_lock(m_registered_callers_mutex);
  for (const auto &[service_name, caller] : m_registered_callers) {

    std::vector<SharedServiceExecutor> executors =
        caller->GetCallableServiceExecutors(true);

    for (const auto &executor : executors) {
      // only offer locally provided services to other nodes.
      std::string service_id = executor->GetId();
      auto job = CreateRemoteServiceRegistrationJob(
          endpoint_id, service_name, service_id,
          this_message_endpoint.ToReference());
      job_manager->KickJob(job);
    }
  }
}

SharedJob RemoteServiceRegistry::CreateRemoteServiceRegistrationJob(
    const std::string &endpoint_id, const std::string &service_name,
    const std::string &service_id,
    common::memory::Reference<networking::messaging::IMessageEndpoint>
        weak_endpoint) {

  DEBUG_ASSERT(weak_endpoint.CanBorrow(), "sending endpoint should exist")
  DEBUG_ASSERT(!endpoint_id.empty(), "endpoint id should not be empty")
  DEBUG_ASSERT(!service_name.empty(), "service name should not be empty")
  DEBUG_ASSERT(!service_id.empty(), "service id should not be empty")

  auto job = std::make_shared<Job>(
      [weak_endpoint, endpoint_id, service_name,
       service_id](jobsystem::JobContext *context) mutable {
        if (auto maybe_endpoint = weak_endpoint.TryBorrow()) {
          auto this_endpoint = maybe_endpoint.value();

          RemoteServiceRegistrationMessage registration;
          registration.SetServiceName(service_name);
          registration.SetId(service_id);

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
