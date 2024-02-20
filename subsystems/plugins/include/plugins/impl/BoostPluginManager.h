#ifndef BOOSTPLUGINMANAGER_H
#define BOOSTPLUGINMANAGER_H

#include "boost/dll/shared_library.hpp"
#include "common/subsystems/SubsystemManager.h"
#include "plugins/IPluginManager.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include <map>

#ifndef _WIN32
#define SHARED_LIB_EXTENSION ".so"
#else
#define SHARED_LIB_EXTENSION ".dll"
#endif

namespace plugins {

/**
 * Uses Boost.DLL to load plugins from a dynamic shared library.
 */
class BoostPluginManager
    : public IPluginManager,
      public std::enable_shared_from_this<BoostPluginManager> {
protected:
  /** Context for plugins of this plugin manager */
  SharedPluginContext m_context;

  /** List of all currently installed plugins */
  std::map<std::string, boost::shared_ptr<IPlugin>> m_plugins;
  mutable jobsystem::mutex m_plugins_mutex;

  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

public:
  explicit BoostPluginManager(
      SharedPluginContext context,
      const common::subsystems::SharedSubsystemManager &subsystems)
      : m_context(std::move(context)), m_subsystems(subsystems){};
  void InstallPlugin(const std::string &path) override;
  void InstallPlugin(boost::shared_ptr<IPlugin> plugin) override;
  void UninstallPlugin(const std::string &name) override;
  SharedPluginContext GetContext() override;
  void InstallPlugins(const std::string &input_path_str) override;
};

} // namespace plugins

#endif // BOOSTPLUGINMANAGER_H
