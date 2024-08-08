#pragma once

#include "AddingServiceExecutor.h"
#include "DelayServiceExecutor.h"
#include "LimitedLocalService.h"
#include "common/test/TryAssertUntilTimeout.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "jobsystem/manager/JobManager.h"
#include "networking/NetworkingManager.h"
#include "services/executor/impl/LocalServiceExecutor.h"
#include "services/registry/impl/remote/RemoteServiceRegistry.h"
#include <gtest/gtest.h>

using namespace hive::jobsystem;
using namespace hive::services;
using namespace hive::services::impl;
using namespace hive::networking;
using namespace hive::common::test;

struct Node {
  std::string uuid;
  common::memory::Owner<common::subsystems::SubsystemManager> subsystems;
  common::memory::Reference<JobManager> job_manager;
  common::memory::Reference<IServiceRegistry> service_registry;
  common::memory::Reference<IMessageEndpoint> network_endpoint;
  int port;
};

Node setupNode(const common::config::SharedConfiguration &config, int port) {
  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();

  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);
  job_manager->StartExecution();
  auto job_manager_ref = job_manager.CreateReference();

  subsystems->AddOrReplaceSubsystem<JobManager>(std::move(job_manager));

  // setup event broker
  auto event_broker =
      common::memory::Owner<events::brokers::JobBasedEventBroker>(
          subsystems.CreateReference());
  subsystems->AddOrReplaceSubsystem<events::IEventBroker>(
      std::move(event_broker));

  auto property_provider = common::memory::Owner<data::PropertyProvider>(
      subsystems.CreateReference());
  auto property_provider_ref = property_provider.CreateReference();
  subsystems->AddOrReplaceSubsystem<data::PropertyProvider>(
      std::move(property_provider));

  // setup networking node
  config->Set("net.port", port);
  auto networking_node = common::memory::Owner<networking::NetworkingManager>(
      subsystems.CreateReference(), config);
  auto networking_endpoint_ref =
      subsystems->RequireSubsystem<IMessageEndpoint>().ToReference();
  subsystems->AddOrReplaceSubsystem<networking::NetworkingManager>(
      std::move(networking_node));

  std::string node_id =
      property_provider_ref.Borrow()->Get<std::string>("net.node.id").value();

  // setup service registry
  auto service_registry =
      common::memory::Owner<services::impl::RemoteServiceRegistry>(
          subsystems.CreateReference());
  auto service_registry_ref = service_registry.CreateReference();
  subsystems->AddOrReplaceSubsystem<services::IServiceRegistry>(
      std::move(service_registry));

  return Node{node_id,
              std::move(subsystems),
              job_manager_ref,
              service_registry_ref,
              networking_endpoint_ref,
              port};
}

TEST(ServiceTests, run_single_remote_service) {
  auto config = std::make_shared<common::config::Configuration>();

  auto node_1 = setupNode(config, 9005);
  auto node_2 = setupNode(config, 9006);

  // first establish connection in order to broadcast the connection
  auto connection_progress =
      node_1.network_endpoint.Borrow()->EstablishConnectionTo("127.0.0.1:9006");

  connection_progress.wait();
  ASSERT_NO_THROW(connection_progress.get());

  SharedServiceExecutor local_service =
      std::make_shared<AddingServiceExecutor>();

  node_1.service_registry.Borrow()->Register(local_service);
  node_1.job_manager.Borrow()->InvokeCycleAndWait();

  TryAssertUntilTimeout(
      [&node_2]() mutable {
        node_2.job_manager.Borrow()->InvokeCycleAndWait();
        return node_2.service_registry.Borrow()->Find("add").get().has_value();
      },
      10s);

  SharedServiceCaller caller =
      node_2.service_registry.Borrow()->Find("add").get().value();
  auto result_fut = caller->IssueCallAsJob(GenerateAddingRequest(3, 5),
                                           node_2.job_manager.Borrow());

  // we need a second threads here: node_1 has to send its call and node_2 needs
  // to process and respond to it. These actions usually take place in different
  // processes, so they must be executed in parallel.
  std::atomic_bool finished = false;
  auto node_1_request_processing = std::thread([&node_1, &finished]() {
    auto job_manager = node_1.job_manager.Borrow();
    while (!finished.load()) {
      job_manager->InvokeCycleAndWait();
    }
  });

  node_2.job_manager.Borrow()->InvokeCycleAndWait();
  finished.store(true);
  node_1_request_processing.join();

  SharedServiceResponse response;
  ASSERT_NO_THROW(response = result_fut.get());
  ASSERT_EQ(8, GetResultOfAddition(response));

  node_1.job_manager.Borrow()->StopExecution();
  node_2.job_manager.Borrow()->StopExecution();
}

