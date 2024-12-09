#include "common/test/TryAssertUntilTimeout.h"
#include "jobsystem/jobs/TimerJob.h"
#include "jobsystem/manager/JobManager.h"
#include <boost/atomic/atomic.hpp>
#include <future>
#include <gtest/gtest.h>

using namespace hive::jobsystem;
using namespace hive;

using namespace std::chrono_literals;

TEST(JobSystem, all_cycle_phases_should_execute) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<jobsystem::JobManager>(config);
  manager->StartExecution();

  std::vector<short> vec;
  SharedJob jobA = std::make_shared<Job>(
      [&](JobContext *context) {
        vec.push_back(0);
        return JobContinuation::DISPOSE;
      },
      "test-init-job", JobExecutionPhase::INIT);
  SharedJob jobB = std::make_shared<Job>(
      [&](JobContext *context) {
        vec.push_back(1);
        return JobContinuation::DISPOSE;
      },
      "test-main-job", JobExecutionPhase::MAIN);
  SharedJob jobC = std::make_shared<Job>(
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

TEST(JobSystem, kick_multiple_jobs_per_cycle) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  int job_count = 5;
  std::atomic_int counter = 0;
  for (int i = 0; i < job_count; i++) {
    auto job = std::make_shared<Job>(
        [&](JobContext *) {
          counter++;
          return JobContinuation::REQUEUE;
        },
        "test-job");
    manager->KickJob(job);
  }

  manager->InvokeCycleAndWait();
  ASSERT_EQ(job_count, counter);

  manager->StopExecution();
}

TEST(JobSystem, requeue_jobs_after_execution) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();
  std::atomic<int> executions = 0;

  auto job = std::make_shared<Job>(
      [&executions](JobContext *) {
        executions++;
        return JobContinuation::REQUEUE;
      },
      "auto-requeue-job");

  manager->KickJob(job);
  manager->InvokeCycleAndWait();
  manager->InvokeCycleAndWait();

  ASSERT_EQ(executions, 2);

  manager->StopExecution();
}

TEST(JobSystem, timed_jobs_should_defer) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  size_t job_executed = 0;
  auto timer_job = std::make_shared<TimerJob>(
      [&](JobContext *) {
        job_executed++;
        return JobContinuation::REQUEUE;
      },
      "timer-test-clean-up-job", 0.01s, CLEAN_UP);

  manager->KickJob(timer_job);
  manager->InvokeCycleAndWait();
  ASSERT_EQ(0, job_executed);

  std::this_thread::sleep_for(0.01s);
  manager->InvokeCycleAndWait();
  ASSERT_EQ(1, job_executed);

  manager->InvokeCycleAndWait();
  ASSERT_EQ(1, job_executed);

  std::this_thread::sleep_for(0.01s);
  manager->InvokeCycleAndWait();
  ASSERT_EQ(2, job_executed);

  manager->StopExecution();
}

