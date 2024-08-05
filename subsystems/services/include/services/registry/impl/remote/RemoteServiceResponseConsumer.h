#pragma once

#include "common/subsystems/SubsystemManager.h"
#include "events/listener/impl/FunctionalEventListener.h"
#include "jobsystem/synchronization/JobMutex.h"
#include "networking/messaging/IMessageConsumer.h"
#include "services/ServiceResponse.h"
#include "services/registry/impl/remote/RemoteExceptions.h"
#include <future>

using namespace hive::networking::messaging;

namespace hive::services::impl {

struct PendingRequestInfo {
  /** promise that must be resolved to complete the request */
  std::promise<SharedServiceResponse> promise;

  /** contains information about the remote endpoint that has been called */
  ConnectionInfo endpoint_info;

  /** handles the case that the remote endpoint can't respond because the
   * connection has been severed. */
  std::shared_ptr<events::FunctionalEventListener> connection_close_handler;
};

/**
 * Processes incoming service responses of recently called remote services. It
 * also manages pending requests and their promises.
 */
class RemoteServiceResponseConsumer : public IMessageConsumer {
private:
  mutable jobsystem::mutex m_pending_promises_mutex;
  std::map<std::string, PendingRequestInfo> m_pending_promises;

  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

public:
  RemoteServiceResponseConsumer() = delete;
  explicit RemoteServiceResponseConsumer(
      common::memory::Reference<common::subsystems::SubsystemManager>
          m_subsystems);

  std::string GetMessageType() const noexcept override;

  void ProcessReceivedMessage(SharedMessage received_message,
                              ConnectionInfo connection_info) noexcept override;

  /**
   * Adds response promise to this consumer. The consumer will resolve this
   * promise when its response has been received and consumed.
   * @param request_id id of request (response will have same id)
   * @param connection_info connection info of endpoint that sent the response
   * @param response_promise response promise
   */
  void
  AddResponsePromise(const std::string &request_id,
                     const ConnectionInfo &connection_info,
                     std::promise<SharedServiceResponse> &&response_promise);

  /**
   * Retrieves response promise for given request id and removes it from the
   * list of pending promises.
   * @param request_id id of request
   * @return response promise or empty optional if no promise was found
   */
  std::optional<std::promise<SharedServiceResponse>>
  RemoveResponsePromise(const std::string &request_id);
};

inline std::string
RemoteServiceResponseConsumer::GetMessageType() const noexcept {
  return "service-response";
}

} // namespace hive::services::impl
