#include "services/registry/impl/remote/RemoteServiceMessagesConverter.h"
#include "logging/LogManager.h"

using namespace hive::services::impl;
using namespace hive::services;

std::optional<SharedServiceResponse>
RemoteServiceMessagesConverter::ToServiceResponse(Message &&message) {

  auto maybe_transaction_id = message.GetAttribute("transaction-id");
  if (!maybe_transaction_id.has_value()) {
    LOG_WARN("web-socket message is not a valid service response: transaction "
             "id is missing")
    return {};
  }

  auto maybe_status_code = message.GetAttribute("status");
  if (!maybe_status_code.has_value()) {
    LOG_WARN("web-socket message is not a valid service response: status code "
             "is missing")
    return {};
  }

  ServiceResponseStatus status;
  try {
    int status_number = std::stoi(maybe_status_code.value());
    status = static_cast<ServiceResponseStatus>(status_number);
  } catch (...) {
    LOG_WARN("web-socket message is not a valid service response: status code "
             "is not a valid number")
    return {};
  }

  auto status_message = message.GetAttribute("status-message").value_or("Ok");

  SharedServiceResponse response = std::make_shared<ServiceResponse>(
      maybe_transaction_id.value(), status, status_message);

  // attributes are moved to avoid copying large attributes containing heavy
  // blobs; message object looses its attributes
  for (const auto &attr_name : message.GetAttributeNames()) {
    response->SetResult<std::string>(
        attr_name, std::move(message.GetAttribute(attr_name).value()));
  }

  return response;
}

SharedMessage RemoteServiceMessagesConverter::FromServiceRequest(
    const services::ServiceRequest &request) {

  SharedMessage message = std::make_shared<Message>("service-request");

  for (const auto &result_name : request.GetParameterNames()) {
    message->SetAttribute(result_name,
                          request.GetParameter(result_name).value());
  }

  message->SetAttribute("transaction-id", request.GetTransactionId());
  message->SetAttribute("service", request.GetServiceName());

  return message;
}

std::optional<SharedServiceRequest>
RemoteServiceMessagesConverter::ToServiceRequest(const SharedMessage &message) {

  auto maybe_transaction_id = message->GetAttribute("transaction-id");
  if (!maybe_transaction_id.has_value()) {
    LOG_WARN("web-socket message is not a valid service request: transaction "
             "id is missing")
    return {};
  }

  auto maybe_service_name = message->GetAttribute("service");
  if (!maybe_service_name.has_value()) {
    LOG_WARN("web-socket message is not a valid service request: service name "
             "is missing")
    return {};
  }

  SharedServiceRequest request = std::make_shared<ServiceRequest>(
      maybe_service_name.value(), maybe_transaction_id.value());

  for (const auto &attr_name : message->GetAttributeNames()) {
    request->SetParameter(attr_name, message->GetAttribute(attr_name).value());
  }

  return request;
}

SharedMessage RemoteServiceMessagesConverter::FromServiceResponse(
    ServiceResponse &&response) {

  SharedMessage message = std::make_shared<Message>("service-response");

  for (const auto &result_name : response.GetResultNames()) {
    message->SetAttribute(result_name,
                          std::move(response.GetResult(result_name).value()));
  }

  message->SetAttribute("transaction-id", response.GetTransactionId());
  message->SetAttribute("status",
                        std::to_string(static_cast<int>(response.GetStatus())));
  message->SetAttribute("status-message", response.GetStatusMessage());

  return message;
}
