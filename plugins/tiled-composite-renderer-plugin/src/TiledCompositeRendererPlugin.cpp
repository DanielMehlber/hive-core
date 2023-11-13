#include "TiledCompositeRendererPlugin.h"
#include "plugins/IPluginManager.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS

using namespace std::chrono_literals;

void TiledCompositeRendererPlugin::Init(plugins::SharedPluginContext context) {
  LOG_INFO("Setting up tiled composition renderer")

  std::weak_ptr<common::subsystems::SubsystemManager> subsystems =
      context->GetKernelSubsystems();

  auto func = [subsystems](jobsystem::JobContext *job_context) {
    auto plugin_manager =
        subsystems.lock()->RequireSubsystem<plugins::IPluginManager>();
    plugin_manager->UninstallPlugin("tiled-composite-renderer");
    return JobContinuation::DISPOSE;
  };

  auto job = std::make_shared<jobsystem::job::TimerJob>(
      std::move(func), "uninstall-self", 2s, CLEAN_UP);

  context->GetKernelSubsystems()
      ->RequireSubsystem<jobsystem::JobManager>()
      ->KickJob(job);
}

void TiledCompositeRendererPlugin::ShutDown(
    plugins::SharedPluginContext context){
    LOG_INFO("Shutting down tiled composition renderer")}

std::string TiledCompositeRendererPlugin::GetName() {
  return "tiled-composite-renderer";
}

TiledCompositeRendererPlugin::~TiledCompositeRendererPlugin() {
  LOG_INFO("Destroyed");
}

extern "C" BOOST_SYMBOL_EXPORT TiledCompositeRendererPlugin plugin;
TiledCompositeRendererPlugin plugin;