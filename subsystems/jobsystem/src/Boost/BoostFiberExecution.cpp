#include "common/assert/Assert.h"
#include "common/config/Configuration.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/profiling/Timer.h"
#include "jobsystem/execution/JobExecution.h"
#include "jobsystem/manager/JobManager.h"
#include "logging/LogManager.h"
#include <future>
#include <memory>
#include <thread>
#include <vector>

// include order matters here
#include "boost/fiber/all.hpp"

using namespace hive::jobsystem::execution;
using namespace hive::jobsystem;
using namespace std::chrono_literals;

typedef boost::fibers::buffered_channel<std::function<void()>> job_channel_t;

/**
 * This implementation of the job execution uses a concept called fibers,
 * which (in contrast to threads) are scheduled by a scheduler cooperatively,
 * not preemptively. In this implementation, fibers are provided by the
 * Boost.Fiber library. They replace heavy-weight native threads in this
 * application.
 *
 * @note Fibers are running on a pool of worker threads which exchange them to
 * share their work. When a job yields, the fiber it's running on might be
 * executed on a different thread after it has been revoked as last time.
 */
struct JobExecution::Impl {
  common::config::SharedConfiguration config;

  /** current number of worker threads in use to execute jobs */
  int worker_thread_count;

  /** worker threads managed by the job execution */
  std::vector<std::shared_ptr<std::thread>> worker_threads;

  /** channel used to transfer jobs to worker threads for execution */
  std::unique_ptr<job_channel_t> job_channel;

  /** Set when starting the execution and cleared when the execution has
   * stopped. */
  common::memory::Reference<JobManager> manager;
};

JobExecution::JobExecution(const common::config::SharedConfiguration &config)
    : m_impl(std::make_unique<Impl>()) {

  m_impl->worker_thread_count = config->GetAsInt("jobs.concurrency", 4);
  m_impl->job_channel = std::make_unique<job_channel_t>(1024);
  m_impl->config = config;
}

JobExecution::~JobExecution() { Stop(); }
JobExecution::JobExecution(JobExecution &&other) noexcept = default;
JobExecution &JobExecution::operator=(JobExecution &&other) noexcept = default;

void JobExecution::Schedule(const std::shared_ptr<Job> &job) {
  DEBUG_ASSERT(m_current_state == RUNNING, "execution must be running")
  DEBUG_ASSERT(m_impl->manager.CanBorrow(),
               "manager must be set and the execution running")

  auto &weak_manager = m_impl->manager;
  auto runner = [weak_manager, job]() mutable {
    if (auto maybe_manager = weak_manager.TryBorrow()) {
      auto manager = maybe_manager.value();

      JobContext context(manager->GetTotalCyclesCount(), manager);
      JobContinuation continuation = job->Execute(&context);

      if (continuation == REQUEUE) {
        manager->KickJobForNextCycle(job);
      }

      job->FinishJob();
    } else {
      LOG_ERR("Cannot execute job "
              << job->GetId()
              << " because job manager has already been destroyed")
    }
  };

  auto status = m_impl->job_channel->push(std::move(runner));

  job->SetState(AWAITING_EXECUTION);

  // check other status codes than 'success'
  if (status != boost::fibers::channel_op_status::success) {
    switch (status) {
    case boost::fibers::channel_op_status::closed:
      LOG_ERR("cannot schedule job " << job->GetId()
                                     << " because channel to fibers is closed")
      break;
    case boost::fibers::channel_op_status::full:
      LOG_WARN("job execution channel to fibers was full and blocked "
               "execution; increasing its buffer size is recommended")
      break;
    default:
      LOG_WARN("scheduling job " << job->GetId()
                                 << " failed for an unknown reason")
      break;
    }
  }
}

// Reminder to self: Do NOT move this definition into a header, even though
// it's begging to be inlined.
//
// A header that is used across compilation units implicitly re-defines
// this function for each one and messes up the underlying static/thread_local
// qualifiers used in fiber::context::active().
//
// Dear Boost developers, I love you, but this is a pain in the Ass.
// Please kill me, it took almost 6h to figure this out.
bool IsExecutedByFiber() {
  // A worker_context is always created inside a fiber.
  return boost::fibers::context::active()->is_context(
      boost::fibers::type::worker_context);
}

void JobExecution::Yield() {
  if (IsExecutedByFiber()) {
    // caller is a fiber, so yield
    boost::this_fiber::yield();
  } else {
    // caller is a thread, so block
    std::this_thread::yield();
  }
}

void ExecuteWorker(job_channel_t *job_channel, std::atomic_int *barrier) {

  /*
   * Work sharing = a scheduler takes fibers from other threads when it has no
   * work in its queue. Each scheduler is managing a thread, so this allows the
   * usage and collaboration of multiple threads for execution.
   *
   * From this call on, the thread is a fiber itself.
   */
  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();

  // notify the barrier that this fiber is ready to be used
  barrier->fetch_sub(1);

  std::function<void()> job;
  boost::fibers::channel_op_status status;
  do {
    // using channel::try_pop() and fiber::yield() instead of channel::pop()
    // directly because it causes bugs on Windows OS: Somehow it schedules the
    // main-fiber "away". This is probably a platform-specific bug of the
    // fcontext_t implementation used under the hood (Boost 1.84). Sadly, the
    // alternative WinFiber implementation for Windows causes other errors
    // giving me a headache, so this is a valid option.
    status = job_channel->try_pop(job);

    if (job) {
      auto fiber = boost::fibers::fiber(std::move(job));
      fiber.detach();
    }

    // make the main fiber yield to allow worker fibers to execute their work.
    boost::this_fiber::yield();
  } while (status != boost::fibers::channel_op_status::closed);

  LOG_WARN("worker thread " << std::this_thread::get_id()
                            << " in fiber job execution terminated")
}

void JobExecution::Start(common::memory::Borrower<JobManager> manager) {

  if (m_current_state != STOPPED) {
    return;
  }

  LOG_DEBUG("Started fiber based job executor with "
            << m_impl->worker_thread_count << " worker threads")

  std::atomic_int barrier{m_impl->worker_thread_count};

  // Spawn worker threads: They will add themselves to the workforce
  for (int i = 0; i < m_impl->worker_thread_count; i++) {
    auto worker = std::make_shared<std::thread>(
        std::bind(&ExecuteWorker, m_impl->job_channel.get(), &barrier));
    m_impl->worker_threads.push_back(std::move(worker));
  }

  while (barrier > 0) {
    std::this_thread::yield();
  }

  m_current_state = RUNNING;
  m_impl->manager = manager.ToReference();
}

void JobExecution::Stop() {
  if (m_current_state != RUNNING) {
    return;
  }

  // closing channel causes workers to exit, so they can be joined
  m_impl->job_channel->close();
  for (auto &worker : m_impl->worker_threads) {
    DEBUG_ASSERT(worker->get_id() != std::this_thread::get_id(),
                 "execution is not supposed to be terminated by one of its own "
                 "worker threads: The thread would join itself")
    worker->join();
  }

  m_impl->worker_threads.clear();
  m_current_state = STOPPED;
  m_impl->manager = common::memory::Reference<JobManager>();
}