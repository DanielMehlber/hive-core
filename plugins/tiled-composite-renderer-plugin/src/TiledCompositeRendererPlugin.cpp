#include "TiledCompositeRendererPlugin.h"
#include "TiledCompositeRenderer.h"
#include "events/listener/impl/FunctionalEventListener.h"
#include "plugins/IPluginManager.h"
#include "services/messages/ServiceRegisteredNotification.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS

using namespace std::chrono_literals;

void TiledCompositeRendererPlugin::Init(plugins::SharedPluginContext context) {
  auto subsystems = context->GetKernelSubsystems();
  auto old_renderer = subsystems->RequireSubsystem<graphics::IRenderer>();
  auto tiled_renderer =
      std::make_shared<TiledCompositeRenderer>(subsystems, old_renderer);
  subsystems->AddOrReplaceSubsystem<graphics::IRenderer>(tiled_renderer);
  LOG_DEBUG("Replaced old renderer with new tiled renderer")

  auto service_registry =
      subsystems->RequireSubsystem<services::IServiceRegistry>();

  if (subsystems->ProvidesSubsystem<events::IEventBroker>()) {
    auto event_broker = subsystems->RequireSubsystem<events::IEventBroker>();

    m_render_service_registered_listener =
        std::make_shared<events::FunctionalEventListener>(
            [tiled_renderer, service_registry](events::SharedEvent event) {
              services::ServiceRegisteredNotification notification(event);
              auto service_name = notification.GetServiceName();
              if (service_name == "render") {
                LOG_INFO("new render service detected")

                // TODO: this operation blocks. Make it yield using Jobsystem.
                auto maybe_caller = service_registry->Find("render").get();

                if (maybe_caller.has_value()) {
                  auto caller = maybe_caller.value();
                  auto service_count = caller->GetCallableCount();
                  tiled_renderer->SetServiceCount(service_count);
                }
              }
            });

    event_broker->RegisterListener(m_render_service_registered_listener,
                                   "service-registered-notification");
  }

  // TODO: add listener for service un-registration

  auto job_system = subsystems->RequireSubsystem<jobsystem::JobManager>();

  /* SET CURRENT COUNT OF RENDER SERVICES FOR TILING */
  auto future_render_caller = service_registry->Find("render");
  job_system->WaitForCompletion(future_render_caller);

  auto render_caller = future_render_caller.get();
  size_t render_service_count = 0;
  if (render_caller.has_value()) {
    render_service_count = render_caller.value()->GetCallableCount();
  }

  tiled_renderer->SetServiceCount(render_service_count);
}

void TiledCompositeRendererPlugin::ShutDown(
    plugins::SharedPluginContext context) {}

std::string TiledCompositeRendererPlugin::GetName() {
  return "tiled-composite-renderer";
}

TiledCompositeRendererPlugin::~TiledCompositeRendererPlugin() {}

extern "C" BOOST_SYMBOL_EXPORT TiledCompositeRendererPlugin plugin;
TiledCompositeRendererPlugin plugin;