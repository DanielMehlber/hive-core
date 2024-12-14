#pragma once

#include "common/subsystems/SubsystemManager.h"
#include "events/listener/impl/FunctionalEventListener.h"
#include "jobsystem/synchronization/JobMutex.h"
#include "networking/messaging/consumer/IMessageConsumer.h"
#include "services/ServiceRequest.h"
#include "services/ServiceResponse.h"
#include <future>

namespace hive::services::impl {

struct PendingRequestInfo {
  /** request object associated with this pending call */
  SharedServiceRequest request;

  /** promise that must be resolved to complete the request */
  std::shared_ptr<std::promise<SharedServiceResponse>> promise;

  /** contains information about the remote endpoint that has been called */
  networking::messaging::ConnectionInfo endpoint_info;

  /** handles the case that the remote endpoint can't respond because the
   * connection has been severed. */
  std::shared_ptr<events::FunctionalEventListener> connection_close_handler;
};

/**
 * Processes incoming service responses of recently called remote services. It
 * also manages pending requests and their promises.
 */
class RemoteServiceResponseConsumer
    : public networking::messaging::IMessageConsumer {
private:
  mutable jobsystem::mutex m_pending_promises_mutex;
  std::map<std::string, PendingRequestInfo> m_pending_requests;

  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

public:
  RemoteServiceResponseConsumer() = delete;
  explicit RemoteServiceResponseConsumer(
      common::memory::Reference<common::subsystems::SubsystemManager>
          m_subsystems);

  virtual ~RemoteServiceResponseConsumer() = default;

  std::string GetMessageType() const override;

  void ProcessReceivedMessage(
      networking::messaging::SharedMessage received_message,
      networking::messaging::ConnectionInfo connection_info) override;

  /**
   * Adds response promise to this consumer. The consumer will resolve this
   * promise when its response has been received and consumed.
   * @param request request object associated with this promise
   * @param connection_info connection info of endpoint that sent the response
   * @param response_promise response promise
   */
  void AddResponsePromise(
      SharedServiceRequest request,
      const networking::messaging::ConnectionInfo &connection_info,
      std::shared_ptr<std::promise<SharedServiceResponse>> response_promise);

  /**
   * Retrieves response promise for given request id and removes it from the
   * list of pending promises.
   * @param request_id id of request
   * @return response promise or empty optional if no promise was found
   */
  std::optional<std::shared_ptr<std::promise<SharedServiceResponse>>>
  RemoveResponsePromise(const std::string &request_id);
};

inline std::string RemoteServiceResponseConsumer::GetMessageType() const {
  return "service-response";
}

} // namespace hive::services::impl
