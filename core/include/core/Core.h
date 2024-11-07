#pragma once

#include "common/config/Configuration.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "data/DataLayer.h"
#include "events/broker/IEventBroker.h"
#include "graphics/renderer/IRenderer.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/NetworkingManager.h"
#include "plugins/IPluginManager.h"
#include "resources/manager/IResourceManager.h"
#include "scene/SceneManager.h"
#include "services/registry/IServiceRegistry.h"

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

namespace hive::core {

/**
 * Encapsulates all components belonging to the core system and allows easy
 * setup.
 */
class CORE_API Core {
protected:
  common::memory::Owner<common::subsystems::SubsystemManager> m_subsystems;
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
  void EnableRenderingService(
      common::memory::Reference<graphics::IRenderer> service_renderer = {});

  // vvv getters and setters (for convenience) vvv

  void SetEventBroker(common::memory::Owner<events::IEventBroker> &&broker);
  common::memory::Borrower<events::IEventBroker> GetEventBroker();

  void SetDataLayer(common::memory::Owner<data::DataLayer> &&provider);
  common::memory::Borrower<data::DataLayer> GetDataLayer();

  void SetResourceManager(
      common::memory::Owner<resources::IResourceManager> &&manager);
  common::memory::Borrower<resources::IResourceManager> GetResourceManager();

  void SetServiceRegistry(
      common::memory::Owner<services::IServiceRegistry> &&registry);
  common::memory::Borrower<services::IServiceRegistry> GetServiceRegistry();

  void SetNetworkingManager(
      common::memory::Owner<networking::NetworkingManager> &&manager);
  common::memory::Borrower<networking::NetworkingManager>
  GetNetworkingManager();

  void
  SetPluginManager(common::memory::Owner<plugins::IPluginManager> &&registry);
  common::memory::Borrower<plugins::IPluginManager> GetPluginManager();

  void SetRenderer(common::memory::Owner<graphics::IRenderer> &&renderer);
  common::memory::Borrower<graphics::IRenderer> GetRenderer();

  common::memory::Borrower<jobsystem::JobManager> GetJobManager();

  common::memory::Borrower<common::subsystems::SubsystemManager>
  GetSubsystemsManager();
};
} // namespace hive::core
