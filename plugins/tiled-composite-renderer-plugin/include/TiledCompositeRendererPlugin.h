#ifndef TILEDCOMPOSITERENDERERPLUGIN_H
#define TILEDCOMPOSITERENDERERPLUGIN_H

#include "graphics/service/RenderService.h"
#include "plugins/IPlugin.h"

class TiledCompositeRendererPlugin : public plugins::IPlugin {
private:
  events::SharedEventListener m_render_service_registered_listener;
  events::SharedEventListener m_render_service_unregistered_listener;

public:
  std::string GetName() override;
  void Init(plugins::SharedPluginContext context) override;
  void ShutDown(plugins::SharedPluginContext context) override;
  ~TiledCompositeRendererPlugin();
};

#endif // TILEDCOMPOSITERENDERERPLUGIN_H
