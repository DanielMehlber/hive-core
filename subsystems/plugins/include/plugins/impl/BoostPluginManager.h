#pragma once

#include "common/subsystems/SubsystemManager.h"
#include "plugins/IPluginManager.h"
#include "resources/manager/IResourceManager.h"

#ifndef _WIN32
#define SHARED_LIB_EXTENSION ".so"
#else
#define SHARED_LIB_EXTENSION ".dll"
#endif

namespace hive::plugins {

/**
 * Uses Boost.DLL to load plugins from a dynamic shared library.
 */
class BoostPluginManager
    : public IPluginManager,
      public common::memory::EnableBorrowFromThis<BoostPluginManager> {
protected:
  /**
   * Pointer to implementation (Pimpl) in source file. This is necessary to
   * constrain Boost's implementation details in the source file and not expose
   * them to the rest of the application.
   * @note Indispensable for ABI stability and to use static-linked Boost.
   */
  struct Impl;
  std::unique_ptr<Impl> m_impl;

  /** Context for plugins of this plugin manager */
  SharedPluginContext m_context;

  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

  /**
   * Unloads and shuts down a plugin with a given name.
   * @param name of the plugin that should be unloaded and shut down
   */
  void UnloadPlugin(const std::string &name);

public:
  explicit BoostPluginManager(
      SharedPluginContext context,
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems);

  ~BoostPluginManager() override;

  BoostPluginManager(const BoostPluginManager &other) = delete;
  BoostPluginManager &operator=(const BoostPluginManager &other) = delete;

  BoostPluginManager(BoostPluginManager &&other) noexcept;
  BoostPluginManager &operator=(BoostPluginManager &&other) noexcept;

  void LoadAndInstallPluginAsJob(const std::string &path) override;
  void InstallPluginAsJob(boost::shared_ptr<IPlugin> plugin) override;
  void LoadPluginsAsJob(const std::string &input_path_str) override;
  void UnloadPluginAsJob(const std::string &name) override;
  SharedPluginContext GetContext() override;
};

} // namespace hive::plugins
