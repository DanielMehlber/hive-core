#include "jobsystem/execution/impl/fiber/BoostFiberExecution.h"
#include "common/assert/Assert.h"
#include "common/profiling/Timer.h"
#include "jobsystem/manager/JobManager.h"

using namespace jobsystem::execution::impl;
using namespace jobsystem::job;
using namespace std::chrono_literals;

BoostFiberExecution::BoostFiberExecution(
    const common::config::SharedConfiguration &config)
    : m_config(config) {
  m_worker_thread_count = config->GetAsInt("jobs.concurrency", 4);
  Init();
}

BoostFiberExecution::~BoostFiberExecution() { ShutDown(); }

void BoostFiberExecution::Init() {
  m_job_channel =
      std::make_unique<boost::fibers::buffered_channel<std::function<void()>>>(
          1024);
}

void BoostFiberExecution::ShutDown() { Stop(); }

void BoostFiberExecution::Schedule(std::shared_ptr<Job> job) {
#ifdef ENABLE_PROFILING
  common::profiling::Timer schedule_timer("job-scheduling");
#endif
  auto &weak_manager = m_managing_instance;
  auto runner = [weak_manager, job]() mutable {
    if (auto maybe_manager = weak_manager.TryBorrow()) {
      auto manager = maybe_manager.value();

      JobContext context(manager->GetTotalCyclesCount(), manager);
      JobContinuation continuation = job->Execute(&context);

      if (continuation == JobContinuation::REQUEUE) {
        manager->KickJobForNextCycle(job);
      }

      job->FinishJob();
    } else {
      LOG_ERR("Cannot execute job "
              << job->GetId()
              << " because job manager has already been destroyed")
      return;
    }
  };

  auto status = m_job_channel->push(std::move(runner));

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
               "execution; inceasing its buffer size is recommended")
      break;
    }
  }
}

void BoostFiberExecution::WaitForCompletion(
    std::shared_ptr<IJobWaitable> waitable) {

#ifdef ENABLE_PROFILING
  common::profiling::Timer waiting_timer("job-waiting-for-completion");
#endif

  if (IsExecutedByFiber()) {
    // caller is a fiber, so yield
    auto id_before = boost::this_fiber::get_id();
    while (!waitable->IsFinished()) {
      boost::this_fiber::yield();
    }

    auto id_after = boost::this_fiber::get_id();
    DEBUG_ASSERT(id_before == id_after,
                 "fiber must not change id during yielding")
  } else {
    // caller is a thread, so block
    while (!waitable->IsFinished()) {
      std::this_thread::yield();
    }
  }
}

void BoostFiberExecution::Start(common::memory::Borrower<JobManager> manager) {

  if (m_current_state != JobExecutionState::STOPPED) {
    return;
  }

  LOG_DEBUG("Started fiber based job executor with " << m_worker_thread_count
                                                     << " worker threads")

  // Spawn worker threads: They will add themselves to the workforce
  for (int i = 0; i < m_worker_thread_count; i++) {
    auto worker = std::make_shared<std::thread>(
        std::bind(&BoostFiberExecution::ExecuteWorker, this));
    m_worker_threads.push_back(worker);
  }

  m_current_state = JobExecutionState::RUNNING;
  m_managing_instance = manager.ToReference();
}

void BoostFiberExecution::Stop() {

  if (m_current_state != JobExecutionState::RUNNING) {
    return;
  }

  // closing channel causes workers to exit, so they can be joined
  m_job_channel->close();
  for (auto &worker : m_worker_threads) {

    /*
     * *** Known bug ahead ***
     *
     * A job in execution has a shared_ptr of the job manager. In some rare
     * unit testing occasions, a timer job was still in execution (outside of
     * cycle?) and therefore the job manager was not destroyed directly.
     *
     * It was destroyed after the job has finished and its shared_ptr has gone
     * out of scope. Therefore, the thread indirectly calling the destructor was
     * the fiber (worker thread) that executed the job, not the main thread. It
     * tried to join itself in the following statement.
     *
     * To fix this
     * - the cause of the out-of-cycle job execution must be found (maybe a
     * synchronization problem)
     * - or this function has to be called at the end of every program by the
     * main thread
     */
    // TODO: Fix this bug
    DEBUG_ASSERT(worker->get_id() != std::this_thread::get_id(),
                 "execution is not supposed to be terminated by one of its own "
                 "worker threads: The thread would join itself")
    worker->join();
  }

  m_worker_threads.clear();

  m_current_state = JobExecutionState::STOPPED;
  m_managing_instance = common::memory::Reference<JobManager>();
}

void BoostFiberExecution::ExecuteWorker() {

  /*
   * Work sharing = a scheduler takes fibers from other threads when it has no
   * work in its queue. Each scheduler is managing a thread, so this allows the
   * usage and collaboration of multiple threads for execution.
   *
   * From this call on, the thread is a fiber itself.
   */
  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();

  std::function<void()> job;
  boost::fibers::channel_op_status status;
  do {
    // using channel::try_pop() and fiber::yield() instead of channel::pop()
    // directly because it causes bugs on Windows OS: Somehow it schedules the
    // main-fiber "away". This is probably a platform-specific bug of the
    // fcontext_t implementation used under the hood (Boost 1.84). Sadly, the
    // alternative WinFiber implementation for Windows causes other errors
    // giving me a headache, so this is a valid option.
    status = m_job_channel->try_pop(job);

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

// Reminder to self: Do NOT move this definition into a header, even though
// it's begging to be inlined.
//
// A header that is used across compilation units implicitily re-defines
// this function for each one and messes up the underlying static/thread_local
// qualifiers used in fiber::context::active().
//
// C++ is such a beeatifulll language :D
// (please kill me, it took almost 6h to figure this out).
bool jobsystem::execution::impl::IsExecutedByFiber() {
  // A worker_context is always created inside a fiber.
  return boost::fibers::context::active()->is_context(
      boost::fibers::type::worker_context);
}