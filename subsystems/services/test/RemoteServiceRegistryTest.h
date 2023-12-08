#ifndef WEBSOCKETMESSAGEREGISTRYTEST_H
#define WEBSOCKETMESSAGEREGISTRYTEST_H

#include "AddingServiceExecutor.h"
#include "common/test/TryAssertUntilTimeout.h"
#include "events/EventFactory.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/NetworkingFactory.h"
#include "services/executor/impl/LocalServiceExecutor.h"
#include "services/registry/impl/remote/RemoteServiceRegistry.h"
#include <gtest/gtest.h>

using namespace jobsystem;
using namespace services;
using namespace services::impl;
using namespace networking;
using namespace common::test;

#define REGISTRY_OF(x) std::get<1>(x)
#define WEB_SOCKET_OF(x) std::get<0>(x)
#define NODE std::tuple<SharedWebSocketPeer, SharedServiceRegistry>

common::subsystems::SharedSubsystemManager SetupSubsystems() {
  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  SharedJobManager job_manager = std::make_shared<JobManager>();
  job_manager->StartExecution();
  subsystems->AddOrReplaceSubsystem(job_manager);

  events::SharedEventBroker message_broker =
      events::EventFactory::CreateBroker(subsystems);
  subsystems->AddOrReplaceSubsystem(message_broker);

  return subsystems;
}

SharedWebSocketPeer
setupPeer(size_t port,
          const common::subsystems::SharedSubsystemManager &subsystems) {
  props::SharedPropertyProvider property_provider =
      std::make_shared<props::PropertyProvider>(subsystems);

  subsystems->AddOrReplaceSubsystem(property_provider);

  property_provider->Set("net.ws.port", port);

  return NetworkingFactory::CreateWebSocketPeer(subsystems);
}

NODE setupNode(size_t port,
               const common::subsystems::SharedSubsystemManager &subsystems) {
  // setup first peer
  SharedWebSocketPeer web_socket_peer = setupPeer(port, subsystems);
  subsystems->AddOrReplaceSubsystem(web_socket_peer);

  SharedServiceRegistry registry =
      std::make_shared<services::impl::RemoteServiceRegistry>(subsystems);

  return {web_socket_peer, registry};
}

TEST(ServiceTests, web_socket_run_single_remote_service) {
  auto subsystems_1 = SetupSubsystems();
  auto job_manager = subsystems_1->RequireSubsystem<JobManager>();

  auto subsystems_2 =
      std::make_shared<common::subsystems::SubsystemManager>(*subsystems_1);

  // each subsystem needs its own event broker
  SharedEventBroker event_broker_2 = EventFactory::CreateBroker(subsystems_2);
  subsystems_2->AddOrReplaceSubsystem<IEventBroker>(event_broker_2);

  NODE node1 = setupNode(9005, subsystems_1);
  NODE node2 = setupNode(9006, subsystems_2);

  // first establish connection in order to broadcast the connection
  auto connection_progress =
      WEB_SOCKET_OF(node1)->EstablishConnectionTo("127.0.0.1:9006");

  connection_progress.wait();
  ASSERT_NO_THROW(connection_progress.get());

  SharedServiceExecutor local_service =
      std::make_shared<AddingServiceExecutor>();
  REGISTRY_OF(node1)->Register(local_service);
  job_manager->InvokeCycleAndWait();

  TryAssertUntilTimeout(
      [&node2, job_manager] {
        job_manager->InvokeCycleAndWait();
        return REGISTRY_OF(node2)->Find("add").get().has_value();
      },
      10s);

  SharedServiceCaller caller = REGISTRY_OF(node2)->Find("add").get().value();
  auto result_fut = caller->Call(GenerateAddingRequest(3, 5), job_manager);
  job_manager->InvokeCycleAndWait();

  SharedServiceResponse response;
  ASSERT_NO_THROW(response = result_fut.get());
  ASSERT_EQ(8, GetResultOfAddition(response));
}

