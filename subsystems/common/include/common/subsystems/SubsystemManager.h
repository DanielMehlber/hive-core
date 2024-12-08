#pragma once

#include "common/exceptions/ExceptionsBase.h"
#include "common/memory/ExclusiveOwnership.h"
#include <any>
#include <boost/core/demangle.hpp>
#include <map>
#include <memory>
#include <optional>
#include <typeindex>

namespace hive::common::subsystems {

/**
 * Thrown when a subsystem cannot be found and wasn't requested optionally.
 */
DECLARE_EXCEPTION(SubsystemNotFoundException);

/**
 * Maintains implementations of subsystems and makes them accessible to
 * each other. This is important because the presence of a subsystem is not
 * guaranteed and must be queried before use.
 */
class SubsystemManager {
  std::map<std::string, std::any> m_subsystems;
  sync::SpinLock m_lock;

public:
  SubsystemManager() = default;
  SubsystemManager(SubsystemManager &other) = delete;
  SubsystemManager(SubsystemManager &&other) noexcept;

  /**
   * Registers subsystem or replaces its implementation if already registered.
   * @tparam subsystem_t type of subsystem. This exact type must be used to
   * retrieve it later. Usually this is an interface type.
   * @param subsystem subsystem that should be registered for that type. Usually
   * this is an interface implementation.
   */
  template <typename subsystem_t>
  void AddOrReplaceSubsystem(memory::Owner<subsystem_t> &&subsystem);

  /**
   * Tries to find a registered subsystem of the requested type and returns it.
   * @tparam subsystem_t type of subsystem. Usually this is an interface type.
   * @return subsystem implementation if it has been registered before.
   */
  template <typename subsystem_t>
  std::optional<memory::Borrower<subsystem_t>> GetSubsystem() const;

  /**
   * Tries to find a registered subsystem of the requested type, but throws an
   * exception if it cannot be found.
   * @tparam subsystem_t type of subsystem. Usually this is an interface type.
   * @return subsystem implementation.
   * @throws SubsystemNotFoundException if subsystem has not been registered
   * before.
   */
  template <typename subsystem_t>
  memory::Borrower<subsystem_t> RequireSubsystem() const;

  /**
   * Checks if a subsystem of the requested type is registered.
   * @tparam subsystem_t type of subsystem. Usually this is an interface type.
   * @return true, if subsystem can be provided by this manager.
   */
  template <typename subsystem_t> bool ProvidesSubsystem() const;

  /**
   * Removes a subsystem from managed subsystems and returns its ownership to
   * the caller. This operation fails, if the subsystem does not exist.
   * @tparam subsystem_t type of the subsystem
   * @return ownership over the removed subsystem.
   * @throws SubsystemNotFoundException if no such subsystem is registered.
   */
  template <typename subsystem_t> memory::Owner<subsystem_t> RemoveSubsystem();
};

inline SubsystemManager::SubsystemManager(SubsystemManager &&other) noexcept
    : m_subsystems(std::move(other.m_subsystems)) {}

template <typename type_t> std::string GetTypeName() {
  const char *name = typeid(type_t).name();
  return boost::core::demangle(name);
}

template <typename subsystem_t>
memory::Owner<subsystem_t> SubsystemManager::RemoveSubsystem() {
  std::unique_lock lock(m_lock);

  std::string subsystem_t_name = GetTypeName<subsystem_t>();
  if (!m_subsystems.contains(subsystem_t_name)) {
    THROW_EXCEPTION(SubsystemNotFoundException,
                    "cannot remove subsystem of type "
                        << subsystem_t_name << " because none was registered");
  }

  const auto &any_subsystem = m_subsystems.at(subsystem_t_name);
  auto subsystem_owner_ptr =
      std::any_cast<std::shared_ptr<memory::Owner<subsystem_t>>>(any_subsystem);

  auto subsystem_owner = std::move(*subsystem_owner_ptr);
  m_subsystems.erase(subsystem_t_name);

  return subsystem_owner;
}

template <typename subsystem_t>
bool SubsystemManager::ProvidesSubsystem() const {
  return m_subsystems.contains(GetTypeName<subsystem_t>());
}

template <typename subsystem_t>
memory::Borrower<subsystem_t> SubsystemManager::RequireSubsystem() const {
  auto opt_subsystem = GetSubsystem<subsystem_t>();
  if (opt_subsystem.has_value()) {
    return opt_subsystem.value();
  }

  THROW_EXCEPTION(SubsystemNotFoundException,
                  "subsystem "
                      << GetTypeName<subsystem_t> << " is not provided")
}

template <typename subsystem_t>
std::optional<memory::Borrower<subsystem_t>>
SubsystemManager::GetSubsystem() const {
  if (ProvidesSubsystem<subsystem_t>()) {
    const auto &any_subsystem = m_subsystems.at(GetTypeName<subsystem_t>());
    auto subsystem_owner_ptr =
        std::any_cast<std::shared_ptr<memory::Owner<subsystem_t>>>(
            any_subsystem);
    return subsystem_owner_ptr->Borrow();
  }

  return {};
}

template <typename subsystem_t>
void SubsystemManager::AddOrReplaceSubsystem(
    memory::Owner<subsystem_t> &&subsystem) {
  std::unique_lock lock(m_lock);

  /* std::any can only handle copy-constructible types, but
   * common::memory::Owner is not. It is therefore wrapped in such a type
   * for this purpose: the std::shared_ptr. I know this is ugly on the inside,
   * but we don't expose the std::shared_ptr to the outside, and it's the only
   * way to get templates, std::any, and non-copy-constructible types to play
   * along... */
  auto subsystem_owner_ptr =
      std::make_shared<memory::Owner<subsystem_t>>(std::move(subsystem));

  std::any subsystem_any = subsystem_owner_ptr;
  m_subsystems[GetTypeName<subsystem_t>()] = subsystem_any;
}

} // namespace hive::common::subsystems
