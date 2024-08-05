#undef min
#include "services/caller/impl/RoundRobinServiceCaller.h"
#include "jobsystem/manager/JobManager.h"
#include <cmath>

using namespace hive::services::impl;
using namespace hive::services;
using namespace hive::jobsystem;

bool RoundRobinServiceCaller::IsCallable() const noexcept {
  std::unique_lock lock(m_service_executors_mutex);
  for (const auto &stub : m_service_executors) {
    if (stub->IsCallable()) {
      return true;
    }
  }

  return false;
}

void RoundRobinServiceCaller::AddExecutor(SharedServiceExecutor executor) {
  std::unique_lock lock(m_service_executors_mutex);

  // no duplicate services allowed
  for (const auto &registered_executor : m_service_executors) {

    // if executor id has already been added, do not add
    if (executor->GetId() == registered_executor->GetId()) {
      LOG_WARN("registration of duplicate executor for service '"
               << executor->GetServiceName() << "' prevented")
      return;
    }
  }

  m_service_executors.push_back(executor);
}

std::future<SharedServiceResponse> RoundRobinServiceCaller::IssueCallAsJob(
    SharedServiceRequest request,
    common::memory::Borrower<jobsystem::JobManager> job_manager,
    bool only_local, bool async) noexcept {

  std::shared_ptr<std::promise<SharedServiceResponse>> promise =
      std::make_shared<std::promise<SharedServiceResponse>>();
  std::future<SharedServiceResponse> future = promise->get_future();

  SharedJob job = std::make_shared<Job>(
      [_this = shared_from_this(), promise, request, only_local,
       job_manager](JobContext *context) {
        std::optional<SharedServiceExecutor> maybe_executor =
            _this->SelectNextUsableCaller(only_local);

        if (maybe_executor.has_value()) {
          // call stub
          SharedServiceExecutor executor = maybe_executor.value();
          auto future_result = executor->IssueCallAsJob(request, job_manager);

          // wait for call to complete
          context->GetJobManager()->WaitForCompletion(future_result);
          try {
            SharedServiceResponse response = future_result.get();
            promise->set_value(response);
          } catch (...) {
            promise->set_exception(std::current_exception());
          }
        } else {
          auto exception = BUILD_EXCEPTION(NoCallableServiceFound,
                                           "no callable stub for service "
                                               << request->GetServiceName());
          promise->set_exception(std::make_exception_ptr(exception));
        }

        return JobContinuation::DISPOSE;
      },
      "round-robin-service-call-" + request->GetServiceName(), MAIN, async);

  job_manager->KickJob(job);
  return future;
}

std::optional<SharedServiceExecutor>
RoundRobinServiceCaller::SelectNextUsableCaller(bool only_local) {
  std::unique_lock lock(m_service_executors_mutex);
  // just do one round-trip searching for finding a callable service stub.
  // Otherwise, there is none.
  size_t tries = 0;
  while (tries < m_service_executors.size()) {
    size_t next_index =
        (std::min(m_service_executors.size() - 1, m_last_index) + 1) %
        m_service_executors.size();

    const SharedServiceExecutor &stub = m_service_executors.at(next_index);
    tries++;
    m_last_index = next_index;

    if (stub->IsCallable()) {
      bool not_local_but_required_to_be = only_local && !stub->IsLocal();
      if (not_local_but_required_to_be) {
        continue;
      }
      return stub;
    }
  }

  return {};
}

bool RoundRobinServiceCaller::ContainsLocallyCallable() const noexcept {
  std::unique_lock lock(m_service_executors_mutex);
  for (const auto &some_executor : m_service_executors) {
    if (some_executor->IsCallable() && some_executor->IsLocal()) {
      return true;
    }
  }

  return false;
}

size_t RoundRobinServiceCaller::GetCallableCount() const noexcept {
  std::unique_lock lock(m_service_executors_mutex);
  size_t callable_count = 0;
  for (auto &some_executor : m_service_executors) {
    if (some_executor->IsCallable()) {
      callable_count++;
    }
  }

  return callable_count;
}
