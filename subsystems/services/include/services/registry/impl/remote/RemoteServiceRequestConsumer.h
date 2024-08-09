#pragma once

#include "jobsystem/manager/JobManager.h"
#include "networking/messaging/IMessageConsumer.h"
#include "networking/messaging/IMessageEndpoint.h"
#include "services/caller/IServiceCaller.h"

using namespace hive::networking::messaging;

namespace hive::services::impl {

/**
 * Processes service calls coming from remote remote hosts. It executes the
 * service and sends its response back to the caller.
 */
class RemoteServiceRequestConsumer : public IMessageConsumer {
private:
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

  typedef std::function<std::future<std::optional<SharedServiceCaller>>(
      const std::string &, bool)>
      query_func_t;

  /** Used to possibly find a registered service */
  query_func_t m_service_query_func;

  /** Used to send responses */
  common::memory::Reference<IMessageEndpoint> m_endpoint;

public:
  RemoteServiceRequestConsumer(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems,
      RemoteServiceRequestConsumer::query_func_t query_func,
      const common::memory::Reference<IMessageEndpoint> &endpoint);

  std::string GetMessageType() const  override;

  void ProcessReceivedMessage(SharedMessage received_message,
                              ConnectionInfo connection_info)  override;
};

inline std::string
RemoteServiceRequestConsumer::GetMessageType() const  {
  return "service-request";
}

} // namespace hive::services::impl
