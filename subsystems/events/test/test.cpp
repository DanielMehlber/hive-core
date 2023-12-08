#include "events/EventFactory.h"
#include <gtest/gtest.h>

common::subsystems::SharedSubsystemManager SetupSubsystems() {
  auto subsystems = std::make_shared<common::subsystems::SubsystemManager>();
  auto job_manager = std::make_shared<jobsystem::JobManager>();
  job_manager->StartExecution();
  subsystems->AddOrReplaceSubsystem<jobsystem::JobManager>(job_manager);
  return subsystems;
}

TEST(Messaging, receive_event) {
  auto subsystems = SetupSubsystems();

  auto broker = events::EventFactory::CreateBroker(subsystems);

  bool event_received_a = false;
  bool event_received_b = false;
  bool event_received_c = false;

  auto subscriber_a = std::make_shared<events::FunctionalEventListener>(
      [&](SharedEvent event) { event_received_a = true; });
  auto subscriber_b = std::make_shared<events::FunctionalEventListener>(
      [&](SharedEvent event) { event_received_b = true; });
  auto subscriber_c = std::make_shared<events::FunctionalEventListener>(
      [&](SharedEvent event) { event_received_c = true; });

  broker->RegisterListener(subscriber_a, "test-event");
  broker->RegisterListener(subscriber_b, "test-event");
  broker->RegisterListener(subscriber_c, "some-other-event");

  auto evt = std::make_shared<events::Event>("test-event");
  broker->FireEvent(evt);

  auto job_manager = subsystems->RequireSubsystem<jobsystem::JobManager>();
  job_manager->InvokeCycleAndWait();
  ASSERT_TRUE(event_received_a);
  ASSERT_TRUE(event_received_b);
  ASSERT_FALSE(event_received_c);
}

TEST(Messaging, subscriber_unsubscribe) {
  auto subsystems = SetupSubsystems();
  auto broker = events::EventFactory::CreateBroker(subsystems);

  std::atomic_int event_received_counter_a = 0;
  std::atomic_int event_received_counter_b = 0;

  auto subscriber_a = std::make_shared<events::FunctionalEventListener>(
      [&](SharedEvent event) { event_received_counter_a++; });
  auto subscriber_b = std::make_shared<events::FunctionalEventListener>(
      [&](SharedEvent event) { event_received_counter_b++; });

  broker->RegisterListener(subscriber_a, "test-event");
  broker->RegisterListener(subscriber_b, "test-event");

  auto evt = std::make_shared<events::Event>("test-event");
  broker->FireEvent(evt);

  auto job_manager = subsystems->RequireSubsystem<JobManager>();
  job_manager->InvokeCycleAndWait();
  ASSERT_EQ(event_received_counter_a, 1);
  ASSERT_EQ(event_received_counter_b, 1);

  broker->UnregisterListener(subscriber_a);
  broker->FireEvent(evt);
  job_manager->InvokeCycleAndWait();
  ASSERT_EQ(event_received_counter_a, 1);
  ASSERT_EQ(event_received_counter_b, 2);
}

TEST(Messaging, subscriber_destroyed) {
  auto subsystems = SetupSubsystems();
  auto broker = events::EventFactory::CreateBroker(subsystems);

  auto job_manager = subsystems->RequireSubsystem<JobManager>();

  std::atomic_int event_received_counter_a = 0;
  std::atomic_int event_received_counter_b = 0;

  auto subscriber_a = std::make_shared<events::FunctionalEventListener>(
      [&](SharedEvent event) { event_received_counter_a++; });
  broker->RegisterListener(subscriber_a, "test-event");

  auto evt = std::make_shared<events::Event>("test-event");

  {
    auto subscriber_b = std::make_shared<events::FunctionalEventListener>(
        [&](SharedEvent event) { event_received_counter_b++; });
    broker->RegisterListener(subscriber_b, "test-event");
    broker->FireEvent(evt);
    job_manager->InvokeCycleAndWait();
    ASSERT_EQ(event_received_counter_a, 1);
    ASSERT_EQ(event_received_counter_b, 1);
  }

  broker->FireEvent(evt);
  job_manager->InvokeCycleAndWait();
  ASSERT_EQ(event_received_counter_a, 2);
  ASSERT_EQ(event_received_counter_b, 1);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}