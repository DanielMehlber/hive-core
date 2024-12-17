#pragma once

#include "data/DataLayer.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"
#include "networking/NetworkingManager.h"
#include "services/registry/impl/p2p/PeerToPeerServiceRegistry.h"

using namespace hive::services;
using namespace hive::networking;
using namespace hive::graphics;
using namespace hive;
using namespace hive::networking::messaging;
using namespace hive::data;

struct Node {
  std::string uuid;
  common::memory::Owner<common::subsystems::SubsystemManager> subsystems;
  common::memory::Reference<jobsystem::JobManager> job_manager;
  common::memory::Reference<NetworkingManager> networking_mgr;
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

  auto data_layer = common::memory::Owner<DataLayer>(subsystems.Borrow());
  auto data_layer_ref = data_layer.CreateReference();
  subsystems->AddOrReplaceSubsystem<DataLayer>(std::move(data_layer));

  // setup networking manager
  config->Set("net.websocket.port", port);
  auto networking_manager = common::memory::Owner<NetworkingManager>(
      subsystems.CreateReference(), config);

  auto networking_manager_ref = networking_manager.CreateReference();

  subsystems->AddOrReplaceSubsystem(std::move(networking_manager));

  // setup service registry
  common::memory::Owner<IServiceRegistry> registry =
      common::memory::Owner<impl::PeerToPeerServiceRegistry>(
          subsystems.CreateReference());
  auto registry_ref = registry.CreateReference();
  subsystems->AddOrReplaceSubsystem<IServiceRegistry>(std::move(registry));

  std::string uuid = data_layer_ref.Borrow()->Get("net.node.id").get().value();

  return Node{uuid,
              std::move(subsystems),
              job_manager_ref,
              networking_manager_ref,
              registry_ref,
              port};
}
