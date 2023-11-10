#ifndef KERNEL_H
#define KERNEL_H

#include "common/subsystems/SubsystemManager.h"
#include "graphics/renderer/IRenderer.h"
#include "jobsystem/manager/JobManager.h"
#include "messaging/broker/IMessageBroker.h"
#include "networking/NetworkingManager.h"
#include "plugins/IPluginManager.h"
#include "properties/PropertyProvider.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include "scene/SceneManager.h"
#include "services/registry/IServiceRegistry.h"

namespace kernel {

/**
 * Encapsulates all components belonging to the kernel and allows easy setup.
 */
class Kernel {
protected:
  std::shared_ptr<common::subsystems::SubsystemManager> m_subsystems;

public:
  Kernel();

  jobsystem::SharedJobManager GetJobManager() const;

  props::SharedPropertyProvider GetPropertyProvider() const;

  messaging::SharedBroker GetMessageBroker() const;

  resourcemgmt::SharedResourceManager GetResourceManager() const;
  void SetResourceManager(
      const resourcemgmt::SharedResourceManager &resourceManager);

  services::SharedServiceRegistry GetServiceRegistry() const;
  void
  SetServiceRegistry(const services::SharedServiceRegistry &serviceRegistry);

  networking::SharedNetworkingManager GetNetworkingManager() const;
  void SetNetworkingManager(
      const networking::SharedNetworkingManager &networkingManager);

  plugins::SharedPluginManager GetPluginManager() const;
  void SetPluginManager(const plugins::SharedPluginManager &pluginManager);

  scene::SharedScene GetScene() const;
  void SetScene(const scene::SharedScene &scene);

  graphics::SharedRenderer GetRenderer() const;
  void SetRenderer(const graphics::SharedRenderer &renderer);
};

inline jobsystem::SharedJobManager Kernel::GetJobManager() const {
  return m_subsystems->GetSubsystem<jobsystem::JobManager>().value();
}

inline props::SharedPropertyProvider Kernel::GetPropertyProvider() const {
  return m_subsystems->GetSubsystem<props::PropertyProvider>().value();
}

inline resourcemgmt::SharedResourceManager Kernel::GetResourceManager() const {
  return m_subsystems->GetSubsystem<resourcemgmt::IResourceManager>().value();
}

inline messaging::SharedBroker Kernel::GetMessageBroker() const {
  return m_subsystems->GetSubsystem<messaging::IMessageBroker>().value();
}

inline void Kernel::SetResourceManager(
    const resourcemgmt::SharedResourceManager &resourceManager) {
  m_subsystems->AddOrReplaceSubsystem<resourcemgmt::IResourceManager>(
      resourceManager);
}

inline services::SharedServiceRegistry Kernel::GetServiceRegistry() const {
  return m_subsystems->GetSubsystem<services::IServiceRegistry>().value();
}

inline void Kernel::SetServiceRegistry(
    const services::SharedServiceRegistry &serviceRegistry) {
  m_subsystems->GetSubsystem<services::IServiceRegistry>().value();
}

inline networking::SharedNetworkingManager
Kernel::GetNetworkingManager() const {
  return m_subsystems->GetSubsystem<networking::NetworkingManager>().value();
}

inline void Kernel::SetNetworkingManager(
    const networking::SharedNetworkingManager &networkingManager) {
  m_subsystems->AddOrReplaceSubsystem<networking::NetworkingManager>(
      networkingManager);
}

inline plugins::SharedPluginManager Kernel::GetPluginManager() const {
  return m_subsystems->GetSubsystem<plugins::IPluginManager>().value();
}

inline void
Kernel::SetPluginManager(const plugins::SharedPluginManager &pluginManager) {
  m_subsystems->AddOrReplaceSubsystem<plugins::IPluginManager>(pluginManager);
}

inline scene::SharedScene Kernel::GetScene() const {
  return m_subsystems->GetSubsystem<scene::SceneManager>().value();
}

inline void Kernel::SetScene(const scene::SharedScene &scene) {
  m_subsystems->AddOrReplaceSubsystem<scene::SceneManager>(scene);
}

inline graphics::SharedRenderer Kernel::GetRenderer() const {
  return m_subsystems->GetSubsystem<graphics::IRenderer>().value();
}

inline void Kernel::SetRenderer(const graphics::SharedRenderer &renderer) {
  m_subsystems->AddOrReplaceSubsystem<graphics::IRenderer>(renderer);
}

} // namespace kernel

#endif // KERNEL_H
