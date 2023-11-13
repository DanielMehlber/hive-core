#ifndef IPLUGINMANAGER_H
#define IPLUGINMANAGER_H

#include "IPlugin.h"
#include <string>

namespace plugins {

class IPluginManager {
public:
  /**
   * Loads plugin from file-system and installs it.
   * @param path to file containing plugin
   */
  virtual void InstallPlugin(const std::string &path) = 0;

  /**
   * Installs a loaded plugin object and initialize it.
   * @note Already installed plugins will be re-installed.
   * @param plugin plugin to install
   */
  virtual void InstallPlugin(std::shared_ptr<IPlugin> plugin) = 0;

  /**
   * Shutdown and uninstall plugin.
   * @param name identifier name of plugin
   */
  virtual void UninstallPlugin(const std::string &name) = 0;

  /**
   * Get the current plugin context
   * @return current plugin context
   */
  virtual SharedPluginContext GetContext() = 0;

  /**
   * Installs all plugins located in a directory
   * @param path to the directory
   */
  virtual void InstallPlugins(const std::string &path) = 0;
};

typedef std::shared_ptr<IPluginManager> SharedPluginManager;

} // namespace plugins

#endif // IPLUGINMANAGER_H
