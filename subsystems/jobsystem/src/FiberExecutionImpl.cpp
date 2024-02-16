#include "jobsystem/execution/impl/fiber/FiberExecutionImpl.h"
#include "common/assert/Assert.h"
#include "common/profiling/Timer.h"
#include "jobsystem/manager/JobManager.h"

using namespace jobsystem::execution::impl;
using namespace jobsystem::job;
using namespace std::chrono_literals;

FiberExecutionImpl::FiberExecutionImpl(
    const common::config::SharedConfiguration &config)
    : m_config(config) {
  m_worker_thread_count = config->GetAsInt("jobs.concurrency", 4);
  Init();
}

FiberExecutionImpl::~FiberExecutionImpl() { ShutDown(); }

void FiberExecutionImpl::Init() {
  m_job_channel =
      std::make_unique<boost::fibers::buffered_channel<std::function<void()>>>(
          1024);
}

void FiberExecutionImpl::ShutDown() { Stop(); }

void FiberExecutionImpl::Schedule(std::shared_ptr<Job> job) {
#ifdef ENABLE_PROFILING
  common::profiling::Timer schedule_timer("job-scheduling");
#endif
  auto &weak_manager = m_managing_instance;
  auto runner = [weak_manager, job]() {
    if (weak_manager.expired()) {
      LOG_ERR("Cannot execute job "
              << job->GetId()
              << " because job manager has already been destroyed")
      return;
    }

    auto manager = weak_manager.lock();

    JobContext context(manager->GetTotalCyclesCount(), manager);
    JobContinuation continuation = job->Execute(&context);

    if (continuation == JobContinuation::REQUEUE) {
      manager->KickJobForNextCycle(job);
    }

    job->FinishJob();
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

void FiberExecutionImpl::WaitForCompletion(
    std::shared_ptr<IJobWaitable> waitable) {

#ifdef ENABLE_PROFILING
  common::profiling::Timer waiting_timer("job-waiting-for-completion");
#endif

  bool is_called_from_fiber =
      !boost::fibers::context::active()->is_context(boost::fibers::type::none);
  if (is_called_from_fiber) {
    // caller is a fiber, so yield
    auto id_before = boost::this_fiber::get_id();
    while (!waitable->IsFinished()) {
      boost::this_fiber::yield();
    }

    auto id_after = boost::this_fiber::get_id();
    ASSERT(id_before == id_after, "fiber must not change id during yielding")
  } else {
    // caller is a thread, so block
    while (!waitable->IsFinished()) {
      std::this_thread::sleep_for(0.1ms);
    }
  }
}

void FiberExecutionImpl::Start(std::weak_ptr<JobManager> manager) {

  if (m_current_state != JobExecutionState::STOPPED) {
    return;
  }

  LOG_DEBUG("Started fiber based job executor with " << m_worker_thread_count
                                                     << " worker threads")

  // Spawn worker threads: They will add themselves to the workforce
  for (int i = 0; i < m_worker_thread_count; i++) {
    auto worker = std::make_shared<std::thread>(
        std::bind(&FiberExecutionImpl::ExecuteWorker, this));
    m_worker_threads.push_back(worker);
  }

  m_current_state = JobExecutionState::RUNNING;
  m_managing_instance = std::move(manager);
}

void FiberExecutionImpl::Stop() {

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
    ASSERT(worker->get_id() != std::this_thread::get_id(),
           "execution is not supposed to be terminated by one of its own "
           "worker threads: The thread would join itself");
    worker->join();
  }

  m_worker_threads.clear();

  m_current_state = JobExecutionState::STOPPED;
  m_managing_instance.reset();
}

void FiberExecutionImpl::ExecuteWorker() {

  /*
   * Work sharing = a scheduler takes fibers from other threads when it has no
   * work in its queue. Each scheduler is managing a thread, so this allows the
   * usage and collaboration of multiple threads for execution.
   *
   * From this call on, the thread is a fiber itself.
   */
  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();

  std::function<void()> job;
  while (boost::fibers::channel_op_status::closed != m_job_channel->pop(job)) {
    auto fiber = boost::fibers::fiber(job);
    fiber.detach();

    // avoid keeping some anonymous functions (and their captured variables)
    // hostage (caused SEGFAULTS in some scenarios) by releasing it.
    job = nullptr;
  }
}