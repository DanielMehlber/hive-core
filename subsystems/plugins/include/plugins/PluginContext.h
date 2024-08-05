#pragma once

#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "events/broker/IEventBroker.h"
#include "graphics/renderer/IRenderer.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/NetworkingManager.h"
#include "properties/PropertyProvider.h"
#include "resources/manager/IResourceManager.h"
#include "services/registry/IServiceRegistry.h"
#include <memory>
#include <utility>

namespace plugins {

class PluginContext {
protected:
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

public:
  explicit PluginContext(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems)
      : m_subsystems(subsystems){};

  common::memory::Reference<common::subsystems::SubsystemManager>
  GetKernelSubsystems() const;
};

inline common::memory::Reference<common::subsystems::SubsystemManager>
PluginContext::GetKernelSubsystems() const {
  return m_subsystems;
}

typedef std::shared_ptr<PluginContext> SharedPluginContext;

} // namespace plugins
