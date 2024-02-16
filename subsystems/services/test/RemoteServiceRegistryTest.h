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

TEST(ServiceTests, run_single_remote_service) {
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();

  auto peer_1 = setupPeerNode(job_manager, config, 9005);
  auto peer_2 = setupPeerNode(job_manager, config, 9006);

  // first establish connection in order to broadcast the connection
  auto peer_1_networking_subsystem =
      peer_1->RequireSubsystem<IMessageEndpoint>();
  auto connection_progress =
      peer_1_networking_subsystem->EstablishConnectionTo("127.0.0.1:9006");

  connection_progress.wait();
  ASSERT_NO_THROW(connection_progress.get());

  SharedServiceExecutor local_service =
      std::make_shared<AddingServiceExecutor>();

  auto peer_1_service_registry = peer_1->RequireSubsystem<IServiceRegistry>();
  peer_1_service_registry->Register(local_service);
  job_manager->InvokeCycleAndWait();

  auto peer_2_service_registry = peer_2->RequireSubsystem<IServiceRegistry>();
  TryAssertUntilTimeout(
      [&peer_2_service_registry, job_manager] {
        job_manager->InvokeCycleAndWait();
        return peer_2_service_registry->Find("add").get().has_value();
      },
      10s);

  SharedServiceCaller caller =
      peer_2_service_registry->Find("add").get().value();
  auto result_fut = caller->Call(GenerateAddingRequest(3, 5), job_manager);
  job_manager->InvokeCycleAndWait();

  SharedServiceResponse response;
  ASSERT_NO_THROW(response = result_fut.get());
  ASSERT_EQ(8, GetResultOfAddition(response));
}

TEST(ServiceTests, remote_service_load_balancing) {
  auto config = std::make_shared<common::config::Configuration>();
  config->Set("jobs.concurrency", (int)std::thread::hardware_concurrency());
  auto job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();

  auto main_peer = setupPeerNode(job_manager, config, 9004);

  auto main_peer_registry = main_peer->RequireSubsystem<IServiceRegistry>();
  auto main_peer_networking_subsystem =
      main_peer->RequireSubsystem<IMessageEndpoint>();

  std::vector<std::pair<common::subsystems::SharedSubsystemManager,
                        std::shared_ptr<AddingServiceExecutor>>>
      peers;

  for (int i = 9005; i < 9010; i++) {

    auto ith_peer = setupPeerNode(job_manager, config, i);

    auto ith_peer_registry = ith_peer->RequireSubsystem<IServiceRegistry>();
    auto ith_peer_networking_subsystem =
        ith_peer->RequireSubsystem<IMessageEndpoint>();

    // first establish connection in order to broadcast the connection
    auto connection_progress =
        ith_peer_networking_subsystem->EstablishConnectionTo("127.0.0.1:9004");

    connection_progress.wait();
    ASSERT_NO_THROW(connection_progress.get());

    std::shared_ptr<AddingServiceExecutor> service =
        std::make_shared<AddingServiceExecutor>();

    peers.emplace_back(ith_peer, service);
  }

  // process established connection before registering & broadcasting service
  job_manager->InvokeCycleAndWait();

  // register service at peers
  for (auto &[peer, service] : peers) {
    auto peer_service_registry = peer->RequireSubsystem<IServiceRegistry>();
    peer_service_registry->Register(service);
  }

  job_manager->InvokeCycleAndWait();

  // wait until remote services have been registered due to service broadcast
  TryAssertUntilTimeout(
      [main_peer_registry, job_manager] {
        job_manager->InvokeCycleAndWait();
        auto opt_caller = main_peer_registry->Find("add").get();
        if (!opt_caller.has_value()) {
          return false;
        }

        auto caller = opt_caller.value();
        return caller->GetCallableCount() == 5;
      },
      10s);

  // call service 5 times (should equally distribute among all peers)
  auto caller = main_peer_registry->Find("add").get().value();
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

    ASSERT_NO_THROW(future.get());
  }

  // check that each service has been called 1 time (round-robin)
  for (auto &[peer, service] : peers) {
    ASSERT_EQ(1, service->count);
  }
}

TEST(ServiceTests, web_socket_peer_destroyed) {
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();

  auto peer_1 = setupPeerNode(job_manager, config, 9005);
  auto peer_1_service_registry = peer_1->RequireSubsystem<IServiceRegistry>();
  auto peer_1_networking_subsystem =
      peer_1->RequireSubsystem<IMessageEndpoint>();

  {
    auto peer_2 = setupPeerNode(job_manager, config, 9006);
    auto peer_2_networking_subsystem =
        peer_2->RequireSubsystem<IMessageEndpoint>();
    auto peer_2_service_registry = peer_2->RequireSubsystem<IServiceRegistry>();

    auto progress =
        peer_2_networking_subsystem->EstablishConnectionTo("127.0.0.1:9005");

    ASSERT_NO_THROW(progress.get());

    SharedServiceExecutor adding_service =
        std::make_shared<AddingServiceExecutor>();
    peer_2_service_registry->Register(adding_service);

    // wait until service is available to node1 (because node2 provides it)
    TryAssertUntilTimeout(
        [job_manager, &peer_1_service_registry] {
          job_manager->InvokeCycleAndWait();
          return peer_1_service_registry->Find("add").get().has_value();
        },
        10s);
  }

  // we need to wait until the connection closed
  TryAssertUntilTimeout(
      [job_manager, peer_1_networking_subsystem] {
        job_manager->InvokeCycleAndWait();
        return peer_1_networking_subsystem->GetActiveConnectionCount() == 0;
      },
      10s);

  // scenario: both node2 and its services are gone.
  ASSERT_FALSE(peer_1_service_registry->Find("add").get().has_value());
}

#endif // WEBSOCKETMESSAGEREGISTRYTEST_H