TEST(JobSystem, jobs_kick_other_jobs) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  bool jobACompleted = false;
  bool jobBCompleted = false;
  bool jobCCompleted = false;
  bool jobDCompleted = false;
  auto jobA = std::make_shared<Job>(
      [&](JobContext *context) {
        // job inside job (should execute directly)
        auto jobB = std::make_shared<Job>(
            [&](JobContext *context) {
              // job inside job inside job (should also execute directly)
              auto jobC = std::make_shared<Job>(
                  [&](JobContext *context) {
                    // the phase of this job has passed, so it should not be
                    // executed
                    auto jobD = std::make_shared<Job>(
                        [&](JobContext *context) {
                          jobDCompleted = true;
                          return JobContinuation::DISPOSE;
                        },
                        "job-d", INIT);
                    context->GetJobManager()->KickJob(jobD);
                    jobCCompleted = true;
                    return JobContinuation::DISPOSE;
                  },
                  "job-c");
              context->GetJobManager()->KickJob(jobC);
              jobBCompleted = true;
              return JobContinuation::DISPOSE;
            },
            "job-b");

        SharedJobCounter counter = std::make_shared<JobCounter>();
        jobB->AddCounter(counter);
        context->GetJobManager()->KickJob(jobB);
        jobACompleted = true;
        return JobContinuation::DISPOSE;
      },
      "job-a");

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
TEST(JobSystem, execute_huge_amounts_of_jobs) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  SharedJobCounter absolute_counter = std::make_shared<JobCounter>();

  for (int i = 0; i < 20; i++) {
    SharedJob job = std::make_shared<Job>(
        [absolute_counter, &manager](JobContext *context) {
          for (int i = 0; i < 10; i++) {
            SharedJob job = std::make_shared<Job>(
                [](JobContext *) { return JobContinuation::DISPOSE; },
                "bulk-job");
            job->AddCounter(absolute_counter);
            manager->KickJob(job);
          }
          return JobContinuation::DISPOSE;
        },
        "bulk-job");
    job->AddCounter(absolute_counter);
    manager->KickJob(job);
  }

  manager->InvokeCycleAndWait();
  bool all_finished = absolute_counter->IsFinished();
  ASSERT_TRUE(all_finished);

  manager->StopExecution();
}

TEST(JobSystem, wait_for_job_inside_job) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  std::vector<short> order;

  SharedJob job = std::make_shared<Job>(
      [&](JobContext *) {
        SharedJobCounter counter = std::make_shared<JobCounter>();
        SharedJob otherJob = std::make_shared<Job>(
            [&](JobContext *) {
              order.push_back(1);
              return JobContinuation::DISPOSE;
            },
            "inside-job");
        otherJob->AddCounter(counter);

        manager->KickJob(otherJob);
        manager->Await(counter);
        order.push_back(2);
        return JobContinuation::DISPOSE;
      },
      "outside-job");

  manager->KickJob(job);
  manager->InvokeCycleAndWait();

  ASSERT_EQ(1, order.at(0));
  ASSERT_EQ(2, order.at(1));

  manager->StopExecution();
}

TEST(JobSystem, detach_jobs_after_execution) {
  std::atomic<short> execution_counter = 0;
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  SharedJob job = std::make_shared<Job>(
      [&](JobContext *context) {
        execution_counter++;
        return JobContinuation::REQUEUE;
      },
      "counter-job");

  manager->KickJob(job);
  manager->InvokeCycleAndWait();
  manager->InvokeCycleAndWait();
  ASSERT_EQ(2, execution_counter);

  manager->DetachJob(job->GetId());
  manager->InvokeCycleAndWait();
  ASSERT_EQ(2, execution_counter);

  manager->StopExecution();
}

TEST(JobSystem, detach_jobs_mid_execution) {
  std::atomic<short> execution_counter = 0;
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  SharedJobCounter detach_job_counter = std::make_shared<JobCounter>();
  SharedJob job = std::make_shared<Job>(
      [&](JobContext *context) {
        context->GetJobManager()->Await(detach_job_counter);
        execution_counter++;
        return JobContinuation::REQUEUE;
      },
      "counter-job");

  SharedJob detach_job = std::make_shared<Job>(
      [&](JobContext *context) {
        context->GetJobManager()->DetachJob(job->GetId());
        return JobContinuation::DISPOSE;
      },
      "detach-job");
  detach_job->AddCounter(detach_job_counter);

  manager->KickJob(job);
  manager->KickJob(detach_job);
  manager->InvokeCycleAndWait();
  manager->InvokeCycleAndWait();

  ASSERT_EQ(1, execution_counter);

  manager->StopExecution();
}

TEST(JobSystem, wait_for_future_completion) {
  std::vector<short> order;
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  SharedJob job = std::make_shared<Job>(
      [&order](JobContext *context) {
        std::future<void> future = std::async(std::launch::async, [&order] {
          std::this_thread::sleep_for(0.1s);
          order.push_back(1);
        });

        order.push_back(0);
        context->GetJobManager()->Await(future);
        order.push_back(2);
        return JobContinuation::DISPOSE;
      },
      "future-test-job");

  manager->KickJob(job);
  manager->InvokeCycleAndWait();

  ASSERT_EQ(order.at(0), 0);
  ASSERT_EQ(order.at(1), 1);
  ASSERT_EQ(order.at(2), 2);

  manager->StopExecution();
}

