#include "services/registry/impl/local/LocalOnlyServiceRegistry.h"
#include "logging/LogManager.h"
#include "services/caller/impl/RoundRobinServiceCaller.h"

using namespace hive::services;
using namespace hive::services::impl;

void LocalOnlyServiceRegistry::Register(const SharedServiceExecutor &executor) {

  std::string service_name = executor->GetServiceName();

  std::unique_lock services_lock(m_service_callers_mutex);
  if (!m_service_callers.contains(service_name)) {
    m_service_callers[service_name] =
        std::make_shared<RoundRobinServiceCaller>(service_name);
  }

  m_service_callers.at(service_name)->AddExecutor(executor);

  LOG_DEBUG("new service '" << service_name
                            << "' has been registered in local registry")
}

void LocalOnlyServiceRegistry::UnregisterAll(const std::string &service_name) {
  std::unique_lock services_lock(m_service_callers_mutex);
  if (m_service_callers.contains(service_name)) {
    m_service_callers.erase(service_name);
    LOG_DEBUG("service '" << service_name
                          << "' has been unregistered from local registry")
  }
}

void LocalOnlyServiceRegistry::Unregister(
    const SharedServiceExecutor &executor) {
  DEBUG_ASSERT(executor != nullptr, "executor should not be null")
  Unregister(executor->GetId());
}

void LocalOnlyServiceRegistry::Unregister(const std::string &executor_id) {
  DEBUG_ASSERT(!executor_id.empty(), "executor id should not be empty")
  std::unique_lock lock(m_service_callers_mutex);
  for (auto &service : m_service_callers) {
    service.second->RemoveExecutor(executor_id);
  }
}

std::future<std::optional<SharedServiceCaller>>
LocalOnlyServiceRegistry::Find(const std::string &name, bool only_local) {
  std::promise<std::optional<SharedServiceCaller>> promise;
  std::future<std::optional<SharedServiceCaller>> future = promise.get_future();

  std::unique_lock services_lock(m_service_callers_mutex);
  if (m_service_callers.contains(name)) {
    SharedServiceCaller caller = m_service_callers.at(name);
    if (caller->IsCallable()) {
      if (!(only_local) && caller->ContainsLocallyCallable()) {
        promise.set_value(caller);
      } else {
        LOG_WARN("service '" << name
                             << "' is registered in local registry, but not "
                                "locally callable")
        promise.set_value({});
      }
    } else {
      LOG_WARN(
          "service '"
          << name
          << "' was registered in local registry, but is not usable anymore")
      promise.set_value({});
    }
  } else {
    promise.set_value({});
  }

  return future;
}
