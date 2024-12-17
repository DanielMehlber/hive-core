#pragma once

#include <hive/plugins/IPlugin.h>

class DemoPlugin : public hive::plugins::IPlugin {
public:
  DemoPlugin() = default;
  ~DemoPlugin() override = default;

  std::string GetName() override;
  void Init(hive::plugins::SharedPluginContext context) override;
  void ShutDown(hive::plugins::SharedPluginContext context) override;
};