TEST(JobSynchronization, wait_for_future) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  auto promise = std::make_shared<std::promise<void>>();
  auto future = promise->get_future();

  std::atomic_bool barrier;
  barrier.store(false);

  SharedJob promise_job = std::make_shared<Job>(
      [promise, &barrier](JobContext *context) {
        // yield until the future job has been started
        while (!barrier) {
          context->GetJobManager()->WaitForDuration(10ms);
        }

        promise->set_value();
        return DISPOSE;
      },
      "promise-job");

  SharedJob future_job = std::make_shared<Job>(
      [&future, &barrier](JobContext *context) mutable {
        barrier = true;
        context->GetJobManager()->Await(future);
        return DISPOSE;
      },
      "future-job");

  manager->KickJob(promise_job);
  manager->KickJob(future_job);

  manager->InvokeCycleAndWait();

  manager->StopExecution();
}

TEST(JobSynchronization, recursive_mutex_threads) {
  bool finished = false;

  // check that multiple sequential locks can be performed by the same thread
  std::thread th([&finished] {
    jobsystem::recursive_mutex recursive_mutex;
    std::unique_lock lock1(recursive_mutex);
    std::unique_lock lock2(recursive_mutex);
    std::unique_lock lock3(recursive_mutex);
    finished = true;
  });

  common::test::TryAssertUntilTimeout([&finished]() { return finished; }, 1s);
  th.join();
}

TEST(JobSynchronization, jobs_lock_recursive_mutex) {
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = common::memory::Owner<JobManager>(config);
  job_manager->StartExecution();

  jobsystem::recursive_mutex recursive_mutex;
  std::atomic_bool should_be_currently_locked = false;

  // main idea: fire lots of jobs which all lock the same recursive mutex and
  // assert they do not enter the same critical section in parallel.
  for (int i = 0; i < 100; i++) {
    auto job = std::make_shared<Job>(
        [&recursive_mutex, &should_be_currently_locked,
         i](JobContext *context) {
          std::unique_lock lock(recursive_mutex);

          // if this is true, this means that this critical section is actually
          // locked by another fiber. The current fiber shouldn't get access to
          // this section, therefore the recursive mutex does not work as
          // expected.
          DEBUG_ASSERT(!should_be_currently_locked, "should not be locked");
          should_be_currently_locked = true;

          {
            // check if recursive locking works
            std::unique_lock lock1(recursive_mutex);
          }

          // if the recursive mutex does not lock correctly or falsely releases
          // after recursive locking, this delay introduces time for other
          // fibers to enter the critical section and cause assertion errors.
          std::this_thread::sleep_for(0.001s);
          should_be_currently_locked = false;

          return JobContinuation::DISPOSE;
        },
        "test-job");
    job_manager->KickJob(job);
  }

  job_manager->InvokeCycleAndWait();
  job_manager->StopExecution();
}

TEST(JobSystem, async_jobs) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  std::atomic<int> cycles = 0;

  SharedJob job = std::make_shared<Job>(
      [&cycles](JobContext *context) {
        while (cycles < 3) {
          std::this_thread::sleep_for(1ms);
        }
        return JobContinuation::DISPOSE;
      },
      "async-job", MAIN, true);

  manager->KickJob(job);

  for (int i = 0; i < 3; i++) {
    manager->InvokeCycleAndWait();
    cycles++;
    ASSERT_NE(job->GetState(), JobState::DETACHED);
  }

  cycles++;
  manager->InvokeCycleAndWait();

  std::this_thread::sleep_for(10ms);
  ASSERT_EQ(job->GetState(), JobState::EXECUTION_FINISHED);

  manager->StopExecution();
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}