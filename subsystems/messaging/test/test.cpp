#include "messaging/MessagingFactory.h"
#include <gtest/gtest.h>

common::subsystems::SharedSubsystemManager SetupSubsystems() {
  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  auto job_manager = std::make_shared<jobsystem::JobManager>();
  job_manager->StartExecution();
  subsystems->AddOrReplaceSubsystem<jobsystem::JobManager>(job_manager);
  return subsystems;
}

TEST(Messaging, receive_message) {
  auto subsystems = SetupSubsystems();

  auto broker = messaging::MessagingFactory::CreateBroker(subsystems);

  bool message_received_a = false;
  bool message_received_b = false;
  bool message_received_c = false;

  auto subscriber_a = messaging::MessagingFactory::CreateSubscriber(
      [&](SharedMessage event) { message_received_a = true; });
  auto subscriber_b = messaging::MessagingFactory::CreateSubscriber(
      [&](SharedMessage event) { message_received_b = true; });
  auto subscriber_c = messaging::MessagingFactory::CreateSubscriber(
      [&](SharedMessage event) { message_received_c = true; });

  broker->AddSubscriber(subscriber_a, "test-event");
  broker->AddSubscriber(subscriber_b, "test-event");
  broker->AddSubscriber(subscriber_c, "some-other-event");

  auto msg = std::make_shared<messaging::Message>("test-event");
  broker->PublishMessage(msg);

  auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();
  job_manager->InvokeCycleAndWait();
  ASSERT_TRUE(message_received_a);
  ASSERT_TRUE(message_received_b);
  ASSERT_FALSE(message_received_c);
}

TEST(Messaging, subscriber_unsubscribe) {
  auto subsystems = SetupSubsystems();
  auto broker = messaging::MessagingFactory::CreateBroker(subsystems);

  std::atomic_int message_received_counter_a = 0;
  std::atomic_int message_received_counter_b = 0;

  auto subscriber_a = messaging::MessagingFactory::CreateSubscriber(
      [&](SharedMessage event) { message_received_counter_a++; });
  auto subscriber_b = messaging::MessagingFactory::CreateSubscriber(
      [&](SharedMessage event) { message_received_counter_b++; });

  broker->AddSubscriber(subscriber_a, "test-event");
  broker->AddSubscriber(subscriber_b, "test-event");

  auto msg = messaging::MessagingFactory::CreateMessage("test-event");
  broker->PublishMessage(msg);

  auto job_manager = subsystems->RequireSubsystem<JobManager>();
  job_manager->InvokeCycleAndWait();
  ASSERT_EQ(message_received_counter_a, 1);
  ASSERT_EQ(message_received_counter_b, 1);

  broker->RemoveSubscriber(subscriber_a);
  broker->PublishMessage(msg);
  job_manager->InvokeCycleAndWait();
  ASSERT_EQ(message_received_counter_a, 1);
  ASSERT_EQ(message_received_counter_b, 2);
}

TEST(Messaging, subscriber_destroyed) {
  auto subsystems = SetupSubsystems();
  auto broker = messaging::MessagingFactory::CreateBroker(subsystems);

  auto job_manager = subsystems->RequireSubsystem<JobManager>();

  std::atomic_int message_received_counter_a = 0;
  std::atomic_int message_received_counter_b = 0;

  auto subscriber_a = messaging::MessagingFactory::CreateSubscriber(
      [&](SharedMessage event) { message_received_counter_a++; });
  broker->AddSubscriber(subscriber_a, "test-event");

  auto msg = messaging::MessagingFactory::CreateMessage("test-event");

  {
    auto subscriber_b = messaging::MessagingFactory::CreateSubscriber(
        [&](SharedMessage event) { message_received_counter_b++; });
    broker->AddSubscriber(subscriber_b, "test-event");
    broker->PublishMessage(msg);
    job_manager->InvokeCycleAndWait();
    ASSERT_EQ(message_received_counter_a, 1);
    ASSERT_EQ(message_received_counter_b, 1);
  }

  broker->PublishMessage(msg);
  job_manager->InvokeCycleAndWait();
  ASSERT_EQ(message_received_counter_a, 2);
  ASSERT_EQ(message_received_counter_b, 1);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}