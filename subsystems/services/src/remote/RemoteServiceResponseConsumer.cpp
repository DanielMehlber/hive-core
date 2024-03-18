#include "services/registry/impl/remote/RemoteServiceResponseConsumer.h"
#include "logging/LogManager.h"
#include "services/registry/impl/remote/RemoteServiceMessagesConverter.h"

using namespace services::impl;

void RemoteServiceResponseConsumer::ProcessReceivedMessage(
    SharedMessage received_message, ConnectionInfo connection_info) noexcept {

  auto maybe_response = RemoteServiceMessagesConverter::ToServiceResponse(
      std::move(*received_message));

  if (!maybe_response.has_value()) {
    LOG_WARN("received message of type '"
             << GetMessageType()
             << "' cannot be converted to service response. Response dropped. ")
    return;
  }

  SharedServiceResponse response = maybe_response.value();
  std::string transaction_id = response->GetTransactionId();

  std::unique_lock lock(m_promises_mutex);
  if (!m_promises.contains(transaction_id)) {
    LOG_WARN("received service response for unknown request " << transaction_id)
    return;
  }

  std::promise<SharedServiceResponse> &promise = m_promises.at(transaction_id);

  switch (response->GetStatus()) {
  case OK:
    LOG_DEBUG("received service response for pending request "
              << transaction_id << " from " << connection_info.GetHostname())
    break;
  case PARAMETER_ERROR:
    LOG_WARN("received service response for pending request "
             << transaction_id << " from " << connection_info.GetHostname()
             << ", but there was an parameter error: "
             << response->GetStatusMessage())
    break;
  case INTERNAL_ERROR:
    LOG_WARN("received service response for pending request "
             << transaction_id << " from " << connection_info.GetHostname()
             << ", but an internal error occurred: "
             << response->GetStatusMessage())
    break;
  case GONE:
    LOG_WARN("received service response for request "
             << transaction_id << " from " << connection_info.GetHostname()
             << ", but service is gone from endpoint: "
             << response->GetStatusMessage())
    break;
  }

  promise.set_value(response);
  m_promises.erase(transaction_id);
}

void RemoteServiceResponseConsumer::AddResponsePromise(
    const std::string &request_id,
    std::promise<SharedServiceResponse> &&response_promise) {
  std::unique_lock lock(m_promises_mutex);
  m_promises[request_id] = std::move(response_promise);
}