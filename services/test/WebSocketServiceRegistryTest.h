#ifndef WEBSOCKETMESSAGEREGISTRYTEST_H
#define WEBSOCKETMESSAGEREGISTRYTEST_H

#include "AddingServiceStub.h"
#include "common/test/TryAssertUntilTimeout.h"
#include "jobsystem/manager/JobManager.h"
#include "messaging/MessagingFactory.h"
#include "networking/NetworkingFactory.h"
#include "services/registry/impl/websockets/WebSocketServiceRegistry.h"
#include "services/stub/impl/LocalServiceStub.h"
#include <gtest/gtest.h>

using namespace jobsystem;
using namespace services;
using namespace services::impl;
using namespace networking;
using namespace common::test;

SharedWebSocketPeer setupPeer(size_t port, const SharedJobManager &job_manager,
                              const SharedBroker &message_broker) {
  props::SharedPropertyProvider property_provider =
      std::make_shared<props::PropertyProvider>(message_broker);

  property_provider->Set("net.ws.port", port);

  return NetworkingFactory::CreateWebSocketPeer(job_manager, property_provider);
}

TEST(ServiceTests, web_socket_run_single_remote_service) {
  SharedJobManager job_manager = std::make_shared<JobManager>();
  messaging::SharedBroker message_broker =
      messaging::MessagingFactory::CreateBroker(job_manager);

  // setup first peer
  SharedWebSocketPeer web_socket_peer_a =
      setupPeer(9005, job_manager, message_broker);
  SharedServiceRegistry registry_a =
      std::make_shared<services::impl::WebSocketServiceRegistry>(
          web_socket_peer_a, job_manager);

  // setup second peer
  SharedWebSocketPeer web_socket_peer_b =
      setupPeer(9006, job_manager, message_broker);
  SharedServiceRegistry registry_b =
      std::make_shared<services::impl::WebSocketServiceRegistry>(
          web_socket_peer_b, job_manager);

  // first establish connection in order to broadcast the connection
  auto connection_progress =
      web_socket_peer_a->EstablishConnectionTo("127.0.0.1:9006");

  connection_progress.wait();
  ASSERT_NO_THROW(connection_progress.get());

  SharedServiceStub local_service = std::make_shared<AddingServiceStub>();
  registry_a->Register(local_service);
  job_manager->InvokeCycleAndWait();

  TryAssertUntilTimeout(
      [registry_b, job_manager] {
        job_manager->InvokeCycleAndWait();
        return registry_b->Find("add").get().has_value();
      },
      10s);

  SharedServiceCaller caller = registry_b->Find("add").get().value();
  auto result_fut = caller->Call(GenerateAddingRequest(3, 5), job_manager);
  job_manager->InvokeCycleAndWait();

  SharedServiceResponse response;
  ASSERT_NO_THROW(response = result_fut.get());
  ASSERT_EQ(8, GetResultOfAddition(response));
}

TEST(ServiceTests, web_service_load_balancing) {
  SharedJobManager job_manager = std::make_shared<JobManager>();
  messaging::SharedBroker message_broker =
      messaging::MessagingFactory::CreateBroker(job_manager);

  // setup first peer
  SharedWebSocketPeer main_web_socket_peer =
      setupPeer(9004, job_manager, message_broker);
  SharedServiceRegistry main_registry =
      std::make_shared<services::impl::WebSocketServiceRegistry>(
          main_web_socket_peer, job_manager);

  std::vector<std::tuple<SharedWebSocketPeer, SharedServiceRegistry,
                         std::shared_ptr<AddingServiceStub>>>
      peers;
  for (int i = 9005; i < 9010; i++) {
    // setup first peer
    SharedWebSocketPeer web_socket_peer =
        setupPeer(i, job_manager, message_broker);
    SharedServiceRegistry registry =
        std::make_shared<services::impl::WebSocketServiceRegistry>(
            web_socket_peer, job_manager);

    // first establish connection in order to broadcast the connection
    auto connection_progress =
        web_socket_peer->EstablishConnectionTo("127.0.0.1:9004");

    connection_progress.wait();
    ASSERT_NO_THROW(connection_progress.get());

    std::shared_ptr<AddingServiceStub> service =
        std::make_shared<AddingServiceStub>();

    peers.emplace_back(web_socket_peer, registry, service);
  }

  // register service at peers
  for (auto &peer : peers) {
    std::get<1>(peer)->Register(std::get<2>(peer));
  }

  job_manager->InvokeCycleAndWait();

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

#endif // WEBSOCKETMESSAGEREGISTRYTEST_H
