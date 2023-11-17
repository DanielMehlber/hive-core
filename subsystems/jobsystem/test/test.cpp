#include "jobsystem/JobSystem.h"
#include "jobsystem/JobSystemFactory.h"
#include "jobsystem/manager/JobManager.h"
#include <gtest/gtest.h>

using namespace jobsystem;
using namespace std::chrono_literals;

TEST(JobSystem, allPhases) {
  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();

  std::vector<short> vec;
  SharedJob jobA = JobSystemFactory::CreateJob(
      [&](JobContext *context) {
        vec.push_back(0);
        return JobContinuation::DISPOSE;
      },
      JobExecutionPhase::INIT);
  SharedJob jobB = JobSystemFactory::CreateJob(
      [&](JobContext *context) {
        vec.push_back(1);
        return JobContinuation::DISPOSE;
      },
      JobExecutionPhase::MAIN);
  SharedJob jobC = JobSystemFactory::CreateJob(
      [&](JobContext *context) {
        vec.push_back(2);
        return JobContinuation::DISPOSE;
      },
      JobExecutionPhase::CLEAN_UP);

  manager->KickJob(jobA);
  manager->KickJob(jobC);
  manager->KickJob(jobB);

  manager->InvokeCycleAndWait();
  ASSERT_EQ(0, vec.at(0));
  ASSERT_EQ(1, vec.at(1));
  ASSERT_EQ(2, vec.at(2));
}

TEST(JobSystem, multiple_jobs_per_phase) {
  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();

  int job_count = 5;
  std::atomic_int counter = 0;
  for (int i = 0; i < job_count; i++) {
    auto job = JobSystemFactory::CreateJob([&](JobContext *) {
      counter++;
      return JobContinuation::REQUEUE;
    });
    manager->KickJob(job);
  }

  manager->InvokeCycleAndWait();
  ASSERT_EQ(job_count, counter);
}

TEST(JobSystem, auto_requeue) {
  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();
  int executions = 0;

  auto job = JobSystemFactory::CreateJob([&](JobContext *) {
    executions++;
    return JobContinuation::REQUEUE;
  });

  manager->KickJob(job);
  manager->InvokeCycleAndWait();
  manager->InvokeCycleAndWait();

  ASSERT_EQ(executions, 2);
}

TEST(JobSystem, timer_job) {
  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();

  size_t job_executed = 0;
  auto timer_job = JobSystemFactory::CreateJob<TimerJob>(
      [&](JobContext *) {
        job_executed++;
        return JobContinuation::REQUEUE;
      },
      1s, CLEAN_UP);

  manager->KickJob(timer_job);
  manager->InvokeCycleAndWait();
  ASSERT_EQ(0, job_executed);

  std::this_thread::sleep_for(1s);
  manager->InvokeCycleAndWait();
  ASSERT_EQ(1, job_executed);

  manager->InvokeCycleAndWait();
  ASSERT_EQ(1, job_executed);

  std::this_thread::sleep_for(1s);
  manager->InvokeCycleAndWait();
  ASSERT_EQ(2, job_executed);
}

