#ifndef WEBSOCKETMESSAGECONSUMERJOB_H
#define WEBSOCKETMESSAGECONSUMERJOB_H

#include "IWebSocketMessageConsumer.h"
#include <jobsystem/job/Job.h>

namespace networking::websockets {

/**
 * @brief Job that uses a consumer to process a received message.
 */
class WebSocketMessageConsumerJob : public jobsystem::job::Job {
protected:
  const SharedWebSocketMessage m_message;
  const SharedWebSocketMessageConsumer m_consumer;

  jobsystem::job::JobContinuation
  ConsumeMessage(jobsystem::JobContext *context);

public:
  WebSocketMessageConsumerJob(SharedWebSocketMessageConsumer consumer,
                              SharedWebSocketMessage message);
};
} // namespace networking::websockets

#endif /* WEBSOCKETMESSAGECONSUMERJOB_H */
