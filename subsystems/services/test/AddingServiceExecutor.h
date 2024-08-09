#pragma once

#include "logging/LogManager.h"
#include "services/executor/impl/LocalServiceExecutor.h"

using namespace std::placeholders;

class AddingServiceExecutor
    : public hive::services::impl::LocalServiceExecutor {
public:
  std::atomic_size_t count{0};

  AddingServiceExecutor()
      : hive::services::impl::LocalServiceExecutor(
            "add", std::bind(&AddingServiceExecutor::Add, this, _1)){};

  std::future<SharedServiceResponse> Add(const SharedServiceRequest &request) {
    LOG_DEBUG("executing adding service")
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
    count++;
    return completion_future;
  }

  bool IsCallable() override { return true; }
};

SharedServiceRequest GenerateAddingRequest(int a, int b) {
  SharedServiceRequest request = std::make_shared<ServiceRequest>("add");
  request->SetParameter("a", a);
  request->SetParameter("b", b);
  return request;
}

int GetResultOfAddition(const SharedServiceResponse &result) {
  return std::stoi(result->GetResult("sum").value());
}
