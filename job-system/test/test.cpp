#include "jobsystem/JobManager.h"
#include <gtest/gtest.h>

using namespace jobsystem;

TEST(JobSystem, cycleTest) {
  JobManager manager;
  bool jobACompleted = false;
  std::shared_ptr<Job> jobA = std::make_shared<Job>([&]() {
    jobACompleted = true;
    return JobContinuation::DISPOSE;
  });

  manager.KickJob(jobA);
  manager.InvokeCycleAndWait();
  ASSERT_TRUE(jobACompleted);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}