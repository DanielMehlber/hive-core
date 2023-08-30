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

TEST(JobSystem, auto_requeue) {
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

TEST(JobSystem, timer_job) {
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

TEST(JobSystem, jobs_kicking_jobs) {
  JobManager manager;
  bool jobACompleted = false;
  bool jobBCompleted = false;
  bool jobCCompleted = false;
  bool jobDCompleted = false;
  auto jobA = std::make_shared<Job>([&](JobContext *context) {
    // job inside job (should execute directly)
    auto jobB = std::make_shared<Job>([&](JobContext *context) {
      // job inside job inside job (should also execute directly)
      auto jobC = std::make_shared<Job>([&](JobContext *context) {
        // the phase of this job has passed, so it should not be executed
        auto jobD = std::make_shared<Job>(
            [&](JobContext *context) {
              jobDCompleted = true;
              return JobContinuation::DISPOSE;
            },
            INIT);
        context->GetJobManager()->KickJob(jobD);
        jobCCompleted = true;
        return JobContinuation::DISPOSE;
      });
      context->GetJobManager()->KickJob(jobC);
      jobBCompleted = true;
      return JobContinuation::DISPOSE;
    });
    std::shared_ptr<JobCounter> counter = std::make_shared<JobCounter>();
    jobB->AddCounter(counter);
    context->GetJobManager()->KickJob(jobB);
    jobACompleted = true;
    return JobContinuation::DISPOSE;
  });

  manager.KickJob(jobA);
  manager.InvokeCycleAndWait();

  ASSERT_TRUE(jobACompleted);
  ASSERT_TRUE(jobBCompleted);
  ASSERT_TRUE(jobCCompleted);
  ASSERT_FALSE(jobDCompleted);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}