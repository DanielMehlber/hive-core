#include "jobsystem/execution/impl/fiber/FiberExecutionImpl.h"
#include "jobsystem/manager/JobManager.h"

using namespace jobsystem::execution::impl;
using namespace jobsystem::job;
using namespace std::chrono_literals;

FiberExecutionImpl::FiberExecutionImpl() { Init(); }
FiberExecutionImpl::~FiberExecutionImpl() { ShutDown(); }

void FiberExecutionImpl::Init() {}

void FiberExecutionImpl::ShutDown() { Stop(); }

void FiberExecutionImpl::Schedule(std::shared_ptr<Job> job) {
  auto runner = [this, job]() {
    JobContext context(m_managing_instance->GetTotalCyclesCount(),
                       m_managing_instance);
    JobContinuation continuation = job->Execute(&context);
    job->SetState(JobState::EXECUTION_FINISHED);
    if (continuation == JobContinuation::REQUEUE) {
      m_managing_instance->KickJobForNextCycle(job);
    }
  };

  m_job_channel->push(runner);
}

void FiberExecutionImpl::WaitForCompletion(
    std::shared_ptr<IJobWaitable> waitable) {

  bool is_called_from_fiber =
      !boost::fibers::context::active()->is_context(boost::fibers::type::none);
  if (is_called_from_fiber) {
    // caller is a fiber, so yield
    while (!waitable->IsFinished()) {
      boost::this_fiber::yield();
    }
  } else {
    // caller is a thread, so block
    while (!waitable->IsFinished()) {
      std::this_thread::sleep_for(1ms);
    }
  }
}

void FiberExecutionImpl::Start(JobManager *manager) {

  if (m_current_state != JobExecutionState::STOPPED) {
    return;
  }

  m_job_channel =
      std::make_unique<boost::fibers::buffered_channel<std::function<void()>>>(
          1024);

  std::shared_ptr<boost::fibers::barrier> m_init_sync_barrier =
      std::make_shared<boost::fibers::barrier>(m_worker_thread_count + 1);
  // Spawn worker threads: They will add themselfes to the workforce
  for (int i = 0; i < m_worker_thread_count; i++) {
    auto worker = std::make_shared<std::thread>(std::bind(
        &FiberExecutionImpl::ExecuteWorker, this, m_init_sync_barrier));
    m_worker_threads.push_back(worker);
  }

  // wait until all threads have started
  // m_init_sync_barrier->wait();
  m_current_state = JobExecutionState::RUNNING;
  m_managing_instance = manager;
}

void FiberExecutionImpl::Stop() {

  if (m_current_state != JobExecutionState::RUNNING) {
    return;
  }

  // closing channel causes workers to exit, so they can be joined
  m_job_channel->close();
  for (auto &worker : m_worker_threads) {
    worker->join();
  }

  m_worker_threads.clear();

  m_current_state = JobExecutionState::STOPPED;
  m_managing_instance = nullptr;
}

void FiberExecutionImpl::ExecuteWorker(
    std::shared_ptr<boost::fibers::barrier> m_init_sync_barrier) {

  /*
   * Work stealing = a scheduler takes fibers from other threads when it has no
   * work in its queue. Each scheduler is managing a thread, so this allows the
   * usage and collaboration of mutliple threads for execution.
   *
   * From this call on, the thread is a fiber iteself.
   */
  boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();

  // wait for all worker threads to be ready
  // m_init_sync_barrier->wait();

  /*
   * Remember: This is no longer a thread: It is a fiber. This is also not a
   * normal mutex or a normal condition variable: This is a fiber-mutex and and
   * fiber-condition-variable.
   *
   * This means that the main-fiber is paused until the termination signal is
   * sent, not the current thread. Under the hood, it schedules other fibers as
   * long as this condition variable is blocking the main-fiber.
   *
   * When the termination flag and condition triggers, this thread will complete
   * and is therefore joinable.
   */
  std::function<void()> job;
  while (boost::fibers::channel_op_status::closed != m_job_channel->pop(job)) {
    auto fiber = boost::fibers::fiber(job);
    fiber.detach();
  }
}