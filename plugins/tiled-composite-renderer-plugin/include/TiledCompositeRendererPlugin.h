#ifndef TILEDCOMPOSITERENDERERPLUGIN_H
#define TILEDCOMPOSITERENDERERPLUGIN_H

#include "graphics/service/RenderService.h"
#include "plugins/IPlugin.h"

class TiledCompositeRendererPlugin : public plugins::IPlugin {
private:
public:
  std::string GetName() override;
  void Init(plugins::SharedPluginContext context) override;
  void ShutDown(plugins::SharedPluginContext context) override;
  ~TiledCompositeRendererPlugin();
};

#endif // TILEDCOMPOSITERENDERERPLUGIN_H
