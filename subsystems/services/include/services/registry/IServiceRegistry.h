#pragma once

#include "services/caller/IServiceCaller.h"
#include <memory>

namespace hive::services {

/**
 * Acts as service look-up: Services can be centrally managed, registered and
 * queried.
 */
class IServiceRegistry {
public:
  /**
   * Register a new service executor.
   * @param stub service executor to register
   * @attention if a service with the same service name is already registered,
   * it will be replaced.
   */
  virtual void Register(const SharedServiceExecutor &stub) = 0;

  /**
   * Unregister all service executors by their service name.
   * @param name of service
   * @attention This will remote all executors, both remote and local.
   */
  virtual void Unregister(const std::string &name) = 0;

  /**
   * Tries to find a service in the registry asynchronously.
   * @param name unique service name
   * @return a service stub, if the service exists, when it has been resolved
   */
  virtual std::future<std::optional<SharedServiceCaller>>
  Find(const std::string &name, bool only_local = false) noexcept = 0;

  virtual ~IServiceRegistry() = default;
};

} // namespace hive::services
