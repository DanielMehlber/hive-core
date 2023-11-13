#include "TiledCompositeRendererPlugin.h"
#include "plugins/IPluginManager.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS

using namespace std::chrono_literals;

void TiledCompositeRendererPlugin::Init(plugins::SharedPluginContext context) {
  LOG_INFO("Setting up tiled composition renderer")

  auto job = std::make_shared<jobsystem::job::TimerJob>(
      [subsystems =
           context->GetKernelSubsystems()](jobsystem::JobContext *job_context) {
        auto plugin_manager =
            subsystems->RequireSubsystem<plugins::IPluginManager>();
        plugin_manager->UninstallPlugin("tiled-composite-renderer");
        return JobContinuation::DISPOSE;
      },
      2s);

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

extern "C" {
// boost::shared_ptr<TiledCompositeRendererPlugin> create() {
//   return boost::shared_ptr<TiledCompositeRendererPlugin>(
//       new TiledCompositeRendererPlugin());
// }

TiledCompositeRendererPlugin *create() {
  return new TiledCompositeRendererPlugin();
}
}