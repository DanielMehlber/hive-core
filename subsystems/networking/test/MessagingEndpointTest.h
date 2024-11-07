#pragma once

#include "common/test/TryAssertUntilTimeout.h"
#include "data/DataLayer.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "networking/NetworkingManager.h"
#include "networking/messaging/ConnectionInfo.h"

using namespace hive::networking;
using namespace hive::networking::messaging;
using namespace hive::jobsystem;
using namespace hive::data;
using namespace hive::events;
using namespace std::chrono_literals;
using namespace hive::common::test;

struct Node {
  std::string uuid;
  common::memory::Reference<NetworkingManager> networking_manager;
  common::memory::Owner<common::subsystems::SubsystemManager> subsystems;
  common::memory::Reference<JobManager> job_manager;
  size_t port;
};

inline Node SetupWebSocketPeer(size_t port) {
  // configure subsystems
  auto config = std::make_shared<common::config::Configuration>();
  config->Set("net.port", port);

  // setup separate subsystems for this peer (but reuse job-system)
  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();

  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);
  job_manager->StartExecution();

  auto job_manager_ref = job_manager.CreateReference();
  subsystems->AddOrReplaceSubsystem<JobManager>(std::move(job_manager));

  auto event_broker =
      common::memory::Owner<events::brokers::JobBasedEventBroker>(
          subsystems.CreateReference());
  subsystems->AddOrReplaceSubsystem<IEventBroker>(std::move(event_broker));

  auto property_provider =
      common::memory::Owner<DataLayer>(subsystems.Borrow());
  subsystems->AddOrReplaceSubsystem<DataLayer>(std::move(property_provider));

  auto networking_manager = common::memory::Owner<NetworkingManager>(
      subsystems.CreateReference(), config);

  auto networking_manager_ref = networking_manager.CreateReference();

  std::string id = subsystems->RequireSubsystem<DataLayer>()
                       ->Get("net.node.id")
                       .get()
                       .value();

  subsystems->AddOrReplaceSubsystem<NetworkingManager>(
      std::move(networking_manager));

  return Node{id, networking_manager_ref, std::move(subsystems),
              job_manager_ref, port};
}

class TestConsumer : public IMessageConsumer {
public:
  mutable jobsystem::mutex counter_mutex;
  size_t counter{0};
  std::string GetMessageType() const override { return "test-type"; }
  void ProcessReceivedMessage(SharedMessage received_message,
                              ConnectionInfo connection_info) override {
    std::unique_lock lock(counter_mutex);
    counter++;
  }
};

inline void
sendMessageToNode(const SharedMessage &message,
                  const common::memory::Borrower<IMessageEndpoint> &peer,
                  const std::string &uri) {
  std::future<void> sending_result = peer->Send(uri, message);
  sending_result.wait();
  ASSERT_NO_THROW(sending_result.get());
}

inline void waitUntilConnectionCompleted(Node &node1, Node &node2) {
  TryAssertUntilTimeout(
      [&node1, &node2]() {
        node2.job_manager.Borrow()->InvokeCycleAndWait();
        node1.job_manager.Borrow()->InvokeCycleAndWait();
        bool node1_connected =
            node1.networking_manager.Borrow()
                ->GetSomeMessageEndpointConnectedTo(node2.uuid)
                .has_value();
        bool node2_connected =
            node2.networking_manager.Borrow()
                ->GetSomeMessageEndpointConnectedTo(node1.uuid)
                .has_value();
        return node1_connected && node2_connected;
      },
      5s);
}

TEST(WebSockets, connection_establishment) {
  auto config = std::make_shared<common::config::Configuration>();

  Node node1 = SetupWebSocketPeer(9003);
  Node node2 = SetupWebSocketPeer(9004);

  auto result = node1.networking_manager.Borrow()
                    ->GetDefaultMessageEndpoint()
                    .value()
                    ->EstablishConnectionTo("ws://127.0.0.1:9004");
  result.wait();

  waitUntilConnectionCompleted(node1, node2);

  ConnectionInfo connection_info;
  ASSERT_NO_THROW(connection_info = result.get());
  ASSERT_FALSE(connection_info.hostname.empty());
  ASSERT_FALSE(connection_info.endpoint_id.empty());
}

