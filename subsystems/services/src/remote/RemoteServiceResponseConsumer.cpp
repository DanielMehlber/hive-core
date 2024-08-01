#include "services/registry/impl/remote/RemoteServiceResponseConsumer.h"
#include "common/exceptions/ExceptionsBase.h"
#include "events/broker/IEventBroker.h"
#include "logging/LogManager.h"
#include "networking/messaging/events/ConnectionClosedEvent.h"
#include "services/registry/impl/remote/RemoteServiceMessagesConverter.h"

using namespace services::impl;

RemoteServiceResponseConsumer::RemoteServiceResponseConsumer(
    common::memory::Reference<common::subsystems::SubsystemManager> subsystems)
    : m_subsystems(subsystems) {}

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

  std::unique_lock lock(m_pending_promises_mutex);
  if (!m_pending_promises.contains(transaction_id)) {
    LOG_WARN("received service response for unknown request " << transaction_id)
    return;
  }

  auto pending_request = std::move(m_pending_promises.at(transaction_id));

  switch (response->GetStatus()) {
  case OK:
    LOG_DEBUG("received service response for pending request "
              << transaction_id << " from " << connection_info.hostname)
    break;
  case PARAMETER_ERROR:
    LOG_WARN("received service response for pending request "
             << transaction_id << " from " << connection_info.hostname
             << ", but there was an parameter error: "
             << response->GetStatusMessage())
    break;
  case INTERNAL_ERROR:
    LOG_WARN("received service response for pending request "
             << transaction_id << " from " << connection_info.hostname
             << ", but an internal error occurred: "
             << response->GetStatusMessage())
    break;
  case GONE:
    LOG_WARN("received service response for request "
             << transaction_id << " from " << connection_info.hostname
             << ", but service is gone from endpoint: "
             << response->GetStatusMessage())
    break;
  }

  pending_request.promise.set_value(response);
  m_pending_promises.erase(transaction_id);
}

void RemoteServiceResponseConsumer::AddResponsePromise(
    const std::string &request_id, const ConnectionInfo &connection_info,
    std::promise<SharedServiceResponse> &&response_promise) {

  std::weak_ptr<RemoteServiceResponseConsumer> weak_response_consumer =
      std::dynamic_pointer_cast<RemoteServiceResponseConsumer>(
          shared_from_this());

  // this handler will be called when the connection to the remote endpoint is
  // closed before the response is received to cancel the request and avoid
  // blocking.
  auto connection_closed_handler = std::make_shared<
      events::FunctionalEventListener>([connection_info, weak_response_consumer,
                                        request_id](
                                           events::SharedEvent event) mutable {
    networking::ConnectionClosedEvent connection_closed(std::move(event));
    std::string endpoint_id = connection_closed.GetEndpointId();

    if (connection_info.endpoint_id == endpoint_id) {
      LOG_ERR("connection to remote endpoint '"
              << endpoint_id
              << "' has been closed mid service request. Request cancelled.")

      if (auto response_consumer = weak_response_consumer.lock()) {
        auto promise =
            std::move(response_consumer->RemoveResponsePromise(request_id));
        auto exception = BUILD_EXCEPTION(
            ServiceEndpointDisconnectedException,
            "connection to remote endpoint '"
                << endpoint_id
                << "' has been closed mid service request. Request cancelled.");
        promise->set_exception(std::make_exception_ptr(exception));
      }
    }
  });

  PendingRequestInfo info{.promise = std::move(response_promise),
                          .endpoint_info = connection_info,
                          .connection_close_handler =
                              connection_closed_handler};

  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    auto event_broker = subsystems->RequireSubsystem<events::IEventBroker>();
    event_broker->RegisterListener(
        connection_closed_handler,
        networking::ConnectionClosedEvent::c_event_name);
  } else {
    LOG_WARN("cannot register handler for severed connection during service "
             "call because required subsystems are not available")
  }

  std::unique_lock lock(m_pending_promises_mutex);
  m_pending_promises[request_id] = std::move(info);
}

std::optional<std::promise<services::SharedServiceResponse>>
RemoteServiceResponseConsumer::RemoveResponsePromise(
    const std::string &request_id) {
  std::unique_lock lock(m_pending_promises_mutex);
  if (m_pending_promises.contains(request_id)) {
    auto pending_request_info = std::move(m_pending_promises.at(request_id));
    m_pending_promises.erase(request_id);
    return std::move(pending_request_info.promise);
  } else {
    return {};
  }
}
