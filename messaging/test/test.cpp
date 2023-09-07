#include "messaging/Messaging.h"
#include <gtest/gtest.h>
#include <jobsystem/manager/JobManager.h>

TEST(Messaging, receive_message) {
  auto job_manager = std::make_shared<jobsystem::JobManager>();
  auto broker = messaging::MessagingFactory::CreateBroker(job_manager);

  bool message_received_a = false;
  bool message_received_b = false;
  bool message_received_c = false;

  auto subscriber_a = messaging::MessagingFactory::CreateSubscriber(
      [&](const SharedMessage event) { message_received_a = true; });
  auto subscriber_b = messaging::MessagingFactory::CreateSubscriber(
      [&](const SharedMessage event) { message_received_b = true; });
  auto subscriber_c = messaging::MessagingFactory::CreateSubscriber(
      [&](const SharedMessage event) { message_received_c = true; });

  broker->AddSubscriber(subscriber_a, "test-event");
  broker->AddSubscriber(subscriber_b, "test-event");
  broker->AddSubscriber(subscriber_c, "some-other-event");

  auto msg = std::make_shared<messaging::Message>("test-event");
  broker->PublishMessage(msg);

  job_manager->InvokeCycleAndWait();
  ASSERT_TRUE(message_received_a);
  ASSERT_TRUE(message_received_b);
  ASSERT_FALSE(message_received_c);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}