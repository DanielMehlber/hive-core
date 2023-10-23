#ifndef ISERVICEREGISTRY_H
#define ISERVICEREGISTRY_H

#include "services/caller/IServiceCaller.h"
#include <memory>

namespace services {

/**
 * Acts as service look-up: Services can be centrally managed and registered,
 * for others to find and use.
 */
class IServiceRegistry {
public:
  /**
   * Register a new service to the registry
   * @param name name of this service
   * @param stub service to register
   * @attention if a service with this name is already registered, it will be
   * overwritten
   */
  virtual void Register(const SharedServiceStub &stub) = 0;

  /**
   * Unregister service by its service name
   * @param name of service
   */
  virtual void Unregister(const std::string &name) = 0;

  /**
   * Tries to find a service using this service registry
   * @param name unique service name
   * @return a service stub, if the service exists, when it has been resolved
   */
  virtual std::future<std::optional<SharedServiceCaller>>
  Find(const std::string &name, bool only_local = false) noexcept = 0;
};

typedef std::shared_ptr<IServiceRegistry> SharedServiceRegistry;

} // namespace services

#endif // ISERVICEREGISTRY_H
