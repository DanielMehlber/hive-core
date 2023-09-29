#include "PropertyChangeListenerTest.h"
#include "jobsystem/JobSystem.h"
#include "properties/PropertyProvider.h"
#include <gtest/gtest.h>

using namespace props;

TEST(PropertyTest, prop_get_set) {
  jobsystem::SharedJobManager job_manager =
      std::make_shared<jobsystem::JobManager>();
  messaging::SharedBroker broker =
      messaging::MessagingFactory::CreateBroker(job_manager);
  PropertyProvider provider(broker);

  ASSERT_FALSE(provider.Get<std::string>("example.test.value").has_value());
  provider.Set<std::string>("example.test.value", "test");
  auto optional_value = provider.Get<std::string>("example.test.value");
  ASSERT_TRUE(optional_value.has_value());
  ASSERT_TRUE(optional_value.value().compare("test") == 0);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}