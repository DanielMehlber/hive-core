#include "services/executor/impl/LocalServiceExecutor.h"
#include "common/uuid/UuidGenerator.h"

using namespace services;
using namespace services::impl;

LocalServiceExecutor::LocalServiceExecutor(
    std::string service_name,
    std::function<std::future<SharedServiceResponse>(SharedServiceRequest)>
        func)
    : m_func(std::move(func)), m_service_name(std::move(service_name)),
      m_id(common::uuid::UuidGenerator::Random()) {}

std::future<SharedServiceResponse> LocalServiceExecutor::IssueCallAsJob(
    SharedServiceRequest request,
    common::memory::Borrower<jobsystem::JobManager> job_manager) {

  // for when the job has resolved
  std::shared_ptr<std::promise<SharedServiceResponse>> completion_promise =
      std::make_shared<std::promise<SharedServiceResponse>>();
  std::future<SharedServiceResponse> completion_future =
      completion_promise->get_future();

  jobsystem::job::SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [request, _this = shared_from_this(),
       completion_promise](jobsystem::JobContext *context) mutable {
        LOG_DEBUG("executing local service '" << request->GetServiceName()
                                              << "' for request "
                                              << request->GetTransactionId())
        try {
          auto result = _this->m_func(request);
          context->GetJobManager()->WaitForCompletion(result);
          completion_promise->set_value(result.get());
        } catch (...) {
          auto exception_ptr =
              std::make_exception_ptr(std::current_exception());
          completion_promise->set_exception(exception_ptr);
        }
        return jobsystem::job::JobContinuation::DISPOSE;
      },
      "local-service-execution-(" + request->GetServiceName() + ")-" +
          common::uuid::UuidGenerator::Random());

  job_manager->KickJob(job);

  return completion_future;
}

std::string LocalServiceExecutor::GetServiceName() { return m_service_name; }
