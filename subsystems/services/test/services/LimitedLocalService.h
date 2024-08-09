#pragma once

#include "services/executor/impl/LocalServiceExecutor.h"
#include <future>
#include <utility>

/**
 * Simply restricts the count of concurrent calls to a service. All service
 * instances finish when a signal is given. This allows data-race-free
 * concurrent service executions for testing purposes.
 */
class LimitedLocalService : public hive::services::impl::LocalServiceExecutor {
private:
  std::shared_future<void> m_completion_signal;
  std::shared_ptr<std::atomic_size_t> m_waiting_executions_count;
  common::memory::Reference<jobsystem::JobManager> m_job_manager;

public:
  LimitedLocalService(
      size_t limit, std::shared_future<void> completion_signal,
      std::shared_ptr<std::atomic_size_t> waiting_executions_count,
      common::memory::Reference<jobsystem::JobManager> job_manager)
      : hive::services::impl::LocalServiceExecutor(
            "limited", std::bind(&LimitedLocalService::Execute, this, _1), true,
            limit),
        m_completion_signal(std::move(completion_signal)),
        m_job_manager(job_manager),
        m_waiting_executions_count(std::move(waiting_executions_count)){};

  std::future<SharedServiceResponse>
  Execute(const SharedServiceRequest &request) {
    std::promise<SharedServiceResponse> completion_promise;
    std::future<SharedServiceResponse> completion_future =
        completion_promise.get_future();

    // wait until signal has been given
    m_waiting_executions_count->operator++();
    m_job_manager.Borrow()->WaitForCompletion(m_completion_signal);
    m_waiting_executions_count->operator--();

    completion_promise.set_value(std::make_shared<ServiceResponse>(
        request->GetTransactionId(), ServiceResponseStatus::OK));

    return completion_future;
  }
};
