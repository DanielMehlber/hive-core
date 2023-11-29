#include "services/registry/impl/websockets/WebSocketServiceMessagesConverter.h"
#include "logging/LogManager.h"

using namespace services::impl;
using namespace services;

std::optional<SharedServiceResponse>
WebSocketServiceMessagesConverter::ToServiceResponse(PeerMessage &&message) {

  auto opt_transaction_id = message.GetAttribute("transaction-id");
  if (!opt_transaction_id.has_value()) {
    LOG_WARN("web-socket message is not a valid service response: transaction "
             "id is missing");
    return {};
  }

  auto opt_status_code = message.GetAttribute("status");
  if (!opt_status_code.has_value()) {
    LOG_WARN("web-socket message is not a valid service response: status code "
             "is missing");
    return {};
  }

  ServiceResponseStatus status;
  try {
    int status_number = std::stoi(opt_status_code.value());
    status = static_cast<ServiceResponseStatus>(status_number);
  } catch (...) {
    LOG_WARN("web-socket message is not a valid service response: status code "
             "is not a valid number");
    return {};
  }

  auto status_message = message.GetAttribute("status-message").value_or("Ok");

  SharedServiceResponse response = std::make_shared<ServiceResponse>(
      opt_transaction_id.value(), status, status_message);

  // attributes are moved to avoid copying large attributes containing heavy
  // blobs; message object looses its attributes
  for (const auto &attr_name : message.GetAttributeNames()) {
    response->SetResult<std::string>(
        attr_name, std::move(message.GetAttribute(attr_name).value()));
  }

  return response;
}

SharedWebSocketMessage WebSocketServiceMessagesConverter::FromServiceRequest(
    const services::ServiceRequest &request) {

  SharedWebSocketMessage message =
      std::make_shared<PeerMessage>("service-request");

  for (const auto &result_name : request.GetParameterNames()) {
    message->SetAttribute(result_name,
                          request.GetParameter(result_name).value());
  }

  message->SetAttribute("transaction-id", request.GetTransactionId());
  message->SetAttribute("service", request.GetServiceName());

  return message;
}

std::optional<SharedServiceRequest>
WebSocketServiceMessagesConverter::ToServiceRequest(
    const SharedWebSocketMessage &message) {

  auto opt_transaction_id = message->GetAttribute("transaction-id");
  if (!opt_transaction_id.has_value()) {
    LOG_WARN("web-socket message is not a valid service request: transaction "
             "id is missing");
    return {};
  }

  auto opt_service_name = message->GetAttribute("service");
  if (!opt_service_name.has_value()) {
    LOG_WARN("web-socket message is not a valid service request: service name "
             "is missing");
    return {};
  }

  SharedServiceRequest request = std::make_shared<ServiceRequest>(
      opt_service_name.value(), opt_transaction_id.value());

  for (const auto &attr_name : message->GetAttributeNames()) {
    request->SetParameter(attr_name, message->GetAttribute(attr_name).value());
  }

  return request;
}

SharedWebSocketMessage WebSocketServiceMessagesConverter::FromServiceResponse(
    ServiceResponse &&response) {

  SharedWebSocketMessage message =
      std::make_shared<PeerMessage>("service-response");

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
