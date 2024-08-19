#pragma once

#include "services/registry/IServiceRegistry.h"
#include <future>
#include <map>

namespace hive::services::impl {

/**
 * A registry for services that can be called directly.
 * @attention This registry does not have any registration, request-response
 * implementations for remote service calls. It is only meant for local
 * services.
 */
class LocalOnlyServiceRegistry : public services::IServiceRegistry {
protected:
  mutable jobsystem::mutex m_service_callers_mutex;
  std::map<std::string, services::SharedServiceCaller> m_service_callers;

public:
  void Register(const services::SharedServiceExecutor &executor) override;

  void UnregisterAll(const std::string &service_name) override;
  void Unregister(const SharedServiceExecutor &executor) override;
  void Unregister(const std::string &executor_id) override;

  std::future<std::optional<services::SharedServiceCaller>>
  Find(const std::string &name, bool only_local) override;
};
} // namespace hive::services::impl
