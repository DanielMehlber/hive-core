#pragma once

#include "common/config/Configuration.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include <memory>

namespace hive::plugins {

/**
 * Enables access to the core's subsystems and other facilities that are
 * required when initializing and shutting down plugins.
 */
class PluginContext {
protected:
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;
  std::shared_ptr<common::config::Configuration> m_config;

public:
  explicit PluginContext(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      const std::shared_ptr<common::config::Configuration> &config)
      : m_subsystems(subsystems), m_config{config} {}

  common::memory::Reference<common::subsystems::SubsystemManager>
  GetSubsystems() const;

  std::shared_ptr<common::config::Configuration> GetConfig() const;
};

inline common::memory::Reference<common::subsystems::SubsystemManager>
PluginContext::GetSubsystems() const {
  return m_subsystems;
}

inline std::shared_ptr<common::config::Configuration>
PluginContext::GetConfig() const {
  return m_config;
}

typedef std::shared_ptr<PluginContext> SharedPluginContext;

} // namespace hive::plugins
