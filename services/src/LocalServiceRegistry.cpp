#include "services/registry/impl/LocalServiceRegistry.h"
#include "logging/LogManager.h"

using namespace services::impl;

void LocalServiceRegistry::Register(const std::string &name,
                                    const SharedServiceStub &stub) {
  if (m_registered_services.contains(name)) {
    LOG_WARN(
        "service '"
        << name
        << "' is already registered in local registry, but will be replaced");
  }

  m_registered_services[name] = stub;
  LOG_DEBUG("new service '" << name
                            << "' has been registered in local registry");
}

void LocalServiceRegistry::Unregister(const std::string &name) {
  if (m_registered_services.contains(name)) {
    m_registered_services.erase(name);
    LOG_DEBUG("service '" << name
                          << "' has been unregistered from local registry");
  }
}

std::future<std::optional<SharedServiceStub>>
LocalServiceRegistry::Find(const std::string &name) noexcept {
  std::promise<std::optional<SharedServiceStub>> promise;
  std::future<std::optional<SharedServiceStub>> future = promise.get_future();

  if (m_registered_services.contains(name)) {
    SharedServiceStub stub = m_registered_services.at(name);
    if (stub->IsUsable()) {
      promise.set_value(stub);
    } else {
      LOG_WARN(
          "service '"
          << name
          << "' was registered in local registry, but is not usable anymore");
      promise.set_value({});
    }
  } else {
    promise.set_value({});
  }

  return future;
}
