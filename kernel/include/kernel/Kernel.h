#ifndef KERNEL_H
#define KERNEL_H

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
#include "common/config/Configuration.h"
#include <memory>

#ifdef _WIN32
// Windows specific
#ifdef kernel_EXPORT
#define KERNEL_API __declspec(dllexport)
#error es hat geklappt
#else
#define KERNEL_API __declspec(dllimport)
#endif
#else
#define KERNEL_API
#endif

using namespace std::chrono_literals;

namespace kernel {

/**
 * Encapsulates all components belonging to the kernel and allows easy setup.
 */
class KERNEL_API Kernel {
protected:
  std::shared_ptr<common::subsystems::SubsystemManager> m_subsystems;
  bool m_should_shutdown{false};

public:
  Kernel(common::config::SharedConfiguration config =
             std::make_shared<common::config::Configuration>(),
         bool only_local = true);

  ~Kernel();

  /**
   * Checks if the kernel is supposed to shut down
   * @return true if shutdown should be performed
   */
  bool ShouldShutdown() const;

  /**
   * Advices the kernel to shutdown on the next occasion.
   * @param value shutdown value
   */
  void Shutdown(bool value = true);

  /**
   * @brief Places a timed rendering job in the job system and causes the
   * current renderer subsystem to render automatically.
   * @note Uses renderer registered in SubsystemsManager
   */
  void EnableRenderingJob();

  /**
   * @brief Registers rendering service in current service registry subsystem.
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
} // namespace kernel

#endif // KERNEL_H
