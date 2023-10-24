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

TEST(ServiceTests, web_socket_register_service) {
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

#endif // WEBSOCKETMESSAGEREGISTRYTEST_H
