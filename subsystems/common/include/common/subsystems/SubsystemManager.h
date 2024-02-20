#ifndef SUBSYSTEMMANAGER_H
#define SUBSYSTEMMANAGER_H

#include "boost/core/demangle.hpp"
#include "common/exceptions/ExceptionsBase.h"
#include <any>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <typeindex>

namespace common::subsystems {

/**
 * Thrown when a subsystem cannot be found and wasn't requested optionally.
 */
DECLARE_EXCEPTION(SubsystemNotFoundException);

/**
 * Maintains implementations of subsystems and makes them accessible to
 * each other. This is important because the presence of a subsystem is not
 * guaranteed and must be queried before use.
 */
class SubsystemManager : public std::enable_shared_from_this<SubsystemManager> {
private:
  std::unordered_map<std::type_index, std::any> m_subsystems;

public:
  SubsystemManager() = default;
  SubsystemManager(const SubsystemManager &other);

  /**
   * Registers subsystem or replaces its implementation if already registered.
   * @tparam subsystem_t type of subsystem. This exact type must be used to
   * retrieve it later. Usually this is an interface type.
   * @param subsystem subsystem that should be registered for that type. Usually
   * this is an interface implementation.
   */
  template <typename subsystem_t>
  void AddOrReplaceSubsystem(const std::shared_ptr<subsystem_t> &subsystem);

  /**
   * Tries to find a registered subsystem of the requested type and returns it.
   * @tparam subsystem_t type of subsystem. Usually this is an interface type.
   * @return subsystem implementation if it has been registered before.
   */
  template <typename subsystem_t>
  std::optional<std::shared_ptr<subsystem_t>> GetSubsystem() const;

  /**
   * Tries to find a registered subsystem of the requested type, but throws an
   * exception if it cannot be found.
   * @tparam subsystem_t type of subsystem. Usually this is an interface type.
   * @return subsystem implementation.
   * @throws SubsystemNotFoundException if subsystem has not been registered
   * before.
   */
  template <typename subsystem_t>
  std::shared_ptr<subsystem_t> RequireSubsystem() const;

  /**
   * Checks if a subsystem of the requested type is registered.
   * @tparam subsystem_t type of subsystem. Usually this is an interface type.
   * @return true, if subsystem can be provided by this manager.
   */
  template <typename subsystem_t> bool ProvidesSubsystem() const;
};

inline SubsystemManager::SubsystemManager(const SubsystemManager &other)
    : m_subsystems(other.m_subsystems) {}

template <typename subsystem_t>
bool SubsystemManager::ProvidesSubsystem() const {
  return m_subsystems.contains(typeid(subsystem_t));
}

template <typename subsystem_t>
std::shared_ptr<subsystem_t> SubsystemManager::RequireSubsystem() const {
  auto opt_subsystem = GetSubsystem<subsystem_t>();
  if (opt_subsystem.has_value()) {
    return opt_subsystem.value();
  } else {
    const char *name = typeid(subsystem_t).name();

    auto demangled = boost::core::demangle(name);
    THROW_EXCEPTION(SubsystemNotFoundException,
                    "subsystem " << demangled << " is not provided")
  }
}

template <typename subsystem_t>
std::optional<std::shared_ptr<subsystem_t>>
SubsystemManager::GetSubsystem() const {
  if (ProvidesSubsystem<subsystem_t>()) {
    return std::any_cast<std::shared_ptr<subsystem_t>>(
        m_subsystems.at(typeid(subsystem_t)));
  }

  return {};
}

template <typename subsystem_t>
void SubsystemManager::AddOrReplaceSubsystem(
    const std::shared_ptr<subsystem_t> &subsystem) {
  m_subsystems[typeid(subsystem_t)] = subsystem;
}

typedef std::shared_ptr<SubsystemManager> SharedSubsystemManager;

} // namespace common::subsystems

#endif // SUBSYSTEMMANAGER_H
