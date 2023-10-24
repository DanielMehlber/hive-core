#ifndef ADDINGSERVICESTUB_H
#define ADDINGSERVICESTUB_H

#include "services/stub/impl/LocalServiceStub.h"

using namespace std::placeholders;

class AddingServiceStub : public services::impl::LocalServiceStub {
public:
  AddingServiceStub()
      : services::impl::LocalServiceStub(
            "add", std::bind(&AddingServiceStub::Add, this, _1)){};

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

SharedServiceRequest GenerateAddingRequest(int a, int b) {
  SharedServiceRequest request = std::make_shared<ServiceRequest>("add");
  request->SetParameter("a", a);
  request->SetParameter("b", b);
  return request;
}

int GetResultOfAddition(SharedServiceResponse result) {
  return std::stoi(result->GetResult("sum").value());
}

#endif // ADDINGSERVICESTUB_H
