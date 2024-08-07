#pragma once

#include "plugins/IPlugin.h"

class TestPlugin : public hive::plugins::IPlugin {
private:
  std::atomic_size_t m_init_count;
  std::atomic_size_t m_shutdown_count;

public:
  std::string GetName() override { return "test"; };

  void Init(hive::plugins::SharedPluginContext context) override {
    m_init_count++;
  };
  void ShutDown(hive::plugins::SharedPluginContext context) override {
    m_shutdown_count++;
  }

  size_t GetInitCount() { return m_init_count; }
  size_t GetShutdownCount() { return m_shutdown_count; }
};
