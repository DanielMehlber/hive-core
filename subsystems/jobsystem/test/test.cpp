#include "jobsystem/JobSystemFactory.h"
#include "jobsystem/manager/JobManager.h"
#include <future>
#include <gtest/gtest.h>

using namespace jobsystem;
using namespace std::chrono_literals;

TEST(JobSystem, allPhases) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<jobsystem::JobManager>(config);
  manager->StartExecution();

  std::vector<short> vec;
  SharedJob jobA = JobSystemFactory::CreateJob(
      [&](JobContext *context) {
        vec.push_back(0);
        return JobContinuation::DISPOSE;
      },
      "test-init-job", JobExecutionPhase::INIT);
  SharedJob jobB = JobSystemFactory::CreateJob(
      [&](JobContext *context) {
        vec.push_back(1);
        return JobContinuation::DISPOSE;
      },
      "test-main-job", JobExecutionPhase::MAIN);
  SharedJob jobC = JobSystemFactory::CreateJob(
      [&](JobContext *context) {
        vec.push_back(2);
        return JobContinuation::DISPOSE;
      },
      "test-clean-up-job", JobExecutionPhase::CLEAN_UP);

  manager->KickJob(jobA);
  manager->KickJob(jobC);
  manager->KickJob(jobB);

  manager->InvokeCycleAndWait();
  ASSERT_EQ(0, vec.at(0));
  ASSERT_EQ(1, vec.at(1));
  ASSERT_EQ(2, vec.at(2));

  manager->StopExecution();
}

TEST(JobSystem, multiple_jobs_per_phase) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<JobManager>(config);
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

  manager->StopExecution();
}

TEST(JobSystem, auto_requeue) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<JobManager>(config);
  manager->StartExecution();
  std::atomic<int> executions = 0;

  auto job = JobSystemFactory::CreateJob([&executions](JobContext *) {
    executions++;
    return JobContinuation::REQUEUE;
  });

  manager->KickJob(job);
  manager->InvokeCycleAndWait();
  manager->InvokeCycleAndWait();

  ASSERT_EQ(executions, 2);

  manager->StopExecution();
}

TEST(JobSystem, timer_job) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<JobManager>(config);
  manager->StartExecution();

  size_t job_executed = 0;
  auto timer_job = JobSystemFactory::CreateJob<TimerJob>(
      [&](JobContext *) {
        job_executed++;
        return JobContinuation::REQUEUE;
      },
      "timer-test-clean-up-job", 1s, CLEAN_UP);

  manager->KickJob(timer_job);
  manager->InvokeCycleAndWait();
  ASSERT_EQ(0, job_executed);

  std::this_thread::sleep_for(1.2s);
  manager->InvokeCycleAndWait();
  ASSERT_EQ(1, job_executed);

  manager->InvokeCycleAndWait();
  ASSERT_EQ(1, job_executed);

  std::this_thread::sleep_for(1.2s);
  manager->InvokeCycleAndWait();
  ASSERT_EQ(2, job_executed);

  manager->StopExecution();
}

TEST(JobSystem, jobs_kicking_jobs) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<JobManager>(config);
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

  manager->StopExecution();
}

// Checks that no jobs are getting lost (due to data races)
TEST(JobSystem, job_bulk) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<JobManager>(config);
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
  bool all_finished = absolute_counter->IsFinished();
  ASSERT_TRUE(all_finished);

  manager->StopExecution();
}

// single threaded job implementations do not allow waiting inside jobs because
// doing so is the only surefire way to deadlock the entire execution
#ifndef JOB_SYSTEM_SINGLE_THREAD
TEST(JobSystem, wait_for_job_inside_job) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<JobManager>(config);
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

  manager->StopExecution();
}
#endif

TEST(JobSystem, detach_jobs) {
  std::atomic<short> execution_counter = 0;
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<JobManager>(config);
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

  manager->StopExecution();
}

// this waits for counters, what is not possible inside jobs using a single
// threaded implementation
#ifndef JOB_SYSTEM_SINGLE_THREAD
TEST(JobSystem, detach_jobs_mid_execution) {
  std::atomic<short> execution_counter = 0;
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<JobManager>(config);
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

  manager->StopExecution();
}
#endif

TEST(JobSystem, wait_for_future_completion) {
  std::vector<short> order;
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = std::make_shared<JobManager>(config);
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

  manager->StopExecution();
}

// this waits for counters, what is not possible inside jobs using a single
// threaded implementation
#ifndef JOB_SYSTEM_SINGLE_THREAD
TEST(JobSystem, test_yield) {
  auto config = std::make_shared<common::config::Configuration>();
  config->Set("jobs.concurrency", 1);
  auto manager = std::make_shared<JobManager>(config);
  manager->StartExecution();

  auto promise = std::make_shared<std::promise<void>>();
  auto future = promise->get_future();

  SharedJob promise_job =
      JobSystemFactory::CreateJob([promise](JobContext *context) {
        for (int i = 0; i < 100; i++) {
          boost::this_fiber::yield();
        }

        promise->set_value();
        return JobContinuation::DISPOSE;
      });

  manager->KickJob(promise_job);

  SharedJob future_job =
      JobSystemFactory::CreateJob([&future](JobContext *context) {
        context->GetJobManager()->WaitForCompletion(future);
        return JobContinuation::DISPOSE;
      });
  manager->KickJob(future_job);

  manager->InvokeCycleAndWait();

  manager->StopExecution();
}
#endif

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}