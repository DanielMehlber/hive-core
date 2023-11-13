#ifndef WEBSOCKETMESSAGECONSUMERJOB_H
#define WEBSOCKETMESSAGECONSUMERJOB_H

#include "IWebSocketMessageConsumer.h"
#include "networking/Networking.h"
#include <jobsystem/job/Job.h>

namespace networking::websockets {

/**
 * @brief A job that, when scheduled, makes a message consumer process a message
 * of a certain type.
 */
class NETWORKING_API WebSocketMessageConsumerJob : public jobsystem::job::Job {
protected:
  const SharedWebSocketMessage m_message;
  const SharedWebSocketMessageConsumer m_consumer;
  const WebSocketConnectionInfo m_connection_info;

  jobsystem::job::JobContinuation
  ConsumeMessage([[maybe_unused]] jobsystem::JobContext *context);

public:
  WebSocketMessageConsumerJob(SharedWebSocketMessageConsumer consumer,
                              SharedWebSocketMessage message,
                              WebSocketConnectionInfo connection_info);
};
} // namespace networking::websockets

#endif /* WEBSOCKETMESSAGECONSUMERJOB_H */