TEST(JobSystem, jobs_kicking_jobs) {
  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();

  bool jobACompleted = false;
  bool jobBCompleted = false;
  bool jobCCompleted = false;
  bool jobDCompleted = false;
  auto jobA = JobSystemFactory::CreateJob([&](JobContext *context) {
    // job inside job (should execute directly)
    auto jobB = JobSystemFactory::CreateJob([&](JobContext *context) {
      // job inside job inside job (should also execute directly)
      auto jobC = JobSystemFactory::CreateJob([&](JobContext *context) {
        // the phase of this job has passed, so it should not be executed
        auto jobD = JobSystemFactory::CreateJob(
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

    SharedJobCounter counter = JobSystemFactory::CreateCounter();
    jobB->AddCounter(counter);
    context->GetJobManager()->KickJob(jobB);
    jobACompleted = true;
    return JobContinuation::DISPOSE;
  });

  manager->KickJob(jobA);
  manager->InvokeCycleAndWait();

  ASSERT_TRUE(jobACompleted);
  ASSERT_TRUE(jobBCompleted);
  ASSERT_TRUE(jobCCompleted);
  ASSERT_FALSE(jobDCompleted);

  manager->InvokeCycleAndWait();

  ASSERT_TRUE(jobDCompleted);
}

// Checks that no jobs are getting lost (due to data races)
TEST(JobSystem, job_bulk) {
  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();

  SharedJobCounter absolute_counter = JobSystemFactory::CreateCounter();

  for (int i = 0; i < 20; i++) {
    SharedJob job = JobSystemFactory::CreateJob(
        [absolute_counter, &manager](JobContext *context) {
          for (int i = 0; i < 10; i++) {
            SharedJob job = JobSystemFactory::CreateJob(
                [](JobContext *) { return JobContinuation::DISPOSE; });
            job->AddCounter(absolute_counter);
            manager->KickJob(job);
          }
          return JobContinuation::DISPOSE;
        });
    job->AddCounter(absolute_counter);
    manager->KickJob(job);
  }

  manager->InvokeCycleAndWait();
  if (!absolute_counter->IsFinished()) {
    throw "";
  }
  ASSERT_TRUE(absolute_counter->IsFinished());
}

TEST(JobSystem, wait_for_job_inside_job) {
// single threaded job implementations do not allow waiting inside jobs because
// doing so is the only surefire way to deadlock the entire execution
#ifndef JOB_SYSTEM_SINGLE_THREAD

  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();

  std::vector<short> order;

  SharedJob job = JobSystemFactory::CreateJob([&](JobContext *) {
    SharedJobCounter counter = JobSystemFactory::CreateCounter();
    SharedJob otherJob = JobSystemFactory::CreateJob([&](JobContext *) {
      order.push_back(1);
      return JobContinuation::DISPOSE;
    });
    otherJob->AddCounter(counter);

    manager->KickJob(otherJob);
    manager->WaitForCompletion(counter);
    order.push_back(2);
    return JobContinuation::DISPOSE;
  });

  manager->KickJob(job);
  manager->InvokeCycleAndWait();

  ASSERT_EQ(1, order.at(0));
  ASSERT_EQ(2, order.at(1));

#endif
}

TEST(JobSystem, detach_jobs) {
  std::atomic<short> execution_counter = 0;
  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();

  SharedJob job = JobSystemFactory::CreateJob([&](JobContext *context) {
    execution_counter++;
    return JobContinuation::REQUEUE;
  });

  manager->KickJob(job);
  manager->InvokeCycleAndWait();
  manager->InvokeCycleAndWait();
  ASSERT_EQ(2, execution_counter);

  manager->DetachJob(job->GetId());
  manager->InvokeCycleAndWait();
  ASSERT_EQ(2, execution_counter);
}

TEST(JobSystem, detach_jobs_mid_execution) {
// this waits for counters, what is not possible inside jobs using a single
// threaded implementation
#ifndef JOB_SYSTEM_SINGLE_THREAD
  std::atomic<short> execution_counter = 0;
  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();

  SharedJobCounter detach_job_counter = JobSystemFactory::CreateCounter();
  SharedJob job = JobSystemFactory::CreateJob([&](JobContext *context) {
    context->GetJobManager()->WaitForCompletion(detach_job_counter);
    execution_counter++;
    return JobContinuation::REQUEUE;
  });

  SharedJob detach_job = JobSystemFactory::CreateJob([&](JobContext *context) {
    context->GetJobManager()->DetachJob(job->GetId());
    return JobContinuation::DISPOSE;
  });
  detach_job->AddCounter(detach_job_counter);

  manager->KickJob(job);
  manager->KickJob(detach_job);
  manager->InvokeCycleAndWait();
  manager->InvokeCycleAndWait();

  ASSERT_EQ(1, execution_counter);
#endif
}

TEST(JobSystem, wait_for_future_completion) {
  std::vector<short> order;
  auto manager = std::make_shared<JobManager>();
  manager->StartExecution();

  SharedJob job = JobSystemFactory::CreateJob([&order](JobContext *context) {
    std::future<void> future = std::async(std::launch::async, [&order] {
      std::this_thread::sleep_for(1s);
      order.push_back(1);
    });

    order.push_back(0);
    context->GetJobManager()->WaitForCompletion(future);
    order.push_back(2);
    return JobContinuation::DISPOSE;
  });

  manager->KickJob(job);
  manager->InvokeCycleAndWait();

  ASSERT_EQ(order.at(0), 0);
  ASSERT_EQ(order.at(1), 1);
  ASSERT_EQ(order.at(2), 2);
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}