#ifndef IPLUGIN_H
#define IPLUGIN_H

#include "PluginContext.h"
#include "common/exceptions/ExceptionsBase.h"

namespace plugins {

DECLARE_EXCEPTION(PluginSetupFailedException);

/**
 * A plugin that can be loaded by the plugin manager.
 * @attention Plugin implementation are not allowed to be placed in some
 * namespace. Otherwise they cannot be loaded by their symbol.
 * @attention Plugin implementations need a default constructor.
 */
class IPlugin {
public:
  virtual ~IPlugin() = default;
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
