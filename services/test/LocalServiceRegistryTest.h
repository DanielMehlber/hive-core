#ifndef LOCALSERVICEREGISTRYTEST_H
#define LOCALSERVICEREGISTRYTEST_H

#include "services/registry/impl/local/LocalOnlyServiceRegistry.h"
#include "services/stub/impl/LocalServiceStub.h"
#include <gtest/gtest.h>

using namespace services;
using namespace std::placeholders;

class AddingService : public services::impl::LocalServiceStub {
public:
  AddingService()
      : services::impl::LocalServiceStub(
            "add", std::bind(&AddingService::Add, this, _1)){};

  std::future<SharedServiceResponse> Add(SharedServiceRequest request) {
    auto opt_a = request->GetParameter("a");
    auto opt_b = request->GetParameter("b");
    std::promise<SharedServiceResponse> completion_promise;
    std::future<SharedServiceResponse> completion_future =
        completion_promise.get_future();

    SharedServiceResponse response =
        std::make_shared<ServiceResponse>(request->GetTransactionId());

    if (opt_a.has_value() && opt_b.has_value()) {
      int a, b;
      try {
        a = std::stoi(opt_a.value());
        b = std::stoi(opt_b.value());
      } catch (...) {
        response->SetStatus(ServiceResponseStatus::PARAMETER_ERROR);
        response->SetStatusMessage(
            "parameters are not convertible into numbers");
      }

      int result = a + b;
      response->SetResult("sum", result);
    } else {
      response->SetStatus(ServiceResponseStatus::PARAMETER_ERROR);
      response->SetStatusMessage("parameters are missing");
    }

    completion_promise.set_value(response);
    return completion_future;
  }

  bool IsUsable() override { return true; }
};

TEST(ServiceTests, adding_service) {
  SharedServiceRegistry registry =
      std::make_shared<services::impl::LocalOnlyServiceRegistry>();

  SharedServiceStub adding_service = std::make_shared<AddingService>();
  jobsystem::SharedJobManager job_manager =
      std::make_shared<jobsystem::JobManager>();

  registry->Register(adding_service);

  SharedServiceCaller service_caller = registry->Find("add").get().value();

  SharedServiceRequest request = std::make_shared<ServiceRequest>("add");
  request->SetParameter("a", 5);
  request->SetParameter("b", 6);

  auto result_fut = service_caller->Call(request, job_manager);
  job_manager->InvokeCycleAndWait();

  result_fut.wait();
  auto result = result_fut.get();
  ASSERT_EQ(11, std::stoi(result->GetResult("sum").value()));
}

#endif // LOCALSERVICEREGISTRYTEST_H
