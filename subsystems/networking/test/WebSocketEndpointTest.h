#ifndef WEBSOCKETPEERTEST_H
#define WEBSOCKETPEERTEST_H

#include "common/test/TryAssertUntilTimeout.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "networking/NetworkingManager.h"
#include "networking/messaging/ConnectionInfo.h"

using namespace networking;
using namespace networking::messaging;
using namespace jobsystem;
using namespace props;
using namespace events;
using namespace std::chrono_literals;
using namespace common::test;

struct Node {
  common::memory::Reference<IMessageEndpoint> endpoint;
  common::memory::Owner<common::subsystems::SubsystemManager> subsystems;
  common::memory::Reference<jobsystem::JobManager> job_manager;
  size_t port;
};

Node SetupWebSocketPeer(size_t port) {
  // configure subsystems
  auto config = std::make_shared<common::config::Configuration>();
  config->Set("net.port", port);

  // setup separate subsystems for this peer (but reuse job-system)
  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();

  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);
  auto job_manager_ref = job_manager.CreateReference();
  subsystems->AddOrReplaceSubsystem<JobManager>(std::move(job_manager));

  auto event_broker =
      common::memory::Owner<events::brokers::JobBasedEventBroker>(
          subsystems.CreateReference());
  subsystems->AddOrReplaceSubsystem<IEventBroker>(std::move(event_broker));

  auto networking_manager =
      common::memory::Owner<networking::NetworkingManager>(
          subsystems.CreateReference(), config);

  subsystems->AddOrReplaceSubsystem<networking::NetworkingManager>(
      std::move(networking_manager));

  auto endpoint = subsystems->RequireSubsystem<IMessageEndpoint>();

  return Node{endpoint.ToReference(), std::move(subsystems), job_manager_ref,
              port};
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
                        common::memory::Borrower<IMessageEndpoint> peer,
                        const std::string &uri) {
  std::future<void> sending_result = peer->Send(uri, message);
  sending_result.wait();
  ASSERT_NO_THROW(sending_result.get());
}

TEST(WebSockets, websockets_connection_establishment) {
  auto config = std::make_shared<common::config::Configuration>();

  Node peer1 = SetupWebSocketPeer(9003);
  Node peer2 = SetupWebSocketPeer(9004);

  auto result =
      peer1.endpoint.Borrow()->EstablishConnectionTo("ws://127.0.0.1:9004");
  result.wait();

  ASSERT_NO_THROW(result.get());
}

