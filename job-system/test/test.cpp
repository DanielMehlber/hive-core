#include "jobsystem/JobManager.h"
#include <gtest/gtest.h>

using namespace jobsystem;
using namespace std::chrono_literals;

TEST(JobSystem, allPhases) {
  JobManager manager;

  std::vector<short> vec;
  std::shared_ptr<Job> jobA = std::make_shared<Job>(
      [&](JobContext *context) {
        vec.push_back(0);
        return JobContinuation::DISPOSE;
      },
      JobExecutionPhase::INIT);
  std::shared_ptr<Job> jobB = std::make_shared<Job>(
      [&](JobContext *context) {
        vec.push_back(1);
        return JobContinuation::DISPOSE;
      },
      JobExecutionPhase::MAIN);
  std::shared_ptr<Job> jobC = std::make_shared<Job>(
      [&](JobContext *context) {
        vec.push_back(2);
        return JobContinuation::DISPOSE;
      },
      JobExecutionPhase::CLEAN_UP);

  manager.KickJob(jobA);
  manager.KickJob(jobC);
  manager.KickJob(jobB);

  manager.InvokeCycleAndWait();
  ASSERT_EQ(0, vec.at(0));
  ASSERT_EQ(1, vec.at(1));
  ASSERT_EQ(2, vec.at(2));
}

TEST(JobSystem, autoReschedule) {
  JobManager manager;
  int executions = 0;

  auto job = std::make_shared<Job>([&](JobContext *) {
    executions++;
    return JobContinuation::REQUEUE;
  });

  manager.KickJob(job);
  manager.InvokeCycleAndWait();
  manager.InvokeCycleAndWait();

  ASSERT_EQ(executions, 2);
}

TEST(JobSystem, timerJobs) {
  JobManager manager;
  bool job_executed = false;
  auto timer_job = std::make_shared<TimerJob>(
      [&](JobContext *) {
        job_executed = true;
        return JobContinuation::REQUEUE;
      },
      1s);

  manager.KickJob(timer_job);
  manager.InvokeCycleAndWait();
  ASSERT_FALSE(job_executed);

  std::this_thread::sleep_for(1s);
  manager.InvokeCycleAndWait();
  ASSERT_TRUE(job_executed);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}