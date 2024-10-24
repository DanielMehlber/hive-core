#include "TiledCompositeRendererPlugin.h"
#include "TiledCompositeRenderer.h"
#include "common/memory/ExclusiveOwnership.h"
#include "events/listener/impl/FunctionalEventListener.h"
#include "plugins/IPluginManager.h"
#include "services/messages/ServiceRegisteredNotification.h"
#include <boost/dll/alias.hpp> // for BOOST_DLL_ALIAS

using namespace hive;
using namespace std::chrono_literals;

void TiledCompositeRendererPlugin::Init(plugins::SharedPluginContext context) {
  if (auto maybe_subsystems = context->GetSubsystems().TryBorrow()) {
    auto subsystems = maybe_subsystems.value();

    /* Take the old renderer and replace it with the new tiled-based renderer.
     * Use the old renderer as its output renderer. */
    auto old_renderer = subsystems->RemoveSubsystem<graphics::IRenderer>();
    auto tiled_renderer = common::memory::Owner<TiledCompositeRenderer>(
        subsystems.ToReference(), std::move(old_renderer));
    auto tiled_renderer_ref = tiled_renderer.CreateReference();
    subsystems->AddOrReplaceSubsystem<graphics::IRenderer>(
        std::move(tiled_renderer));
    LOG_DEBUG("Replaced old renderer with new tiled renderer")

    auto service_registry =
        subsystems->RequireSubsystem<services::IServiceRegistry>();

    if (subsystems->ProvidesSubsystem<events::IEventBroker>()) {
      auto event_broker = subsystems->RequireSubsystem<events::IEventBroker>();

      m_render_service_registered_listener =
          std::make_shared<events::FunctionalEventListener>(
              [tiled_renderer_ref,
               service_registry](events::SharedEvent event) mutable {
                if (auto maybe_tiled_renderer =
                        tiled_renderer_ref.TryBorrow()) {
                  auto tiled_renderer = maybe_tiled_renderer.value();
                  services::ServiceRegisteredNotification notification(event);
                  auto service_name = notification.GetServiceName();
                  if (service_name == "render") {
                    LOG_INFO("new render service detected")

                    // TODO: this operation blocks. Make it yield using
                    // Jobsystem.
                    auto maybe_caller = service_registry->Find("render").get();

                    if (maybe_caller.has_value()) {
                      auto caller = maybe_caller.value();
                      auto service_count = caller->GetCallableCount();
                      tiled_renderer->SetServiceCount(service_count);
                    }
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

    tiled_renderer_ref.Borrow()->SetServiceCount(render_service_count);
  } else {
    THROW_EXCEPTION(plugins::PluginSetupFailedException,
                    "subsystems are not accessible, but required")
  }
}

void TiledCompositeRendererPlugin::ShutDown(
    plugins::SharedPluginContext context) {}

std::string TiledCompositeRendererPlugin::GetName() {
  return "tiled-composite-renderer";
}

TiledCompositeRendererPlugin::~TiledCompositeRendererPlugin() {}

extern "C" BOOST_SYMBOL_EXPORT TiledCompositeRendererPlugin plugin;
TiledCompositeRendererPlugin plugin;