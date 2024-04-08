#include "common/test/TryAssertUntilTimeout.h"
#include "jobsystem/JobSystemFactory.h"
#include "jobsystem/manager/JobManager.h"
#include "jobsystem/synchronization/JobMutex.h"
#include <future>
#include <gtest/gtest.h>

using namespace jobsystem;
using namespace std::chrono_literals;

TEST(JobSystem, allPhases) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<jobsystem::JobManager>(config);
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
  auto manager = common::memory::Owner<JobManager>(config);
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
  auto manager = common::memory::Owner<JobManager>(config);
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
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  size_t job_executed = 0;
  auto timer_job = JobSystemFactory::CreateJob<TimerJob>(
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

TEST(JobSystem, jobs_kicking_jobs) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
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
  auto manager = common::memory::Owner<JobManager>(config);
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

TEST(JobSystem, wait_for_job_inside_job) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
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

TEST(JobSystem, detach_jobs) {
  std::atomic<short> execution_counter = 0;
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
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

TEST(JobSystem, detach_jobs_mid_execution) {
  std::atomic<short> execution_counter = 0;
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
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

TEST(JobSystem, wait_for_future_completion) {
  std::vector<short> order;
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  SharedJob job = JobSystemFactory::CreateJob([&order](JobContext *context) {
    std::future<void> future = std::async(std::launch::async, [&order] {
      std::this_thread::sleep_for(0.1s);
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

TEST(JobSynchronization, wait_for_future) {
  auto config = std::make_shared<common::config::Configuration>();
  auto manager = common::memory::Owner<JobManager>(config);
  manager->StartExecution();

  auto promise = std::make_shared<std::promise<void>>();
  auto future = promise->get_future();

  SharedJob promise_job =
      JobSystemFactory::CreateJob([promise](JobContext *context) {
        std::this_thread::sleep_for(0.1s);

        promise->set_value();
        return JobContinuation::DISPOSE;
      });

  SharedJob future_job =
      JobSystemFactory::CreateJob([&future](JobContext *context) {
        // TODO: Remove
        auto *_context = boost::fibers::context::active();
        context->GetJobManager()->WaitForCompletion(future);
        return JobContinuation::DISPOSE;
      });

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

TEST(JobSynchronization, recursive_mutex_fibers) {
  auto config = std::make_shared<common::config::Configuration>();
  auto job_manager = common::memory::Owner<JobManager>(config);
  job_manager->StartExecution();

  jobsystem::recursive_mutex recursive_mutex;
  std::atomic_bool should_be_currently_locked = false;

  // main idea: fire lots of jobs which all lock the same recursive mutex and
  // assert they do not enter the same critical section in parallel.
  for (int i = 0; i < 100; i++) {
    auto job =
        std::make_shared<Job>([&recursive_mutex, &should_be_currently_locked,
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
          std::this_thread::sleep_for(0.01s);
          should_be_currently_locked = false;

          return JobContinuation::DISPOSE;
        });
    job_manager->KickJob(job);
  }

  job_manager->InvokeCycleAndWait();
  job_manager->StopExecution();
}

int main(int argc, char **argv) {

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}