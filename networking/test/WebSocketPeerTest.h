#ifndef WEBSOCKETPEERTEST_H
#define WEBSOCKETPEERTEST_H

#include "jobsystem/JobSystem.h"
#include "messaging/Messaging.h"
#include "networking/NetworkingFactory.h"
#include "properties/PropertyFactory.h"

using namespace networking;
using namespace networking::websockets;
using namespace jobsystem;
using namespace props;
using namespace messaging;

SharedWebSocketPeer SetupWebSocketPeer(SharedJobManager job_manager, int port) {
  SharedBroker message_broker = MessagingFactory::CreateBroker(job_manager);
  SharedPropertyProvider property_provider =
      PropertyFactory::CreatePropertyProvider(message_broker);
  property_provider->Set<size_t>("net.ws.port", port);
  SharedWebSocketPeer server =
      NetworkingFactory::CreateWebSocketPeer(job_manager, property_provider);

  return server;
}

TEST(NetworkingTest, websockets_connection_establishment) {
  SharedJobManager job_manager = std::make_shared<JobManager>();
  SharedWebSocketPeer peer1 = SetupWebSocketPeer(job_manager, 9003);
  SharedWebSocketPeer peer2 = SetupWebSocketPeer(job_manager, 9004);

  auto result = peer1->EstablishConnectionTo("ws://127.0.0.1:9004");
  result.wait();

  ASSERT_NO_THROW(result.get());
}

#endif /* WEBSOCKETPEERTEST_H */
