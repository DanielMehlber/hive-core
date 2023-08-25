#include <gtest/gtest.h>
#include <jobsystem/JobSystem.h>
#include <logging/LoggingApi.h>

TEST(JobSystem, checkInit) { jobsystem::JobSystem system; }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}