TEST(ServiceTests, web_service_load_balancing) {
  auto subsystems = SetupSubsystems();
  auto job_manager = subsystems->RequireSubsystem<JobManager>();

  auto subsystems_first_copy =
      std::make_shared<common::subsystems::SubsystemManager>(*subsystems);
  NODE main_node = setupNode(9004, subsystems_first_copy);
  SharedServiceRegistry main_registry = REGISTRY_OF(main_node);
  SharedWebSocketPeer main_web_socket_peer = WEB_SOCKET_OF(main_node);

  std::vector<std::tuple<SharedWebSocketPeer, SharedServiceRegistry,
                         std::shared_ptr<AddingServiceExecutor>,
                         common::subsystems::SharedSubsystemManager>>
      peers;

  for (int i = 9005; i < 9010; i++) {
    auto subsystems_copy =
        std::make_shared<common::subsystems::SubsystemManager>(*subsystems);

    // each subsystem needs its own event broker
    SharedEventBroker event_broker =
        EventFactory::CreateBroker(subsystems_copy);
    subsystems_copy->AddOrReplaceSubsystem<IEventBroker>(event_broker);

    NODE node = setupNode(i, subsystems_copy);

    SharedWebSocketPeer web_socket_peer = WEB_SOCKET_OF(node);
    SharedServiceRegistry registry = REGISTRY_OF(node);

    // first establish connection in order to broadcast the connection
    auto connection_progress =
        web_socket_peer->EstablishConnectionTo("127.0.0.1:9004");

    connection_progress.wait();
    ASSERT_NO_THROW(connection_progress.get());

    std::shared_ptr<AddingServiceExecutor> service =
        std::make_shared<AddingServiceExecutor>();

    peers.emplace_back(web_socket_peer, registry, service, subsystems_copy);
  }

  // register service at peers
  for (auto &peer : peers) {
    REGISTRY_OF(peer)->Register(std::get<2>(peer));
  }
  job_manager->InvokeCycleAndWait();

  // wait until remote services have been registered due to service broadcast
  TryAssertUntilTimeout(
      [main_registry, job_manager] {
        job_manager->InvokeCycleAndWait();
        auto opt_caller = main_registry->Find("add").get();
        if (!opt_caller.has_value()) {
          return false;
        }

        auto caller = opt_caller.value();
        return caller->GetCallableCount() >= 5;
      },
      10s);

  // call service 5 times (should equally distribute among all peers)
  auto caller = main_registry->Find("add").get().value();
  std::vector<std::future<SharedServiceResponse>> responses;
  for (int i = 0; i < 5; i++) {
    auto response = caller->Call(GenerateAddingRequest(1, i), job_manager);
    responses.push_back(std::move(response));
  }

  // wait until all responses arrived
  for (auto &future : responses) {
    TryAssertUntilTimeout(
        [&future, job_manager] {
          job_manager->InvokeCycleAndWait();
          return future.wait_for(0s) == std::future_status::ready;
        },
        10s);
  }

  for (auto &peer : peers) {
    ASSERT_EQ(1, std::get<2>(peer)->count);
  }
}

TEST(ServiceTests, web_socket_peer_destroyed) {
  auto subsystems_1 = SetupSubsystems();
  auto job_manager = subsystems_1->RequireSubsystem<JobManager>();

  auto node1 = setupNode(9005, subsystems_1);
  std::string connected_port;

  {
    auto subsystems_2 =
        std::make_shared<common::subsystems::SubsystemManager>(*subsystems_1);

    // each subsystem needs its own event broker
    SharedEventBroker event_broker_2 = EventFactory::CreateBroker(subsystems_2);
    subsystems_2->AddOrReplaceSubsystem<IEventBroker>(event_broker_2);

    auto node2 = setupNode(9006, subsystems_2);
    auto progress =
        WEB_SOCKET_OF(node2)->EstablishConnectionTo("127.0.0.1:9005");

    ASSERT_NO_THROW(progress.get());

    SharedServiceExecutor adding_service =
        std::make_shared<AddingServiceExecutor>();
    REGISTRY_OF(node2)->Register(adding_service);

    // wait until service is available to node1 (because node2 provides it)
    TryAssertUntilTimeout(
        [job_manager, &node1] {
          job_manager->InvokeCycleAndWait();
          return REGISTRY_OF(node1)->Find("add").get().has_value();
        },
        10s);
  }

  // we need to wait until the connection closed
  TryAssertUntilTimeout(
      [job_manager, &node1] {
        job_manager->InvokeCycleAndWait();
        return WEB_SOCKET_OF(node1)->GetActiveConnectionCount() == 0;
      },
      10s);

  // scenario: both node2 and its services are gone.
  ASSERT_FALSE(REGISTRY_OF(node1)->Find("add").get().has_value());
}

#endif // WEBSOCKETMESSAGEREGISTRYTEST_H
