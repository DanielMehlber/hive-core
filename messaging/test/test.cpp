#include "messaging/Messaging.h"
#include <gtest/gtest.h>
#include <jobsystem/JobManager.h>

TEST(Messaging, receive_message) {
  auto job_manager = std::make_shared<jobsystem::JobManager>();
  auto messaging_manager =
      std::make_shared<messaging::impl::JobBasedMessagingManager>(job_manager);

  bool message_received_a = false;
  bool message_received_b = false;
  bool message_received_c = false;

  auto subscriber_a = std::make_shared<messaging::FunctionalMessageSubscriber>(
      [&](const SharedMessage event) { message_received_a = true; });
  auto subscriber_b = std::make_shared<messaging::FunctionalMessageSubscriber>(
      [&](const SharedMessage event) { message_received_b = true; });
  auto subscriber_c = std::make_shared<messaging::FunctionalMessageSubscriber>(
      [&](const SharedMessage event) { message_received_c = true; });

  messaging_manager->AddSubscriber(subscriber_a, "test-event");
  messaging_manager->AddSubscriber(subscriber_b, "test-event");
  messaging_manager->AddSubscriber(subscriber_c, "some-other-event");

  auto msg = std::make_shared<messaging::Message>("test-event");
  messaging_manager->PublishMessage(msg);

  job_manager->InvokeCycleAndWait();
  ASSERT_TRUE(message_received_a);
  ASSERT_TRUE(message_received_b);
  ASSERT_FALSE(message_received_c);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}