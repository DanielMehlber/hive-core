#ifndef WEBSOCKETSERVICEREQUESTCONSUMER_H
#define WEBSOCKETSERVICEREQUESTCONSUMER_H

#include "jobsystem/manager/JobManager.h"
#include "networking/websockets/IWebSocketMessageConsumer.h"
#include "networking/websockets/IWebSocketPeer.h"
#include "services/caller/IServiceCaller.h"

using namespace networking::websockets;

namespace services::impl {
class WebSocketServiceRequestConsumer : public IWebSocketMessageConsumer {
private:
  /** Used to schedule calls */
  jobsystem::SharedJobManager m_job_manager;

  typedef std::function<std::future<std::optional<SharedServiceCaller>>(
      const std::string &, bool)>
      query_func_type;

  /** Used to possibly find a registered service */
  query_func_type m_service_query_func;

  /** Used to send responses */
  std::weak_ptr<IWebSocketPeer> m_web_socket_peer;

public:
  WebSocketServiceRequestConsumer(
      jobsystem::SharedJobManager job_manager,
      WebSocketServiceRequestConsumer::query_func_type query_func,
      const SharedWebSocketPeer &web_socket_peer);

  std::string GetMessageType() const noexcept override;

  void ProcessReceivedMessage(
      SharedWebSocketMessage received_message,
      WebSocketConnectionInfo connection_info) noexcept override;
};

inline std::string
WebSocketServiceRequestConsumer::GetMessageType() const noexcept {
  return "service-request";
}

} // namespace services::impl

#endif // WEBSOCKETSERVICEREQUESTCONSUMER_H
