#undef min
#include "services/caller/impl/RoundRobinServiceCaller.h"
#include "jobsystem/manager/JobManager.h"
#include "logging/LogManager.h"
#include "services/registry/impl/remote/RemoteExceptions.h"

#include <cmath>
#include <utility>

using namespace hive::services::impl;
using namespace hive::services;
using namespace hive::jobsystem;

RoundRobinServiceCaller::RoundRobinServiceCaller(std::string service_name)
    : m_service_name(std::move(service_name)) {}

bool RoundRobinServiceCaller::IsCallable() const {
  std::unique_lock lock(m_service_executors_mutex);
  for (const auto &executor : m_service_executors) {
    if (executor->IsCallable()) {
      return true;
    }
  }

  return false;
}

void RoundRobinServiceCaller::AddExecutor(
    const SharedServiceExecutor &executor) {
  std::unique_lock lock(m_service_executors_mutex);

  DEBUG_ASSERT(executor != nullptr, "executor should not be null")
  DEBUG_ASSERT(!executor->GetId().empty(), "executor id should not be empty")
  DEBUG_ASSERT(!executor->GetServiceName().empty(),
               "service name should not be empty")
  DEBUG_ASSERT(executor->IsCallable(), "executor should be callable")

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

void RoundRobinServiceCaller::RemoveExecutor(
    const SharedServiceExecutor &executor) {

  DEBUG_ASSERT(executor != nullptr, "executor should not be null")
  RemoveExecutor(executor->GetId());
}

void RoundRobinServiceCaller::RemoveExecutor(const std::string &executor_id) {
  DEBUG_ASSERT(!executor_id.empty(), "executor id should not be empty")

  // remove executor from list
  std::unique_lock lock(m_service_executors_mutex);
  auto count_before = m_service_executors.size();
  m_service_executors.erase(
      std::remove_if(
          m_service_executors.begin(), m_service_executors.end(),
          [&executor_id](const SharedServiceExecutor &some_executor) {
            return some_executor->GetId() == executor_id;
          }),
      m_service_executors.end());
  auto count_after = m_service_executors.size();
  lock.unlock();

  if (count_before > count_after) {
    LOG_DEBUG("service executor '" << executor_id << "' for service '"
                                   << m_service_name << "' removed from caller")
  }
}

SharedJob RoundRobinServiceCaller::BuildServiceCallJob(
    SharedServiceRequest request,
    std::shared_ptr<std::promise<SharedServiceResponse>> promise,
    common::memory::Borrower<jobsystem::JobManager> job_manager,
    CallRetryPolicy retry_on_busy, bool only_local, bool async,
    std::optional<SharedServiceExecutor> maybe_executor, int attempt_number) {

  DEBUG_ASSERT(request != nullptr, "request should not be null")
  DEBUG_ASSERT(!request->GetServiceName().empty(),
               "service name should not be empty")
  DEBUG_ASSERT(!request->GetTransactionId().empty(),
               "transaction id should not be empty")
  DEBUG_ASSERT(promise != nullptr, "promise should not be null")
  DEBUG_ASSERT(attempt_number >= 0, "attempt number should be positive")

  SharedJob job = std::make_shared<Job>(
      [caller = shared_from_this(), promise, request, only_local, job_manager,
       attempt_number, retry_on_busy, async,
       maybe_executor](JobContext *context) mutable {
        // if no executor is specified, select one
        if (!maybe_executor.has_value()) {
          maybe_executor = caller->SelectNextCallableExecutor(only_local);
        }

        if (maybe_executor.has_value()) {
          // call stub
          SharedServiceExecutor executor = maybe_executor.value();
          auto future_response =
              executor->IssueCallAsJob(request, job_manager, async);

          // wait for call to complete
          context->GetJobManager()->Await(future_response);
          try {
            SharedServiceResponse response = future_response.get();

            // if called executor is busy, react using busy behavior
            if (response->GetStatus() == ServiceResponseStatus::BUSY) {
              if (retry_on_busy.max_retries >= attempt_number) {

                // wait before retrying
                job_manager->WaitForDuration(retry_on_busy.retry_interval);

                if (retry_on_busy.try_next_executor) {
                  maybe_executor.reset();
                }

                SharedJob retry_call = caller->BuildServiceCallJob(
                    request, promise, job_manager, retry_on_busy, only_local,
                    async, maybe_executor, attempt_number + 1);

                job_manager->KickJob(retry_call);
                return DISPOSE;
              }
            }

            response->SetResolutionAttempts(attempt_number);
            promise->set_value(response);
          } catch (...) {
            std::string what =
                get_content_of_exception_ptr(std::current_exception());
            auto exception = BUILD_EXCEPTION(
                CallFailedException,
                "exception occurred while calling service '"
                    << request->GetServiceName() << "' from executor '"
                    << executor->GetId() << "': " << what);
            LOG_WARN("exception occurred while calling service '"
                     << request->GetServiceName() << "' from executor "
                     << executor->GetId() << ": " << what)
            promise->set_exception(std::make_exception_ptr(exception));
          }
        } else {
          auto exception = BUILD_EXCEPTION(
              NoCallableServiceFound, "no callable stub for service '"
                                          << request->GetServiceName() << "'");
          promise->set_exception(std::make_exception_ptr(exception));
        }

        return JobContinuation::DISPOSE;
      },
      "round-robin-service-call-" + request->GetServiceName(), MAIN, async);

  return job;
}

std::future<SharedServiceResponse> RoundRobinServiceCaller::IssueCallAsJob(
    SharedServiceRequest request,
    common::memory::Borrower<JobManager> job_manager, bool only_local,
    bool async, CallRetryPolicy retry_on_busy) {

  auto promise = std::make_shared<std::promise<SharedServiceResponse>>();
  std::future<SharedServiceResponse> future = promise->get_future();

  SharedJob job = BuildServiceCallJob(request, promise, job_manager,
                                      retry_on_busy, only_local, async);

  job_manager->KickJob(job);
  return future;
}

std::optional<SharedServiceExecutor>
RoundRobinServiceCaller::SelectNextCallableExecutor(bool only_local) {
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

bool RoundRobinServiceCaller::ContainsLocallyCallable() const {
  std::unique_lock lock(m_service_executors_mutex);
  for (const auto &some_executor : m_service_executors) {
    if (some_executor->IsCallable() && some_executor->IsLocal()) {
      return true;
    }
  }

  return false;
}

size_t RoundRobinServiceCaller::GetCallableCount() const {
  std::unique_lock lock(m_service_executors_mutex);
  size_t callable_count = 0;
  for (auto &some_executor : m_service_executors) {
    if (some_executor->IsCallable()) {
      callable_count++;
    }
  }

  return callable_count;
}

capacity_t RoundRobinServiceCaller::GetCapacity(bool local_only) {
  std::unique_lock lock(m_service_executors_mutex);
  capacity_t capacity = 0;
  for (const auto &some_executor : m_service_executors) {
    if (some_executor->IsCallable() &&
        (!local_only || some_executor->IsLocal())) {
      auto some_executor_capacity = some_executor->GetCapacity();
      // capacity < 0 means unlimited. In this case, return unlimited as well
      if (some_executor_capacity < 0) {
        return UNLIMITED_CAPACITY;
      }
      capacity += some_executor->GetCapacity();
    }
  }

  return capacity;
}

std::vector<SharedServiceExecutor>
RoundRobinServiceCaller::GetCallableServiceExecutors(bool local_only) {
  std::unique_lock lock(m_service_executors_mutex);
  std::vector<SharedServiceExecutor> executors;
  for (auto &some_executor : m_service_executors) {
    if (some_executor->IsCallable()) {
      if (local_only && !some_executor->IsLocal()) {
        continue;
      }

      executors.push_back(some_executor);
    }
  }

  return executors;
}
