#include "services/executor/impl/LocalServiceExecutor.h"
#include "common/uuid/UuidGenerator.h"
#include "logging/LogManager.h"
#include "services/registry/impl/remote/RemoteExceptions.h"

using namespace hive::services;
using namespace hive::services::impl;
using namespace hive::jobsystem;

LocalServiceExecutor::LocalServiceExecutor(std::string service_name,
                                           service_functor_t func, bool async,
                                           size_t capacity)
    : m_func(std::move(func)), m_service_name(std::move(service_name)),
      m_id(common::uuid::UuidGenerator::Random()), m_capacity(capacity),
      m_async(async) {}

std::future<SharedServiceResponse> LocalServiceExecutor::IssueCallAsJob(
    SharedServiceRequest request,
    common::memory::Borrower<jobsystem::JobManager> job_manager, bool async) {

  // for when the job has resolved
  auto completion_promise =
      std::make_shared<std::promise<SharedServiceResponse>>();
  std::future<SharedServiceResponse> completion_future =
      completion_promise->get_future();

  // the service itself determines if it should run async or not
  async = m_async;

  SharedJob job = std::make_shared<Job>(
      [request, executor = shared_from_this(),
       completion_promise](jobsystem::JobContext *context) mutable {
        // check if further calls are allowed or if concurrecy limit is reached
        std::unique_lock lock(executor->m_concurrent_calls_mutex);
        bool concurrency_limit_reached =
            executor->m_capacity > 0 &&
            executor->m_current_concurrent_calls >= executor->m_capacity;
        if (concurrency_limit_reached) {
          SharedServiceResponse response = std::make_shared<ServiceResponse>(
              request->GetTransactionId(), ServiceResponseStatus::BUSY,
              "Service is busy, try again later");
          completion_promise->set_value(response);

          LOG_DEBUG("local service " << executor->m_service_name
                                     << " is too busy to process request "
                                     << request->GetTransactionId())

          return JobContinuation::DISPOSE;
        }
        executor->m_current_concurrent_calls++;
        lock.unlock();

        LOG_DEBUG("executing local service '" << request->GetServiceName()
                                              << "' for request "
                                              << request->GetTransactionId())
        try {
          auto result = executor->m_func(request);
          context->GetJobManager()->WaitForCompletion(result);
          completion_promise->set_value(result.get());
        } catch (...) {
          auto exception_ptr =
              std::make_exception_ptr(std::current_exception());
          std::string what = get_content_of_exception_ptr(exception_ptr);
          LOG_WARN("exception occured while executing local service '"
                   << executor->GetServiceName() << "': " << what)
          auto exception = BUILD_EXCEPTION(
              CallFailedException,
              "exception occured while executing local service '"
                  << executor->GetServiceName() << "': " << what);
          completion_promise->set_exception(std::make_exception_ptr(exception));
        }

        executor->m_current_concurrent_calls--;

        return JobContinuation::DISPOSE;
      },
      "local-service-execution-" + request->GetTransactionId(), MAIN, async);

  job_manager->KickJob(job);

  return completion_future;
}
