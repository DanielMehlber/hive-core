#include <utility>

#include "jobsystem/execution/impl/fiber/FiberExecutionImpl.h"
#include "jobsystem/manager/JobManager.h"

using namespace jobsystem::execution::impl;
using namespace jobsystem::job;
using namespace std::chrono_literals;

FiberExecutionImpl::FiberExecutionImpl() { Init(); }
FiberExecutionImpl::~FiberExecutionImpl() { ShutDown(); }

void FiberExecutionImpl::Init() {
  m_job_channel =
      std::make_unique<boost::fibers::buffered_channel<std::function<void()>>>(
          1024);
}

void FiberExecutionImpl::ShutDown() { Stop(); }

void FiberExecutionImpl::Schedule(std::shared_ptr<Job> job) {
  auto runner = [this, job]() {
    JobContext context(m_managing_instance.lock()->GetTotalCyclesCount(),
                       m_managing_instance.lock());
    JobContinuation continuation = job->Execute(&context);
    job->SetState(JobState::EXECUTION_FINISHED);

    if (continuation == JobContinuation::REQUEUE) {
      m_managing_instance.lock()->KickJobForNextCycle(job);
    }
  };

  m_job_channel->push(std::move(runner));
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

void FiberExecutionImpl::Start(std::weak_ptr<JobManager> manager) {

  if (m_current_state != JobExecutionState::STOPPED) {
    return;
  }

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

    // avoid keeping some lambdas hostage (caused SEGFAULTS in some scenarios)
    job = nullptr;
  }
}