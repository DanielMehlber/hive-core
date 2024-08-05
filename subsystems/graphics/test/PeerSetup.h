#pragma once

#include "events/broker/impl/JobBasedEventBroker.h"
#include "graphics/renderer/impl/OffscreenRenderer.h"
#include "graphics/service/RenderService.h"
#include "graphics/service/RenderServiceRequest.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "networking/NetworkingManager.h"
#include "networking/messaging/IMessageEndpoint.h"
#include "services/registry/impl/remote/RemoteServiceRegistry.h"

using namespace hive::services;
using namespace hive::networking;
using namespace hive::graphics;
using namespace hive;

struct Node {
  std::string uuid;
  common::memory::Owner<common::subsystems::SubsystemManager> subsystems;
  common::memory::Reference<jobsystem::JobManager> job_manager;
  common::memory::Reference<IMessageEndpoint> endpoint;
  common::memory::Reference<IServiceRegistry> registry;
  int port;
};

Node setupNode(const common::config::SharedConfiguration &config, int port) {
  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();

  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);
  job_manager->StartExecution();
  auto job_manager_ref = job_manager.CreateReference();
  subsystems->AddOrReplaceSubsystem(std::move(job_manager));

  // setup event broker
  auto event_broker =
      common::memory::Owner<events::brokers::JobBasedEventBroker>(
          subsystems.CreateReference());
  subsystems->AddOrReplaceSubsystem<events::IEventBroker>(
      std::move(event_broker));

  auto property_provider = common::memory::Owner<data::PropertyProvider>(
      subsystems.CreateReference());
  auto property_providder_ref = property_provider.CreateReference();
  subsystems->AddOrReplaceSubsystem<data::PropertyProvider>(
      std::move(property_provider));

  // setup networking peer
  config->Set("net.port", port);
  auto networking_peer = common::memory::Owner<networking::NetworkingManager>(
      subsystems.CreateReference(), config);
  subsystems->AddOrReplaceSubsystem(std::move(networking_peer));

  auto endpoint_ref =
      subsystems->RequireSubsystem<IMessageEndpoint>().ToReference();

  // setup service registry
  common::memory::Owner<IServiceRegistry> registry =
      common::memory::Owner<services::impl::RemoteServiceRegistry>(
          subsystems.CreateReference());
  auto registry_ref = registry.CreateReference();
  subsystems->AddOrReplaceSubsystem<services::IServiceRegistry>(
      std::move(registry));

  std::string uuid =
      property_providder_ref.Borrow()->Get<std::string>("net.node.id").value();

  return Node{uuid,         std::move(subsystems), job_manager_ref,
              endpoint_ref, registry_ref,          port};
}
