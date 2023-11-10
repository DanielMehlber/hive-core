#ifndef IPLUGIN_H
#define IPLUGIN_H

#include "PluginContext.h"

namespace plugins {

/**
 * Generic interface for plugins that can be loaded and managed by the Plugin
 * Manager.
 * @attention Plugin implementation are not allowed to be placed in some
 * namespace. Otherwise they cannot be loaded by their symbol.
 * @attention Plugin implementations need a default constructor.
 */
class IPlugin {
public:
  /**
   * @return Name of the Plugin Implementation
   */
  virtual std::string GetName() = 0;

  /**
   * Called when the plugin is initialized and should prepare for work.
   * @param context runtime objects that the plugin might need.
   */
  virtual void Init(SharedPluginContext context) = 0;

  /**
   * Called when the plugin is unloaded and should clean up.
   * @param context runtime objects that the plugin might need.
   */
  virtual void ShutDown(SharedPluginContext context) = 0;
};

typedef std::shared_ptr<IPlugin> SharedPlugin;

} // namespace plugins

#endif // IPLUGIN_H