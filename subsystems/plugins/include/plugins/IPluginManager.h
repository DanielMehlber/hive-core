#pragma once

#include "IPlugin.h"
#include <string>

namespace hive::plugins {

/**
 * Loads plugins and manages their life-cycle.
 */
class IPluginManager {
public:
  /**
   * Loads plugin from file-system and installs it.
   * @param path to file containing plugin
   * @note this is not executed directly, but in the next job execution cycle.
   */
  virtual void LoadAndInstallPluginAsJob(const std::string &path) = 0;

  /**
   * Installs a loaded plugin object and initialize it.
   * @param plugin plugin to install
   * @note Already installed plugins will be re-installed.
   * @note this is not executed directly, but in the next job execution cycle.
   */
  virtual void InstallPluginAsJob(boost::shared_ptr<IPlugin> plugin) = 0;

  /**
   * Shutdown and uninstall plugin.
   * @param name identifier name of plugin
   * @note this is not executed directly, but in the next job execution cycle.
   */
  virtual void UnloadPluginAsJob(const std::string &name) = 0;

  /**
   * GetAsInt the current plugin context
   * @return current plugin context
   */
  virtual SharedPluginContext GetContext() = 0;

  /**
   * Installs all plugins located in a directory
   * @param path to the directory
   * @note this is not executed directly, but in the next job execution cycle.
   */
  virtual void LoadPluginsAsJob(const std::string &path) = 0;

  virtual ~IPluginManager() = default;
};

} // namespace hive::plugins
