#ifndef WEBSOCKETPEERTEST_H
#define WEBSOCKETPEERTEST_H

#include "common/test/TryAssertUntilTimeout.h"
#include "events/EventFactory.h"
#include "networking/NetworkingFactory.h"
#include "networking/peers/ConnectionInfo.h"

using namespace networking;
using namespace networking::websockets;
using namespace jobsystem;
using namespace props;
using namespace events;
using namespace std::chrono_literals;
using namespace common::test;

SharedMessageEndpoint
SetupWebSocketPeer(const common::subsystems::SharedSubsystemManager &subsystems,
                   size_t port) {
  auto config = std::make_shared<common::config::Configuration>();
  config->Set("net.port", port);
  auto event_broker = EventFactory::CreateBroker(subsystems);
  subsystems->AddOrReplaceSubsystem<IEventBroker>(event_broker);

  SharedMessageEndpoint server =
      NetworkingFactory::CreateNetworkingPeer(subsystems, config);

  subsystems->AddOrReplaceSubsystem(server);

  return server;
}

class TestConsumer : public IMessageConsumer {
public:
  mutable jobsystem::mutex counter_mutex;
  size_t counter{0};
  std::string GetMessageType() const noexcept override { return "test-type"; }
  void
  ProcessReceivedMessage(SharedMessage received_message,
                         ConnectionInfo connection_info) noexcept override {
    std::unique_lock lock(counter_mutex);
    counter++;
  }
};

void SendMessageAndWait(SharedMessage message,
                        const SharedMessageEndpoint &peer,
                        const std::string &uri) {
  std::future<void> sending_result = peer->Send(uri, message);
  sending_result.wait();
  ASSERT_NO_THROW(sending_result.get());
}

TEST(WebSockets, websockets_connection_establishment) {
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedMessageEndpoint peer1 = SetupWebSocketPeer(subsystems, 9003);
  SharedMessageEndpoint peer2 = SetupWebSocketPeer(subsystems, 9004);

  auto result = peer1->EstablishConnectionTo("ws://127.0.0.1:9004");
  result.wait();

  ASSERT_NO_THROW(result.get());
}

TEST(WebSockets, websockets_message_sending_1_to_1) {
  auto config = std::make_shared<common::config::Configuration>();
  SharedJobManager job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedMessageEndpoint peer1 = SetupWebSocketPeer(subsystems, 9003);
  SharedMessageEndpoint peer2 = SetupWebSocketPeer(subsystems, 9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();
  std::shared_ptr<TestConsumer> test_consumer_2 =
      std::make_shared<TestConsumer>();

  peer1->AddMessageConsumer(test_consumer_1);
  peer2->AddMessageConsumer(test_consumer_2);

  auto result1 = peer1->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();
  ASSERT_NO_THROW(result1.get());

  SharedMessage message = std::make_shared<Message>("test-type");

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
  auto config = std::make_shared<common::config::Configuration>();
  SharedJobManager job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedMessageEndpoint peer1 = SetupWebSocketPeer(subsystems, 9003);
  SharedMessageEndpoint peer2 = SetupWebSocketPeer(subsystems, 9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  peer2->AddMessageConsumer(test_consumer_1);

  auto result1 = peer1->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();
  ASSERT_NO_THROW(result1.get());

  for (int i = 0; i < 5; i++) {
    SharedMessage message = std::make_shared<Message>("test-type");
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
  auto config = std::make_shared<common::config::Configuration>();
  SharedJobManager job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedMessageEndpoint peer1 = SetupWebSocketPeer(subsystems, 9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  peer1->AddMessageConsumer(test_consumer_1);

  std::vector<SharedMessageEndpoint> peers;
  for (size_t i = 9005; i < 9010; i++) {
    // first copy subsystems so that this peer can register its own event broker
    std::shared_ptr<common::subsystems::SubsystemManager> peer_subsystems(
        subsystems);

    SharedMessageEndpoint peer = SetupWebSocketPeer(peer_subsystems, i);

    auto result = peer->EstablishConnectionTo("ws://127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    SharedMessage message = std::make_shared<Message>("test-type");

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
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();

  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem(job_manager);

  SharedMessageEndpoint broadcasting_peer =
      SetupWebSocketPeer(subsystems, 9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  std::vector<SharedMessageEndpoint> peers;
  for (size_t i = 9005; i < 9010; i++) {
    SharedMessageEndpoint recipient_peer = SetupWebSocketPeer(subsystems, i);

    auto result = recipient_peer->EstablishConnectionTo("127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    recipient_peer->AddMessageConsumer(test_consumer_1);
    peers.push_back(recipient_peer);
  }

  // wait until central peer has recognized connections
  TryAssertUntilTimeout(
      [&broadcasting_peer]() {
        return broadcasting_peer->GetActiveConnectionCount() == 5;
      },
      10s);

  SharedMessage message = std::make_shared<Message>("test-type");

  auto future = broadcasting_peer->Broadcast(message);
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
