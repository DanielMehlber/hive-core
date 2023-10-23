#ifndef WEBSOCKETMESSAGEREGISTRYTEST_H
#define WEBSOCKETMESSAGEREGISTRYTEST_H

#include "jobsystem/manager/JobManager.h"
#include "messaging/MessagingFactory.h"
#include "networking/NetworkingFactory.h"
#include "services/registry/impl/websockets/WebSocketServiceRegistry.h"
#include <gtest/gtest.h>

using namespace jobsystem;
using namespace services;
using namespace networking;

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
}

#endif // WEBSOCKETMESSAGEREGISTRYTEST_H
