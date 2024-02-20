#ifndef KERNEL_H
#define KERNEL_H

#include "common/config/Configuration.h"
#include "common/subsystems/SubsystemManager.h"
#include "events/broker/IEventBroker.h"
#include "graphics/renderer/IRenderer.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/NetworkingManager.h"
#include "plugins/IPluginManager.h"
#include "properties/PropertyProvider.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include "scene/SceneManager.h"
#include "services/registry/IServiceRegistry.h"
#include <memory>

#ifdef _WIN32
// Windows specific
#ifdef core_EXPORT
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport)
#endif
#else
#define CORE_API
#endif

using namespace std::chrono_literals;

namespace core {

/**
 * Encapsulates all components belonging to the core system and allows easy
 * setup.
 */
class CORE_API Core {
protected:
  std::shared_ptr<common::subsystems::SubsystemManager> m_subsystems;
  bool m_should_shutdown{false};

public:
  Core(common::config::SharedConfiguration config =
           std::make_shared<common::config::Configuration>(),
       bool only_local = true);

  ~Core();

  /**
   * Checks if the core system is supposed to shut down
   * @return true if shutdown should be performed
   */
  bool ShouldShutdown() const;

  /**
   * Advices the core system to shutdown on the next occasion.
   * @param value shutdown value
   */
  void Shutdown(bool value = true);

  /**
   * Places a timed rendering job in the job system and causes the
   * current renderer subsystem to render automatically.
   * @note Uses renderer registered in SubsystemsManager
   */
  void EnableRenderingJob();

  /**
   * Registers rendering service in current service registry subsystem.
   * @param service_renderer renderer to use in service. If nullptr, the current
   * rendering subsystem will be used.
   */
  void
  EnableRenderingService(graphics::SharedRenderer service_renderer = nullptr);

  // vvv getters and setters vvv

  jobsystem::SharedJobManager GetJobManager() const;

  props::SharedPropertyProvider GetPropertyProvider() const;

  events::SharedEventBroker GetMessageBroker() const;

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

  common::subsystems::SharedSubsystemManager GetSubsystemsManager() const;
};
} // namespace core

#endif // KERNEL_H
