#ifndef PLUGINCONTEXT_H
#define PLUGINCONTEXT_H

#include "common/subsystems/SubsystemManager.h"
#include "events/broker/IEventBroker.h"
#include "graphics/renderer/IRenderer.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/NetworkingManager.h"
#include "properties/PropertyProvider.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include "services/registry/IServiceRegistry.h"
#include <memory>
#include <utility>

namespace plugins {

class PluginContext {
protected:
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

public:
  explicit PluginContext(
      const common::subsystems::SharedSubsystemManager &subsystems)
      : m_subsystems(subsystems){};

  common::subsystems::SharedSubsystemManager GetKernelSubsystems() const;
};

inline common::subsystems::SharedSubsystemManager
PluginContext::GetKernelSubsystems() const {
  return m_subsystems.lock();
}

typedef std::shared_ptr<PluginContext> SharedPluginContext;

} // namespace plugins

#endif // PLUGINCONTEXT_H
