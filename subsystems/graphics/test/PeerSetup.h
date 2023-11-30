#ifndef SIMULATION_FRAMEWORK_PEERSETUP_H
#define SIMULATION_FRAMEWORK_PEERSETUP_H

#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/service/RenderService.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "messaging/MessagingFactory.h"
#include "networking/NetworkingFactory.h"
#include "services/registry/impl/websockets/WebSocketServiceRegistry.h"

#define REGISTRY_OF(x) std::get<1>(x)
#define WEB_SOCKET_OF(x) std::get<0>(x)
#define NODE std::tuple<SharedWebSocketPeer, SharedServiceRegistry>

using namespace services;
using namespace networking;
using namespace graphics;

SharedWebSocketPeer
setupPeer(size_t port,
          const common::subsystems::SharedSubsystemManager &subsystems) {

  props::SharedPropertyProvider property_provider =
      std::make_shared<props::PropertyProvider>(subsystems);

  property_provider->Set("net.ws.port", port);

  subsystems->AddOrReplaceSubsystem(property_provider);

  return NetworkingFactory::CreateWebSocketPeer(subsystems);
}

SharedServiceRequest GenerateRenderingRequest(int width, int height) {
  auto request = std::make_shared<ServiceRequest>("render");
  request->SetParameter("width", width);
  request->SetParameter("height", height);
  return request;
}

NODE setupNode(size_t port,
               const common::subsystems::SharedSubsystemManager &subsystems) {
  // setup first peer
  SharedWebSocketPeer web_socket_peer = setupPeer(port, subsystems);

  subsystems->AddOrReplaceSubsystem(web_socket_peer);

  SharedServiceRegistry registry =
      std::make_shared<services::impl::WebSocketServiceRegistry>(subsystems);

  subsystems->AddOrReplaceSubsystem(registry);

  return {web_socket_peer, registry};
}

common::subsystems::SharedSubsystemManager SetupSubsystems() {
  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();

  SharedJobManager job_manager = std::make_shared<JobManager>();
  job_manager->StartExecution();

  subsystems->AddOrReplaceSubsystem(job_manager);

  messaging::SharedBroker message_broker =
      messaging::MessagingFactory::CreateBroker(subsystems);

  subsystems->AddOrReplaceSubsystem(message_broker);

  return subsystems;
}

#endif // SIMULATION_FRAMEWORK_PEERSETUP_H