TEST(WebSockets, message_passing_1_to_1) {
  auto config = std::make_shared<common::config::Configuration>();

  Node node1 = SetupWebSocketPeer(9003);
  Node node2 = SetupWebSocketPeer(9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();
  std::shared_ptr<TestConsumer> test_consumer_2 =
      std::make_shared<TestConsumer>();

  node1.networking_manager.Borrow()->AddMessageConsumer(test_consumer_1);
  node2.networking_manager.Borrow()->AddMessageConsumer(test_consumer_2);

  // establish connection
  auto result1 = node1.networking_manager.Borrow()
                     ->GetDefaultMessageEndpoint()
                     .value()
                     ->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();

  ASSERT_NO_THROW(result1.get());

  waitUntilConnectionCompleted(node1, node2);

  SharedMessage message = std::make_shared<Message>("test-type");

  sendMessageToNode(
      message,
      node1.networking_manager.Borrow()->GetDefaultMessageEndpoint().value(),
      node2.uuid);

  auto result2 = node2.networking_manager.Borrow()
                     ->GetDefaultMessageEndpoint()
                     .value()
                     ->EstablishConnectionTo("ws://127.0.0.1:9003");
  result2.wait();
  ASSERT_NO_THROW(result2.get());

  sendMessageToNode(
      message,
      node2.networking_manager.Borrow()->GetDefaultMessageEndpoint().value(),
      node1.uuid);

  TryAssertUntilTimeout(
      [&node1, &node2, &test_consumer_1] {
        node1.job_manager.Borrow()->InvokeCycleAndWait();
        node2.job_manager.Borrow()->InvokeCycleAndWait();
        return test_consumer_1->counter == 1;
      },
      10s);

  TryAssertUntilTimeout(
      [&node1, &node2, &test_consumer_2] {
        node1.job_manager.Borrow()->InvokeCycleAndWait();
        node2.job_manager.Borrow()->InvokeCycleAndWait();
        return test_consumer_2->counter == 1;
      },
      10s);
}

TEST(WebSockets, message_receiving_multiple) {
  auto config = std::make_shared<common::config::Configuration>();

  Node node1 = SetupWebSocketPeer(9003);
  Node node2 = SetupWebSocketPeer(9004);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  node2.networking_manager.Borrow()->AddMessageConsumer(test_consumer_1);

  auto result1 = node1.networking_manager.Borrow()
                     ->GetDefaultMessageEndpoint()
                     .value()
                     ->EstablishConnectionTo("ws://127.0.0.1:9004");
  result1.wait();
  ASSERT_NO_THROW(result1.get());

  waitUntilConnectionCompleted(node1, node2);

  for (int i = 0; i < 5; i++) {
    SharedMessage message = std::make_shared<Message>("test-type");
    sendMessageToNode(
        message,
        node1.networking_manager.Borrow()->GetDefaultMessageEndpoint().value(),
        node2.uuid);
  }

  TryAssertUntilTimeout(
      [&node1, &node2, &test_consumer_1] {
        node1.job_manager.Borrow()->InvokeCycleAndWait();
        node2.job_manager.Borrow()->InvokeCycleAndWait();
        return test_consumer_1->counter == 5;
      },
      10s);
}

TEST(WebSockets, message_sending_1_to_n) {
  auto config = std::make_shared<common::config::Configuration>();

  Node node1 = SetupWebSocketPeer(9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  node1.networking_manager.Borrow()->AddMessageConsumer(test_consumer_1);

  std::vector<Node> peers;
  for (size_t i = 9005; i < 9010; i++) {
    Node peer = SetupWebSocketPeer(i);

    auto result = peer.networking_manager.Borrow()
                      ->GetDefaultMessageEndpoint()
                      .value()
                      ->EstablishConnectionTo("ws://127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    waitUntilConnectionCompleted(node1, peer);

    SharedMessage message = std::make_shared<Message>("test-type");

    sendMessageToNode(
        message,
        peer.networking_manager.Borrow()->GetDefaultMessageEndpoint().value(),
        node1.uuid);

    peers.push_back(std::move(peer));
  }

  TryAssertUntilTimeout(
      [&peers, &node1, &test_consumer_1] {
        node1.job_manager.Borrow()->InvokeCycleAndWait();
        for (auto &peer : peers) {
          peer.job_manager.Borrow()->InvokeCycleAndWait();
        }

        return test_consumer_1->counter == 5;
      },
      10s);
}

TEST(WebSockets, message_broadcast) {
  auto config = std::make_shared<common::config::Configuration>();

  Node broadcasting_peer = SetupWebSocketPeer(9003);

  std::shared_ptr<TestConsumer> test_consumer_1 =
      std::make_shared<TestConsumer>();

  std::vector<Node> peers;
  for (size_t i = 9005; i < 9010; i++) {
    Node recipient_node = SetupWebSocketPeer(i);

    auto result = recipient_node.networking_manager.Borrow()
                      ->GetDefaultMessageEndpoint()
                      .value()
                      ->EstablishConnectionTo("127.0.0.1:9003");
    result.wait();
    ASSERT_NO_THROW(result.get());

    waitUntilConnectionCompleted(broadcasting_peer, recipient_node);

    recipient_node.networking_manager.Borrow()->AddMessageConsumer(
        test_consumer_1);
    peers.push_back(std::move(recipient_node));
  }

  // wait until central peer has recognized connections
  TryAssertUntilTimeout(
      [&broadcasting_peer]() {
        broadcasting_peer.job_manager.Borrow()->InvokeCycleAndWait();
        return broadcasting_peer.networking_manager.Borrow()
                   ->GetDefaultMessageEndpoint()
                   .value()
                   ->GetActiveConnectionCount() == 5;
      },
      10s);

  SharedMessage message = std::make_shared<Message>("test-type");

  auto future = broadcasting_peer.networking_manager.Borrow()
                    ->GetDefaultMessageEndpoint()
                    .value()
                    ->IssueBroadcastAsJob(message);
  broadcasting_peer.job_manager.Borrow()->InvokeCycleAndWait();
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
