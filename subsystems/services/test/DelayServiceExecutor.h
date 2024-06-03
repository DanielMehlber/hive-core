#pragma once

#include "services/executor/impl/LocalServiceExecutor.h"

class DelayServiceExecutor : public services::impl::LocalServiceExecutor {
public:
  DelayServiceExecutor()
      : services::impl::LocalServiceExecutor(
            "delay", std::bind(&DelayServiceExecutor::Delay, this, _1)){};

  std::future<SharedServiceResponse>
  Delay(const SharedServiceRequest &request) {
    LOG_DEBUG("executing delay service")
    auto opt_delay = request->GetParameter("delay-milliseconds");
    std::promise<SharedServiceResponse> completion_promise;
    std::future<SharedServiceResponse> completion_future =
        completion_promise.get_future();

    SharedServiceResponse response =
        std::make_shared<ServiceResponse>(request->GetTransactionId());

    if (opt_delay.has_value()) {
      int delay;
      try {
        delay = std::stoi(opt_delay.value());
      } catch (...) {
        response->SetStatus(ServiceResponseStatus::PARAMETER_ERROR);
        response->SetStatusMessage(
            "delay parameter is not convertible into a number");
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    } else {
      response->SetStatus(ServiceResponseStatus::PARAMETER_ERROR);
      response->SetStatusMessage("delay parameter is missing");
    }

    completion_promise.set_value(response);
    return completion_future;
  }

  bool IsCallable() override { return true; }
};

SharedServiceRequest GenerateDelayRequest(int delay) {
  SharedServiceRequest request = std::make_shared<ServiceRequest>("delay");
  request->SetParameter("delay-milliseconds", delay);
  return request;
}
