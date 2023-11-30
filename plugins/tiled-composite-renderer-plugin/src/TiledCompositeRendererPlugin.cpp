#include "TiledCompositeRendererPlugin.h"
#include "TiledCompositeRenderer.h"
#include "plugins/IPluginManager.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS

using namespace std::chrono_literals;

void TiledCompositeRendererPlugin::Init(plugins::SharedPluginContext context) {
  auto subsystems = context->GetKernelSubsystems();
  auto old_renderer = subsystems->RequireSubsystem<graphics::IRenderer>();
  auto tiled_renderer =
      std::make_shared<TiledCompositeRenderer>(subsystems, old_renderer);
  subsystems->AddOrReplaceSubsystem<graphics::IRenderer>(tiled_renderer);

  LOG_DEBUG("Replaced old renderer with new tiled renderer")

  auto tiling_job = std::make_shared<TimerJob>(
      [tiled_renderer](jobsystem::JobContext *context) {
        static int i{1};
        i++;
        if (i > 5) {
          return JobContinuation::DISPOSE;
        } else {
          tiled_renderer->SetServiceCount(i);
          return JobContinuation::REQUEUE;
        }
      },
      5s);

  auto job_system = subsystems->RequireSubsystem<jobsystem::JobManager>();
  job_system->KickJob(tiling_job);

  tiled_renderer->SetServiceCount(1);
}

void TiledCompositeRendererPlugin::ShutDown(
    plugins::SharedPluginContext context) {}

std::string TiledCompositeRendererPlugin::GetName() {
  return "tiled-composite-renderer";
}

TiledCompositeRendererPlugin::~TiledCompositeRendererPlugin() {}

extern "C" BOOST_SYMBOL_EXPORT TiledCompositeRendererPlugin plugin;
TiledCompositeRendererPlugin plugin;