TEST(ServiceTests, remote_service_load_balancing) {
  auto config = std::make_shared<common::config::Configuration>();

  auto central_node = setupNode(config, 9004);

  std::vector<std::pair<Node, std::shared_ptr<AddingServiceExecutor>>> nodes;

  for (int i = 9005; i < 9010; i++) {

    auto ith_node = setupNode(config, i);

    // first establish connection in order to broadcast the connection
    auto connection_progress =
        ith_node.network_endpoint.Borrow()->EstablishConnectionTo(
            "127.0.0.1:9004");

    connection_progress.wait();
    ASSERT_NO_THROW(connection_progress.get());

    std::shared_ptr<AddingServiceExecutor> service =
        std::make_shared<AddingServiceExecutor>();

    nodes.emplace_back(std::move(ith_node), service);
  }

  // process established connection before registering & broadcasting service
  central_node.job_manager.Borrow()->InvokeCycleAndWait();
  for (auto &[node, service] : nodes) {
    node.job_manager.Borrow()->InvokeCycleAndWait();
  }

  // register service at nodes
  for (auto &[node, service] : nodes) {
    node.service_registry.Borrow()->Register(service);
    node.job_manager.Borrow()->InvokeCycleAndWait();
  }

  // wait until remote services have been registered due to service broadcast
  TryAssertUntilTimeout(
      [&central_node]() {
        central_node.job_manager.Borrow()->InvokeCycleAndWait();
        auto maybe_caller =
            central_node.service_registry.Borrow()->Find("add").get();
        if (!maybe_caller.has_value()) {
          return false;
        }

        auto caller = maybe_caller.value();
        return caller->GetCallableCount() == 5;
      },
      10s);

  // call service 5 times (should equally distribute among all nodes)
  auto caller =
      central_node.service_registry.Borrow()->Find("add").get().value();
  std::vector<std::future<SharedServiceResponse>> responses;
  for (int i = 0; i < 5; i++) {
    auto response = caller->IssueCallAsJob(GenerateAddingRequest(1, i),
                                           central_node.job_manager.Borrow());
    responses.push_back(std::move(response));
  }

  // we need a second thread here: while the central node waits, the other nodes
  // must resolve its service request. This can't be done in the same thread
  // sequentially, but only in parallel.
  std::atomic_bool finished = false;
  std::thread nodes_request_processing = std::thread([&nodes, &finished]() {
    while (!finished.load()) {
      for (auto &node : nodes) {
        node.first.job_manager.Borrow()->InvokeCycleAndWait();
      }
    }
  });

  central_node.job_manager.Borrow()->InvokeCycleAndWait();
  finished.store(true);
  nodes_request_processing.join();

  // wait until all responses arrived
  for (auto &future : responses) {
    TryAssertUntilTimeout(
        [&future, &nodes] {
          for (auto &[node, service] : nodes) {
            node.job_manager.Borrow()->InvokeCycleAndWait();
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

  central_node.job_manager.Borrow()->StopExecution();
}

TEST(ServiceTests, web_socket_node_destroyed) {
  auto config = std::make_shared<common::config::Configuration>();

  auto node_1 = setupNode(config, 9005);

  {
    auto node_2 = setupNode(config, 9006);

    auto progress = node_2.network_endpoint.Borrow()->EstablishConnectionTo(
        "127.0.0.1:9005");

    ASSERT_NO_THROW(progress.get());

    node_1.job_manager.Borrow()->InvokeCycleAndWait();
    node_2.job_manager.Borrow()->InvokeCycleAndWait();

    SharedServiceExecutor adding_service =
        std::make_shared<AddingServiceExecutor>();
    node_2.service_registry.Borrow()->Register(adding_service);

    node_2.job_manager.Borrow()->InvokeCycleAndWait();

    // wait until service is available to node1 (because node2 provides it)
    TryAssertUntilTimeout(
        [&node_1] {
          node_1.job_manager.Borrow()->InvokeCycleAndWait();
          return node_1.service_registry.Borrow()
              ->Find("add")
              .get()
              .has_value();
        },
        10s);
  }

  // we need to wait until the connection closed
  TryAssertUntilTimeout(
      [&node_1] {
        node_1.job_manager.Borrow()->InvokeCycleAndWait();
        return node_1.network_endpoint.Borrow()->GetActiveConnectionCount() ==
               0;
      },
      10s);

  // scenario: both node2 and its services are gone.
  ASSERT_FALSE(node_1.service_registry.Borrow()->Find("add").get().has_value());
}

TEST(ServiceTest, async_service_call) {
  auto config = std::make_shared<common::config::Configuration>();

  auto node_1 = setupNode(config, 9005);
  auto node_2 = setupNode(config, 9006);

  auto connection_progress =
      node_1.network_endpoint.Borrow()->EstablishConnectionTo("127.0.0.1:9006");

  connection_progress.wait();
  ASSERT_NO_THROW(connection_progress.get());

  SharedServiceExecutor local_service =
      std::make_shared<DelayServiceExecutor>();

  node_1.service_registry.Borrow()->Register(local_service);
  node_1.job_manager.Borrow()->InvokeCycleAndWait();

  TryAssertUntilTimeout(
      [&node_2]() mutable {
        node_2.job_manager.Borrow()->InvokeCycleAndWait();
        return node_2.service_registry.Borrow()
            ->Find("delay")
            .get()
            .has_value();
      },
      10s);

  SharedServiceCaller caller =
      node_2.service_registry.Borrow()->Find("delay").get().value();

  // call remote service of node 1 asynchronously
  auto result_fut =
      caller->IssueCallAsJob(GenerateDelayRequest(500 /*ms*/),
                             node_2.job_manager.Borrow(), false, true);

  // we need a second thread here: node_1 has to send its call and node_2 needs
  // to process and respond to it. These actions usually take place in different
  // processes, so they must be executed in parallel.
  std::atomic_bool finished = false;
  std::atomic_int cycles = 0;
  auto node_1_request_processing = std::thread([&node_1, &finished, &cycles]() {
    auto job_manager = node_1.job_manager.Borrow();
    while (!finished.load()) {
      job_manager->InvokeCycleAndWait();
    }
  });

  while (result_fut.wait_for(0s) != std::future_status::ready) {
    node_2.job_manager.Borrow()->InvokeCycleAndWait();
    cycles++;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  finished.store(true);
  node_1_request_processing.join();

  // assert that requesting node was not blocked and completed multiple cycles
  ASSERT_TRUE(cycles > 1);
}

TEST(ServiceTest, service_peer_disconnected) {
  auto config = std::make_shared<common::config::Configuration>();

  auto node_1 = setupNode(config, 9005);

  std::future<std::shared_ptr<ServiceResponse>> future_response;

  {
    auto node_2 = setupNode(config, 9006);

    auto connection_progress =
        node_1.network_endpoint.Borrow()->EstablishConnectionTo(
            "127.0.0.1:9006");

    connection_progress.wait();
    ASSERT_NO_THROW(connection_progress.get());

    SharedServiceExecutor local_service =
        std::make_shared<DelayServiceExecutor>();

    node_2.service_registry.Borrow()->Register(local_service);
    node_2.job_manager.Borrow()->InvokeCycleAndWait();

    TryAssertUntilTimeout(
        [&node_1, &node_2]() mutable {
          node_1.job_manager.Borrow()->InvokeCycleAndWait();
          node_2.job_manager.Borrow()->InvokeCycleAndWait();
          return node_1.service_registry.Borrow()
              ->Find("delay")
              .get()
              .has_value();
        },
        10s);

    SharedServiceCaller caller =
        node_1.service_registry.Borrow()->Find("delay").get().value();

    // call remote service of node 1 asynchronously
    future_response = std::move(
        caller->IssueCallAsJob(GenerateDelayRequest(500 /*ms*/),
                               node_1.job_manager.Borrow(), false, false));
  }

  node_1.job_manager.Borrow()->InvokeCycleAndWait();

  WaitForFutureUntilTimeout(future_response, 10s);
  ASSERT_THROW(future_response.get(), std::exception);
}

TEST(ServiceTest, service_executor_busy) {
  auto config = std::make_shared<common::config::Configuration>();

  auto servicing_node = setupNode(config, 9005);
  auto calling_node = setupNode(config, 9006);

  auto connection_progress =
      servicing_node.network_endpoint.Borrow()->EstablishConnectionTo(
          "127.0.0.1:9006");

  connection_progress.wait();
  ASSERT_NO_THROW(connection_progress.get());

  // controls when the limited service is allowed to complete
  std::promise<void> completion_promise;
  std::shared_future<void> completion_future =
      completion_promise.get_future().share();

  // used to count how many calls are currently processed by the service
  auto waiting_executions_counter = std::make_shared<std::atomic_size_t>(0);

  /* service will not complete until the passed future is resolved, rendering
   the service busy for testing purposes */
  SharedServiceExecutor limited_service = std::make_shared<LimitedLocalService>(
      1, completion_future, waiting_executions_counter,
      servicing_node.job_manager);

  servicing_node.service_registry.Borrow()->Register(limited_service);
  servicing_node.job_manager.Borrow()->InvokeCycleAndWait();

  // wait until service is available for the calling node
  TryAssertUntilTimeout(
      [&calling_node]() mutable {
        calling_node.job_manager.Borrow()->InvokeCycleAndWait();
        return calling_node.service_registry.Borrow()
            ->Find("limited")
            .get()
            .has_value();
      },
      10s);

  SharedServiceCaller caller =
      calling_node.service_registry.Borrow()->Find("limited").get().value();

  // no parameters necessary
  auto request = std::make_shared<ServiceRequest>("limited");

  // first call keeps service busy (remember: a single active call is allowed)
  auto future_response_resolve =
      caller->IssueCallAsJob(request, calling_node.job_manager.Borrow(), false,
                             true, BUSY_RESPONSE_RETURN);

  // let the first call be processed. Continue when its waiting.
  TryAssertUntilTimeout(
      [&servicing_node, &calling_node, &waiting_executions_counter]() mutable {
        // these calls do not block because they are asynchronous
        calling_node.job_manager.Borrow()->InvokeCycleAndWait();
        servicing_node.job_manager.Borrow()->InvokeCycleAndWait();
        return *waiting_executions_counter == 1;
      },
      10s);

  // second call should return that the service is busy
  auto future_response_busy = caller->IssueCallAsJob(
      request->Duplicate(), calling_node.job_manager.Borrow(), false, true,
      BUSY_RESPONSE_RETURN);

  // wait until the second call has returned its busy status
  TryAssertUntilTimeout(
      [&servicing_node, &calling_node, &future_response_busy]() mutable {
        // these calls do not block because they are asynchronous
        calling_node.job_manager.Borrow()->InvokeCycleAndWait();
        servicing_node.job_manager.Borrow()->InvokeCycleAndWait();
        auto future_status = future_response_busy.wait_for(0ms);
        return future_status == std::future_status::ready;
      },
      120s);

  SharedServiceResponse expected_busy_response;
  ASSERT_NO_THROW(expected_busy_response = future_response_busy.get());
  ASSERT_EQ(expected_busy_response->GetStatus(), ServiceResponseStatus::BUSY);

  // signal the service to let calls complete
  completion_promise.set_value();

  // wait until the first call has been resolved
  TryAssertUntilTimeout(
      [&servicing_node, &calling_node, &future_response_resolve]() mutable {
        calling_node.job_manager.Borrow()->InvokeCycleAndWait();
        servicing_node.job_manager.Borrow()->InvokeCycleAndWait();
        return future_response_resolve.wait_for(0s) ==
               std::future_status::ready;
      },
      120s);

  SharedServiceResponse expected_ok_response;
  ASSERT_NO_THROW(expected_ok_response = future_response_resolve.get());
  ASSERT_EQ(expected_ok_response->GetStatus(), ServiceResponseStatus::OK);
}