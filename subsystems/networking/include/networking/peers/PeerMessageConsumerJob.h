#ifndef WEBSOCKETMESSAGECONSUMERJOB_H
#define WEBSOCKETMESSAGECONSUMERJOB_H

#include "IPeerMessageConsumer.h"
#include "networking/Networking.h"
#include <jobsystem/job/Job.h>

namespace networking::websockets {

/**
 * @brief A job that, when scheduled, makes a message consumer process a message
 * of a certain type.
 */
class NETWORKING_API PeerMessageConsumerJob : public jobsystem::job::Job {
protected:
  const SharedWebSocketMessage m_message;
  const SharedPeerMessageConsumer m_consumer;
  const PeerConnectionInfo m_connection_info;

  jobsystem::job::JobContinuation
  ConsumeMessage([[maybe_unused]] jobsystem::JobContext *context);

public:
  PeerMessageConsumerJob(SharedPeerMessageConsumer consumer,
                         SharedWebSocketMessage message,
                         PeerConnectionInfo connection_info);
};
} // namespace networking::websockets

#endif /* WEBSOCKETMESSAGECONSUMERJOB_H */