TEST(WebSockets, websockets_message_sending_1_to_1) {
  auto config = std::make_shared<common::config::Configuration>();

  Node peer1 = SetupWebSocketPeer(9003);
  Node peer2 = SetupWebSocketPeer(9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();
  std::shared_ptr<TestConsumer> test_consumer_2 =
      std::make_shared<TestConsumer>();

  peer1.endpoint.Borrow()->AddMessageConsumer(test_consumer_1);
  peer2.endpoint.Borrow()->AddMessageConsumer(test_consumer_2);

  // establish connection
  auto result1 =
      peer1.endpoint.Borrow()->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();
  ASSERT_NO_THROW(result1.get());

  // process established connections
  peer1.job_manager.Borrow()->InvokeCycleAndWait();
  peer2.job_manager.Borrow()->InvokeCycleAndWait();

  SharedMessage message = std::make_shared<Message>("test-type");

  SendMessageAndWait(message, peer1.endpoint.Borrow(), "ws://127.0.0.1:9004");

  auto result2 =
      peer2.endpoint.Borrow()->EstablishConnectionTo("ws://127.0.0.1:9003");
  result2.wait();
  ASSERT_NO_THROW(result2.get());

  SendMessageAndWait(message, peer2.endpoint.Borrow(), "ws://127.0.0.1:9003");

  TryAssertUntilTimeout(
      [&peer1, &peer2, &test_consumer_1] {
        peer1.job_manager.Borrow()->InvokeCycleAndWait();
        peer2.job_manager.Borrow()->InvokeCycleAndWait();
        return test_consumer_1->counter == 1;
      },
      10s);

  TryAssertUntilTimeout(
      [&peer1, &peer2, &test_consumer_2] {
        peer1.job_manager.Borrow()->InvokeCycleAndWait();
        peer2.job_manager.Borrow()->InvokeCycleAndWait();
        return test_consumer_2->counter == 1;
      },
      10s);
}

TEST(WebSockets, websockets_message_receiving_multiple) {
  auto config = std::make_shared<common::config::Configuration>();

  Node peer1 = SetupWebSocketPeer(9003);
  Node peer2 = SetupWebSocketPeer(9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  peer2.endpoint.Borrow()->AddMessageConsumer(test_consumer_1);

  auto result1 =
      peer1.endpoint.Borrow()->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();
  ASSERT_NO_THROW(result1.get());

  // process established connections
  peer1.job_manager.Borrow()->InvokeCycleAndWait();
  peer2.job_manager.Borrow()->InvokeCycleAndWait();

  for (int i = 0; i < 5; i++) {
    SharedMessage message = std::make_shared<Message>("test-type");
    SendMessageAndWait(message, peer1.endpoint.Borrow(), "ws://127.0.0.1:9004");
  }

  TryAssertUntilTimeout(
      [&peer1, &peer2, &test_consumer_1] {
        peer1.job_manager.Borrow()->InvokeCycleAndWait();
        peer2.job_manager.Borrow()->InvokeCycleAndWait();
        return test_consumer_1->counter == 5;
      },
      10s);
}

TEST(WebSockets, websockets_message_sending_1_to_n) {
  auto config = std::make_shared<common::config::Configuration>();

  Node peer1 = SetupWebSocketPeer(9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  peer1.endpoint.Borrow()->AddMessageConsumer(test_consumer_1);

  std::vector<Node> peers;
  for (size_t i = 9005; i < 9010; i++) {
    Node peer = SetupWebSocketPeer(i);

    auto result =
        peer.endpoint.Borrow()->EstablishConnectionTo("ws://127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    // process established connections
    peer.job_manager.Borrow()->InvokeCycleAndWait();
    peer1.job_manager.Borrow()->InvokeCycleAndWait();

    SharedMessage message = std::make_shared<Message>("test-type");

    SendMessageAndWait(message, peer.endpoint.Borrow(), "ws://127.0.0.1:9003");

    peers.push_back(std::move(peer));
  }

  TryAssertUntilTimeout(
      [&peers, &peer1, &test_consumer_1] {
        peer1.job_manager.Borrow()->InvokeCycleAndWait();
        for (auto &peer : peers) {
          peer.job_manager.Borrow()->InvokeCycleAndWait();
        }

        return test_consumer_1->counter == 5;
      },
      10s);
}

TEST(WebSockets, websockets_message_broadcast) {
  auto config = std::make_shared<common::config::Configuration>();

  Node broadcasting_peer = SetupWebSocketPeer(9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  std::vector<Node> peers;
  for (size_t i = 9005; i < 9010; i++) {
    Node recipient_peer = SetupWebSocketPeer(i);

    auto result = recipient_peer.endpoint.Borrow()->EstablishConnectionTo(
        "127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    // process established connections
    recipient_peer.job_manager.Borrow()->InvokeCycleAndWait();
    broadcasting_peer.job_manager.Borrow()->InvokeCycleAndWait();

    recipient_peer.endpoint.Borrow()->AddMessageConsumer(test_consumer_1);
    peers.push_back(std::move(recipient_peer));
  }

  // wait until central peer has recognized connections
  TryAssertUntilTimeout(
      [&broadcasting_peer, &peers]() {
        broadcasting_peer.job_manager.Borrow()->InvokeCycleAndWait();
        return broadcasting_peer.endpoint.Borrow()
                   ->GetActiveConnectionCount() == 5;
      },
      10s);

  SharedMessage message = std::make_shared<Message>("test-type");

  auto future = broadcasting_peer.endpoint.Borrow()->Broadcast(message);
  future.wait();
  ASSERT_EQ(future.get(), 5);

  TryAssertUntilTimeout(
      [&broadcasting_peer, &peers, &test_consumer_1] {
        broadcasting_peer.job_manager.Borrow()->InvokeCycleAndWait();
        for (auto &peer : peers) {
          peer.job_manager.Borrow()->InvokeCycleAndWait();
        }
        return test_consumer_1->counter == 5;
      },
      10s);
}

#endif /* WEBSOCKETPEERTEST_H */
