#ifndef WEBSOCKETPEERTEST_H
#define WEBSOCKETPEERTEST_H

#include "common/test/TryAssertUntilTimeout.h"
#include "jobsystem/JobSystem.h"
#include "messaging/MessagingFactory.h"
#include "networking/NetworkingFactory.h"
#include "networking/peers/PeerConnectionInfo.h"

using namespace networking;
using namespace networking::websockets;
using namespace jobsystem;
using namespace props;
using namespace messaging;
using namespace std::chrono_literals;
using namespace common::test;

SharedWebSocketPeer
SetupWebSocketPeer(const common::subsystems::SharedSubsystemManager &subsystems,
                   int port) {
  SharedBroker message_broker = MessagingFactory::CreateBroker(subsystems);
  subsystems->AddOrReplaceSubsystem(message_broker);

  SharedPropertyProvider property_provider =
      std::make_shared<props::PropertyProvider>(subsystems);
  property_provider->Set<size_t>("net.ws.port", port);
  subsystems->AddOrReplaceSubsystem<props::PropertyProvider>(property_provider);

  SharedWebSocketPeer server =
      NetworkingFactory::CreateWebSocketPeer(subsystems);

  subsystems->AddOrReplaceSubsystem(server);

  return server;
}

class TestConsumer : public IPeerMessageConsumer {
public:
  std::mutex counter_mutex;
  size_t counter{0};
  std::string GetMessageType() const noexcept override { return "test-type"; }
  void
  ProcessReceivedMessage(SharedWebSocketMessage received_message,
                         PeerConnectionInfo connection_info) noexcept override {
    std::unique_lock lock(counter_mutex);
    counter++;
  }
};

void SendMessageAndWait(SharedWebSocketMessage message,
                        const SharedWebSocketPeer &peer,
                        const std::string &uri) {
  std::future<void> sending_result = peer->Send(uri, message);
  sending_result.wait();
  ASSERT_NO_THROW(sending_result.get());
}

TEST(WebSockets, websockets_connection_establishment) {
  SharedJobManager job_manager = std::make_shared<JobManager>();
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedWebSocketPeer peer1 = SetupWebSocketPeer(subsystems, 9003);
  SharedWebSocketPeer peer2 = SetupWebSocketPeer(subsystems, 9004);

  auto result = peer1->EstablishConnectionTo("ws://127.0.0.1:9004");
  result.wait();

  ASSERT_NO_THROW(result.get());
}

TEST(WebSockets, websockets_connection_closing) {
  SharedJobManager job_manager = std::make_shared<JobManager>();
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedWebSocketPeer peer1 = SetupWebSocketPeer(subsystems, 9003);
  SharedWebSocketMessage message = std::make_shared<PeerMessage>("test-type");
  {
    auto subsystems_copy =
        std::make_shared<common::subsystems::SubsystemManager>(*subsystems);
    SharedWebSocketPeer peer2 = SetupWebSocketPeer(subsystems_copy, 9004);
    auto result = peer1->EstablishConnectionTo("ws://127.0.0.1:9004");
    result.wait();

    SendMessageAndWait(message, peer1, "ws://127.0.0.1:9004");
  }

  ASSERT_THROW(peer1->Send("ws://127.0.0.1:9004", message),
               NoSuchPeerException);
}

TEST(WebSockets, websockets_message_sending_1_to_1) {
  SharedJobManager job_manager = std::make_shared<JobManager>();
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedWebSocketPeer peer1 = SetupWebSocketPeer(subsystems, 9003);
  SharedWebSocketPeer peer2 = SetupWebSocketPeer(subsystems, 9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();
  std::shared_ptr<TestConsumer> test_consumer_2 =
      std::make_shared<TestConsumer>();

  peer1->AddConsumer(test_consumer_1);
  peer2->AddConsumer(test_consumer_2);

  auto result1 = peer1->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();
  ASSERT_NO_THROW(result1.get());

  SharedWebSocketMessage message = std::make_shared<PeerMessage>("test-type");

  SendMessageAndWait(message, peer1, "ws://127.0.0.1:9004");

  auto result2 = peer2->EstablishConnectionTo("ws://127.0.0.1:9003");
  result2.wait();
  ASSERT_NO_THROW(result2.get());

  SendMessageAndWait(message, peer2, "ws://127.0.0.1:9003");

  job_manager->InvokeCycleAndWait();
  TryAssertUntilTimeout(
      [&] {
        job_manager->InvokeCycleAndWait();
        return test_consumer_1->counter == 1;
      },
      10s);
  TryAssertUntilTimeout(
      [&] {
        job_manager->InvokeCycleAndWait();
        return test_consumer_2->counter == 1;
      },
      10s);
}

TEST(WebSockets, websockets_message_receiving_multiple) {
  SharedJobManager job_manager = std::make_shared<JobManager>();
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedWebSocketPeer peer1 = SetupWebSocketPeer(subsystems, 9003);
  SharedWebSocketPeer peer2 = SetupWebSocketPeer(subsystems, 9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  peer2->AddConsumer(test_consumer_1);

  auto result1 = peer1->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();
  ASSERT_NO_THROW(result1.get());

  for (int i = 0; i < 5; i++) {
    SharedWebSocketMessage message = std::make_shared<PeerMessage>("test-type");
    SendMessageAndWait(message, peer1, "ws://127.0.0.1:9004");
  }

  job_manager->InvokeCycleAndWait();
  TryAssertUntilTimeout(
      [&] {
        job_manager->InvokeCycleAndWait();
        return test_consumer_1->counter == 5;
      },
      10s);
}

TEST(WebSockets, websockets_message_sending_1_to_n) {
  SharedJobManager job_manager = std::make_shared<JobManager>();
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedWebSocketPeer peer1 = SetupWebSocketPeer(subsystems, 9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  peer1->AddConsumer(test_consumer_1);

  std::vector<SharedWebSocketPeer> peers;
  for (size_t i = 9005; i < 9010; i++) {
    SharedWebSocketPeer peer = SetupWebSocketPeer(subsystems, i);

    auto result = peer->EstablishConnectionTo("ws://127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    SharedWebSocketMessage message = std::make_shared<PeerMessage>("test-type");

    SendMessageAndWait(message, peer, "ws://127.0.0.1:9003");

    peers.push_back(peer);
  }

  job_manager->InvokeCycleAndWait();
  TryAssertUntilTimeout(
      [&] {
        job_manager->InvokeCycleAndWait();
        return test_consumer_1->counter == 5;
      },
      10s);
}

TEST(WebSockets, websockets_message_broadcast) {
  SharedJobManager job_manager = std::make_shared<JobManager>();
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedWebSocketPeer peer1 = SetupWebSocketPeer(subsystems, 9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  std::vector<SharedWebSocketPeer> peers;
  for (size_t i = 9005; i < 9010; i++) {
    SharedWebSocketPeer peer = SetupWebSocketPeer(subsystems, i);

    auto result = peer->EstablishConnectionTo("ws://127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    peer->AddConsumer(test_consumer_1);
    peers.push_back(peer);
  }

  SharedWebSocketMessage message = std::make_shared<PeerMessage>("test-type");

  auto future = peer1->Broadcast(message);
  job_manager->InvokeCycleAndWait();

  future.wait();
  ASSERT_EQ(future.get(), 5);

  TryAssertUntilTimeout(
      [&] {
        job_manager->InvokeCycleAndWait();
        return test_consumer_1->counter == 5;
      },
      10s);
}

#endif /* WEBSOCKETPEERTEST_H */
