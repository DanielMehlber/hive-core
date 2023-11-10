#ifndef SUBSYSTEMMANAGER_H
#define SUBSYSTEMMANAGER_H

#include "common/exceptions/ExceptionsBase.h"
#include <any>
#include <map>
#include <memory>
#include <optional>
#include <typeindex>

namespace common::subsystems {

DECLARE_EXCEPTION(SubsystemNotFoundException);

/**
 * Manages various subsystems
 */
class SubsystemManager : public std::enable_shared_from_this<SubsystemManager> {
private:
  std::unordered_map<std::type_index, std::any> m_subsystems;

public:
  /**
   * Registers subsystem or replaces its implementation if already registered.
   * @tparam subsystem_t type of subsystem
   * @param subsystem subsystem that should be registered for that type
   */
  template <typename subsystem_t>
  void AddOrReplaceSubsystem(const std::shared_ptr<subsystem_t> &subsystem);

  /**
   * Get subsystem if it is registered
   * @tparam subsystem_t type of subsystem
   * @return optional subsystem
   */
  template <typename subsystem_t>
  std::optional<std::shared_ptr<subsystem_t>> GetSubsystem() const;

  /**
   * Get subsystem, but throw exception if it is not provided
   * @tparam subsystem_t type of subsystem
   * @return subsystem
   */
  template <typename subsystem_t>
  std::shared_ptr<subsystem_t> RequireSubsystem() const;
};

template <typename subsystem_t>
std::shared_ptr<subsystem_t> SubsystemManager::RequireSubsystem() const {
  auto opt_subsystem = GetSubsystem<subsystem_t>();
  if (opt_subsystem.has_value()) {
    return opt_subsystem.value();
  } else {
    const char *name = typeid(subsystem_t).name();
    THROW_EXCEPTION(SubsystemNotFoundException,
                    "subsystem " << typeid(subsystem_t).name()
                                 << " is not provided")
  }
}

template <typename subsystem_t>
std::optional<std::shared_ptr<subsystem_t>>
SubsystemManager::GetSubsystem() const {
  if (m_subsystems.contains(typeid(subsystem_t))) {
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
