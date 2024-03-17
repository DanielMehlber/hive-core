#ifndef WEBSOCKETMESSAGEREGISTRYTEST_H
#define WEBSOCKETMESSAGEREGISTRYTEST_H

#include "AddingServiceExecutor.h"
#include "common/test/TryAssertUntilTimeout.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/NetworkingManager.h"
#include "services/executor/impl/LocalServiceExecutor.h"
#include "services/registry/impl/remote/RemoteServiceRegistry.h"
#include <gtest/gtest.h>

using namespace jobsystem;
using namespace services;
using namespace services::impl;
using namespace networking;
using namespace common::test;

common::memory::Owner<common::subsystems::SubsystemManager>
setupNode(const common::config::SharedConfiguration &config, int port) {
  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();

  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);
  job_manager->StartExecution();
  subsystems->AddOrReplaceSubsystem<JobManager>(std::move(job_manager));

  // setup event broker
  auto event_broker =
      common::memory::Owner<events::brokers::JobBasedEventBroker>(
          subsystems.CreateReference());
  subsystems->AddOrReplaceSubsystem<events::IEventBroker>(
      std::move(event_broker));

  // setup networking node
  config->Set("net.port", port);
  auto networking_node = common::memory::Owner<networking::NetworkingManager>(
      subsystems.CreateReference(), config);
  subsystems->AddOrReplaceSubsystem<networking::NetworkingManager>(
      std::move(networking_node));

  // setup service registry
  auto service_registry =
      common::memory::Owner<services::impl::RemoteServiceRegistry>(
          subsystems.CreateReference());
  subsystems->AddOrReplaceSubsystem<services::IServiceRegistry>(
      std::move(service_registry));

  return subsystems;
}

TEST(ServiceTests, run_single_remote_service) {
  auto config = std::make_shared<common::config::Configuration>();

  auto node_1 = setupNode(config, 9005);
  auto node_2 = setupNode(config, 9006);

  auto node_1_job_manager = node_1->RequireSubsystem<jobsystem::JobManager>();
  auto node_2_job_manager = node_2->RequireSubsystem<jobsystem::JobManager>();

  // first establish connection in order to broadcast the connection
  auto node_1_networking_subsystem =
      node_1->RequireSubsystem<IMessageEndpoint>();
  auto connection_progress =
      node_1_networking_subsystem->EstablishConnectionTo("127.0.0.1:9006");

  connection_progress.wait();
  ASSERT_NO_THROW(connection_progress.get());

  SharedServiceExecutor local_service =
      std::make_shared<AddingServiceExecutor>();

  auto node_1_service_registry = node_1->RequireSubsystem<IServiceRegistry>();
  node_1_service_registry->Register(local_service);
  node_2_job_manager->InvokeCycleAndWait();

  auto node_2_service_registry = node_2->RequireSubsystem<IServiceRegistry>();
  TryAssertUntilTimeout(
      [&node_2_service_registry, node_2_job_manager]() mutable {
        node_2_job_manager->InvokeCycleAndWait();
        return node_2_service_registry->Find("add").get().has_value();
      },
      10s);

  SharedServiceCaller caller =
      node_2_service_registry->Find("add").get().value();
  auto result_fut =
      caller->Call(GenerateAddingRequest(3, 5), node_2_job_manager);
  node_2_job_manager->InvokeCycleAndWait();

  SharedServiceResponse response;
  ASSERT_NO_THROW(response = result_fut.get());
  ASSERT_EQ(8, GetResultOfAddition(response));

  node_1_job_manager->StopExecution();
  node_2_job_manager->StopExecution();
}

