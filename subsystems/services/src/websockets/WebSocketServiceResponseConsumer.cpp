#include "services/registry/impl/websockets/WebSocketServiceResponseConsumer.h"
#include "logging/LogManager.h"
#include "services/registry/impl/websockets/WebSocketServiceMessagesConverter.h"

using namespace services::impl;

void WebSocketServiceResponseConsumer::ProcessReceivedMessage(
    SharedWebSocketMessage received_message,
    WebSocketConnectionInfo connection_info) noexcept {

  auto opt_response = WebSocketServiceMessagesConverter::ToServiceResponse(
      std::move(*received_message));

  if (!opt_response.has_value()) {
    LOG_WARN(
        "received message of type '"
        << GetMessageType()
        << "' cannot be converted to service response. Response dropped. ");
    return;
  }

  SharedServiceResponse response = opt_response.value();
  std::string transaction_id = response->GetTransactionId();

  std::unique_lock lock(m_promises_mutex);
  if (!m_promises.contains(transaction_id)) {
    LOG_WARN("received service response for unknown request "
             << transaction_id);
    return;
  }

  std::promise<SharedServiceResponse> &promise = m_promises.at(transaction_id);

  switch (response->GetStatus()) {
  case OK:
    LOG_DEBUG("received service response for request " << transaction_id);
    break;
  case PARAMETER_ERROR:
    LOG_WARN("received service response for request "
             << transaction_id << ", but there was an parameter error: "
             << response->GetStatusMessage());
    break;
  case INTERNAL_ERROR:
    LOG_WARN("received service response for request "
             << transaction_id << ", but an internal error occurred: "
             << response->GetStatusMessage());
    break;
  case GONE:
    LOG_WARN("received service response for request "
             << transaction_id << ", but service is gone from endpoint: "
             << response->GetStatusMessage());
    break;
  }

  promise.set_value(response);
  m_promises.erase(transaction_id);
}

void WebSocketServiceResponseConsumer::AddResponsePromise(
    const std::string &request_id,
    std::promise<SharedServiceResponse> &&response_promise) {
  std::unique_lock lock(m_promises_mutex);
  m_promises[request_id] = std::move(response_promise);
}