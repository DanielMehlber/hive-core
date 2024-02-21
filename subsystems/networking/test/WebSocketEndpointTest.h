#ifndef WEBSOCKETPEERTEST_H
#define WEBSOCKETPEERTEST_H

#include "common/test/TryAssertUntilTimeout.h"
#include "events/EventFactory.h"
#include "networking/NetworkingFactory.h"
#include "networking/messaging/ConnectionInfo.h"

using namespace networking;
using namespace networking::websockets;
using namespace jobsystem;
using namespace props;
using namespace events;
using namespace std::chrono_literals;
using namespace common::test;

struct Node {
  SharedMessageEndpoint endpoint;
  common::subsystems::SharedSubsystemManager subsystems;
  size_t port;
};

Node SetupWebSocketPeer(const std::shared_ptr<JobManager> &job_manager,
                        size_t port) {
  // configure subsystems
  auto config = std::make_shared<common::config::Configuration>();
  config->Set("net.port", port);

  // setup separate subsystems for this peer (but reuse job-system)
  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  subsystems->AddOrReplaceSubsystem<JobManager>(job_manager);
  auto event_broker = EventFactory::CreateBroker(subsystems);
  subsystems->AddOrReplaceSubsystem<IEventBroker>(event_broker);

  SharedMessageEndpoint server =
      NetworkingFactory::CreateNetworkingPeer(subsystems, config);

  subsystems->AddOrReplaceSubsystem(server);

  Node node{server, subsystems, port};

  return node;
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

  Node peer1 = SetupWebSocketPeer(job_manager, 9003);
  Node peer2 = SetupWebSocketPeer(job_manager, 9004);

  auto result = peer1.endpoint->EstablishConnectionTo("ws://127.0.0.1:9004");
  result.wait();

  ASSERT_NO_THROW(result.get());
}

TEST(WebSockets, websockets_message_sending_1_to_1) {
  auto config = std::make_shared<common::config::Configuration>();
  SharedJobManager job_manager = std::make_shared<JobManager>(config);
  job_manager->StartExecution();

  Node peer1 = SetupWebSocketPeer(job_manager, 9003);
  Node peer2 = SetupWebSocketPeer(job_manager, 9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();
  std::shared_ptr<TestConsumer> test_consumer_2 =
      std::make_shared<TestConsumer>();

  peer1.endpoint->AddMessageConsumer(test_consumer_1);
  peer2.endpoint->AddMessageConsumer(test_consumer_2);

  auto result1 = peer1.endpoint->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();
  ASSERT_NO_THROW(result1.get());

  SharedMessage message = std::make_shared<Message>("test-type");

  SendMessageAndWait(message, peer1.endpoint, "ws://127.0.0.1:9004");

  auto result2 = peer2.endpoint->EstablishConnectionTo("ws://127.0.0.1:9003");
  result2.wait();
  ASSERT_NO_THROW(result2.get());

  SendMessageAndWait(message, peer2.endpoint, "ws://127.0.0.1:9003");

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

  Node peer1 = SetupWebSocketPeer(job_manager, 9003);
  Node peer2 = SetupWebSocketPeer(job_manager, 9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  peer2.endpoint->AddMessageConsumer(test_consumer_1);

  auto result1 = peer1.endpoint->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();
  ASSERT_NO_THROW(result1.get());

  for (int i = 0; i < 5; i++) {
    SharedMessage message = std::make_shared<Message>("test-type");
    SendMessageAndWait(message, peer1.endpoint, "ws://127.0.0.1:9004");
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

  Node peer1 = SetupWebSocketPeer(job_manager, 9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  peer1.endpoint->AddMessageConsumer(test_consumer_1);

  std::vector<Node> peers;
  for (size_t i = 9005; i < 9010; i++) {
    // first copy subsystems so that this peer can register its own event broker
    std::shared_ptr<common::subsystems::SubsystemManager> peer_subsystems(
        subsystems);

    Node peer = SetupWebSocketPeer(job_manager, i);

    auto result = peer.endpoint->EstablishConnectionTo("ws://127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    SharedMessage message = std::make_shared<Message>("test-type");

    SendMessageAndWait(message, peer.endpoint, "ws://127.0.0.1:9003");

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

  Node broadcasting_peer = SetupWebSocketPeer(job_manager, 9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  std::vector<Node> peers;
  for (size_t i = 9005; i < 9010; i++) {
    Node recipient_peer = SetupWebSocketPeer(job_manager, i);

    auto result =
        recipient_peer.endpoint->EstablishConnectionTo("127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    recipient_peer.endpoint->AddMessageConsumer(test_consumer_1);
    peers.push_back(recipient_peer);
  }

  // wait until central peer has recognized connections
  TryAssertUntilTimeout(
      [&broadcasting_peer]() {
        return broadcasting_peer.endpoint->GetActiveConnectionCount() == 5;
      },
      10s);

  SharedMessage message = std::make_shared<Message>("test-type");

  auto future = broadcasting_peer.endpoint->Broadcast(message);
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
