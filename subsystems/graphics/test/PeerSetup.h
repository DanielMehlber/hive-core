#ifndef SIMULATION_FRAMEWORK_PEERSETUP_H
#define SIMULATION_FRAMEWORK_PEERSETUP_H

#include "events/EventFactory.h"
#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/service/RenderService.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "networking/NetworkingFactory.h"
#include "services/registry/impl/remote/RemoteServiceRegistry.h"

using namespace services;
using namespace networking;
using namespace graphics;

common::subsystems::SharedSubsystemManager
setupPeerNode(const jobsystem::SharedJobManager &job_manager,
              const common::config::SharedConfiguration &config, int port) {
  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  // setup event broker
  events::SharedEventBroker event_broker =
      events::EventFactory::CreateBroker(subsystems);
  subsystems->AddOrReplaceSubsystem(event_broker);

  // setup networking peer
  config->Set("net.port", port);
  auto networking_peer =
      networking::NetworkingFactory::CreateNetworkingPeer(subsystems, config);
  subsystems->AddOrReplaceSubsystem(networking_peer);

  // setup service registry
  SharedServiceRegistry registry =
      std::make_shared<services::impl::RemoteServiceRegistry>(subsystems);
  subsystems->AddOrReplaceSubsystem<services::IServiceRegistry>(registry);

  return subsystems;
}

#endif // SIMULATION_FRAMEWORK_PEERSETUP_H
