#include "DataChangeListenerTest.h"
#include "data/DataLayer.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include <gtest/gtest.h>

using namespace hive::data;

TEST(PropertyTest, data_get_set) {
  auto subsystems =
      common::memory::Owner<common::subsystems::SubsystemManager>();
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = common::memory::Owner<jobsystem::JobManager>(config);
  job_manager->StartExecution();

  subsystems->AddOrReplaceSubsystem<JobManager>(std::move(job_manager));

  auto data_layer = common::memory::Owner<DataLayer>(subsystems.Borrow());

  ASSERT_FALSE(data_layer->Get("example.test.value").get().has_value());
  data_layer->Set("example.test.value", "test");
  auto optional_value = data_layer->Get("example.test.value").get();
  ASSERT_TRUE(optional_value.has_value());
  ASSERT_TRUE(optional_value.value() == "test");
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}