TEST(ServiceTests, remote_service_load_balancing) {
  auto config = std::make_shared<common::config::Configuration>();
  config->Set("jobs.concurrency", (int)std::thread::hardware_concurrency());

  auto central_node = setupNode(config, 9004);

  auto central_node_registry =
      central_node->RequireSubsystem<IServiceRegistry>();
  auto central_node_networking_subsystem =
      central_node->RequireSubsystem<IMessageEndpoint>();
  auto central_node_job_manager = central_node->RequireSubsystem<JobManager>();

  std::vector<
      std::pair<common::memory::Owner<common::subsystems::SubsystemManager>,
                std::shared_ptr<AddingServiceExecutor>>>
      nodes;

  for (int i = 9005; i < 9010; i++) {

    auto ith_node = setupNode(config, i);

    auto ith_node_registry = ith_node->RequireSubsystem<IServiceRegistry>();
    auto ith_node_networking_subsystem =
        ith_node->RequireSubsystem<IMessageEndpoint>();

    // first establish connection in order to broadcast the connection
    auto connection_progress =
        ith_node_networking_subsystem->EstablishConnectionTo("127.0.0.1:9004");

    connection_progress.wait();
    ASSERT_NO_THROW(connection_progress.get());

    std::shared_ptr<AddingServiceExecutor> service =
        std::make_shared<AddingServiceExecutor>();

    nodes.emplace_back(std::move(ith_node), service);
  }

  // process established connection before registering & broadcasting service
  central_node_job_manager->InvokeCycleAndWait();
  for (auto &[node, service] : nodes) {
    auto node_job_manager = node->RequireSubsystem<jobsystem::JobManager>();
    node_job_manager->InvokeCycleAndWait();
  }

  // register service at nodes
  for (auto &[node, service] : nodes) {
    auto node_service_registry = node->RequireSubsystem<IServiceRegistry>();
    node_service_registry->Register(service);

    auto node_job_manager = node->RequireSubsystem<jobsystem::JobManager>();
    node_job_manager->InvokeCycleAndWait();
  }

  // wait until remote services have been registered due to service broadcast
  TryAssertUntilTimeout(
      [&central_node_registry, &central_node_job_manager]() {
        central_node_job_manager->InvokeCycleAndWait();
        auto maybe_caller = central_node_registry->Find("add").get();
        if (!maybe_caller.has_value()) {
          return false;
        }

        auto caller = maybe_caller.value();
        return caller->GetCallableCount() == 5;
      },
      10s);

  // call service 5 times (should equally distribute among all nodes)
  auto caller = central_node_registry->Find("add").get().value();
  std::vector<std::future<SharedServiceResponse>> responses;
  for (int i = 0; i < 5; i++) {
    auto response =
        caller->Call(GenerateAddingRequest(1, i), central_node_job_manager);
    responses.push_back(std::move(response));
  }

  central_node_job_manager->InvokeCycleAndWait();

  // wait until all responses arrived
  for (auto &future : responses) {
    TryAssertUntilTimeout(
        [&future, &nodes] {
          for (auto &[node, service] : nodes) {
            auto node_job_manager =
                node->RequireSubsystem<jobsystem::JobManager>();
            node_job_manager->InvokeCycleAndWait();
          }
          return future.wait_for(0s) == std::future_status::ready;
        },
        10s);

    ASSERT_NO_THROW(future.get());
  }

  // check that each service has been called 1 time (round-robin)
  for (auto &[node, service] : nodes) {
    ASSERT_EQ(1, service->count);
  }

  central_node_job_manager->StopExecution();
}

TEST(ServiceTests, web_socket_node_destroyed) {
  auto config = std::make_shared<common::config::Configuration>();

  auto node_1 = setupNode(config, 9005);
  auto node_1_service_registry = node_1->RequireSubsystem<IServiceRegistry>();
  auto node_1_networking_subsystem =
      node_1->RequireSubsystem<IMessageEndpoint>();
  auto node_1_job_manager = node_1->RequireSubsystem<jobsystem::JobManager>();

  {
    auto node_2 = setupNode(config, 9006);
    auto node_2_networking_subsystem =
        node_2->RequireSubsystem<IMessageEndpoint>();
    auto node_2_service_registry = node_2->RequireSubsystem<IServiceRegistry>();
    auto node_2_job_manager = node_2->RequireSubsystem<jobsystem::JobManager>();

    auto progress =
        node_2_networking_subsystem->EstablishConnectionTo("127.0.0.1:9005");

    ASSERT_NO_THROW(progress.get());

    node_1_job_manager->InvokeCycleAndWait();
    node_2_job_manager->InvokeCycleAndWait();

    SharedServiceExecutor adding_service =
        std::make_shared<AddingServiceExecutor>();
    node_2_service_registry->Register(adding_service);

    node_2_job_manager->InvokeCycleAndWait();

    // wait until service is available to node1 (because node2 provides it)
    TryAssertUntilTimeout(
        [&node_1_job_manager, &node_1_service_registry] {
          node_1_job_manager->InvokeCycleAndWait();
          return node_1_service_registry->Find("add").get().has_value();
        },
        10s);
  }

  // we need to wait until the connection closed
  TryAssertUntilTimeout(
      [&node_1_job_manager, &node_1_networking_subsystem] {
        node_1_job_manager->InvokeCycleAndWait();
        return node_1_networking_subsystem->GetActiveConnectionCount() == 0;
      },
      10s);

  // scenario: both node2 and its services are gone.
  ASSERT_FALSE(node_1_service_registry->Find("add").get().has_value());
}

#endif // WEBSOCKETMESSAGEREGISTRYTEST_H
