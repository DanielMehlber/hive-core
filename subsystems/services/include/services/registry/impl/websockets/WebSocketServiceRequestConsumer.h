#ifndef WEBSOCKETSERVICEREQUESTCONSUMER_H
#define WEBSOCKETSERVICEREQUESTCONSUMER_H

#include "jobsystem/manager/JobManager.h"
#include "networking/peers/IPeer.h"
#include "networking/peers/IPeerMessageConsumer.h"
#include "services/Services.h"
#include "services/caller/IServiceCaller.h"

using namespace networking::websockets;

namespace services::impl {

/**
 * Processes service calls coming from remote remote hosts. It executes the
 * service and sends its response back to the caller.
 */
class SERVICES_API WebSocketServiceRequestConsumer
    : public IPeerMessageConsumer {
private:
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

  typedef std::function<std::future<std::optional<SharedServiceCaller>>(
      const std::string &, bool)>
      query_func_type;

  /** Used to possibly find a registered service */
  query_func_type m_service_query_func;

  /** Used to send responses */
  std::weak_ptr<IPeer> m_web_socket_peer;

public:
  WebSocketServiceRequestConsumer(
      const common::subsystems::SharedSubsystemManager &subsystems,
      WebSocketServiceRequestConsumer::query_func_type query_func,
      const SharedWebSocketPeer &web_socket_peer);

  std::string GetMessageType() const noexcept override;

  void
  ProcessReceivedMessage(SharedWebSocketMessage received_message,
                         PeerConnectionInfo connection_info) noexcept override;
};

inline std::string
WebSocketServiceRequestConsumer::GetMessageType() const noexcept {
  return "service-request";
}

} // namespace services::impl

#endif // WEBSOCKETSERVICEREQUESTCONSUMER_H
