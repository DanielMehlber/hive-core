#include "services/stub/impl/LocalServiceStub.h"
#include "common/uuid/UuidGenerator.h"

using namespace services;
using namespace services::impl;

LocalServiceStub::LocalServiceStub(
    std::function<std::future<SharedServiceResponse>(SharedServiceRequest)>
        func)
    : m_func(std::move(func)) {}

std::future<SharedServiceResponse>
LocalServiceStub::Call(SharedServiceRequest request,
                       jobsystem::SharedJobManager job_manager) {

  // for when the job has resolved
  std::shared_ptr<std::promise<SharedServiceResponse>> completion_promise =
      std::make_shared<std::promise<SharedServiceResponse>>();
  std::future<SharedServiceResponse> completion_future =
      completion_promise->get_future();

  jobsystem::job::SharedJob job = jobsystem::JobSystemFactory::CreateJob(
      [request, this,
       completion_promise](jobsystem::JobContext *context) mutable {
        try {
          auto result = this->m_func(request);
          context->GetJobManager()->WaitForCompletion(result);
          completion_promise->set_value(result.get());
        } catch (...) {
          auto exception_ptr =
              std::make_exception_ptr(std::current_exception());
          completion_promise->set_exception(exception_ptr);
        }
        return jobsystem::job::JobContinuation::DISPOSE;
      },
      "local-service-call-" + common::uuid::UuidGenerator::Random());

  job_manager->KickJob(job);

  return completion_future;
}
