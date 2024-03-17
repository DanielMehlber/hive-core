#include "PropertyChangeListenerTest.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "properties/PropertyProvider.h"
#include <gtest/gtest.h>

using namespace props;

TEST(PropertyTest, prop_get_set) {
  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);
  job_manager->StartExecution();

  subsystems->AddOrReplaceSubsystem<JobManager>(std::move(job_manager));

  auto broker = common::memory::Owner<events::brokers::JobBasedEventBroker>(
      subsystems.CreateReference());
  subsystems->AddOrReplaceSubsystem<events::IEventBroker>(std::move(broker));

  PropertyProvider provider(subsystems.CreateReference());

  ASSERT_FALSE(provider.Get<std::string>("example.test.value").has_value());
  provider.Set<std::string>("example.test.value", "test");
  auto optional_value = provider.Get<std::string>("example.test.value");
  ASSERT_TRUE(optional_value.has_value());
  ASSERT_TRUE(optional_value.value() == "test");
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}