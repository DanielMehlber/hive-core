#pragma once

#include "plugins/IPlugin.h"

class TiledCompositeRendererPlugin : public hive::plugins::IPlugin {
private:
  /**
   * When a new service has joined the registry, an event is triggered. This
   * listener catches it and modifies the plugin's tiling configuration
   * accordingly.
   */
  hive::events::SharedEventListener m_render_service_registered_listener;

  /**
   * When a service has been removed from the registry, an event is triggered.
   * This listener catches it and modifies the plugin's tiling configuration
   * accordingly.
   */
  hive::events::SharedEventListener m_render_service_unregistered_listener;

public:
  std::string GetName() override;
  void Init(hive::plugins::SharedPluginContext context) override;
  void ShutDown(hive::plugins::SharedPluginContext context) override;
  ~TiledCompositeRendererPlugin() override;
};
