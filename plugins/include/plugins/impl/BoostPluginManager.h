#ifndef BOOSTPLUGINMANAGER_H
#define BOOSTPLUGINMANAGER_H

#include "plugins/IPluginManager.h"
#include "resourcemgmt/manager/IResourceManager.h"
#include <map>

#ifndef _WIN32
#define SHARED_LIB_EXTENSION ".so"
#else
#define SHARED_LIB_EXTENSION ".dll"
#endif

namespace plugins {

class BoostPluginManager
    : public IPluginManager,
      public std::enable_shared_from_this<BoostPluginManager> {
protected:
  /** Context for plugins of this plugin manager */
  SharedPluginContext m_context;

  /** List of all currently installed plugins */
  std::map<std::string, SharedPlugin> m_plugins;
  mutable std::mutex m_plugins_mutex;

  /** Used to load files */
  resourcemgmt::SharedResourceManager m_resource_manager;

public:
  explicit BoostPluginManager(
      SharedPluginContext context,
      resourcemgmt::SharedResourceManager resource_manager)
      : m_context(std::move(context)),
        m_resource_manager(std::move(resource_manager)){};
  void InstallPlugin(const std::string &path) override;
  void InstallPlugin(SharedPlugin plugin) override;
  void UninstallPlugin(const std::string &name) override;
  SharedPluginContext GetContext() override;
  void InstallPlugins(const std::string &path) override;
};

} // namespace plugins

#endif // BOOSTPLUGINMANAGER_H
