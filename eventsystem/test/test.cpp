#include "eventsystem/EventSystem.h"
#include "eventsystem/listeners/FunctionalEventListener.h"
#include "jobsystem/JobManager.h"
#include <gtest/gtest.h>

TEST(EventSystem, basic_init) {
  auto job_manager = std::make_shared<jobsystem::JobManager>();
  auto event_manager =
      std::make_shared<eventsystem::impl::JobBasedEventManager>(job_manager);

  auto listener = std::make_shared<eventsystem::FunctionalEventListener>(
      [&](const EventRef event) {});
  event_manager->AddListener(listener, "test-event");

  // eventsystem::Event event("test-event");
  // event_manager->FireEvent()
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}