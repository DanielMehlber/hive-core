#pragma once

#include "IMessageConsumer.h"
#include <jobsystem/job/Job.h>

namespace hive::networking::messaging {

/**
 * A job that, when scheduled, makes a message consumer process a message
 * of a certain type.
 */
class MessageConsumerJob : public jobsystem::job::Job {
protected:
  const SharedMessage m_message;
  const SharedMessageConsumer m_consumer;
  const ConnectionInfo m_connection_info;

  jobsystem::job::JobContinuation
  ConsumeMessage([[maybe_unused]] jobsystem::JobContext *context);

public:
  MessageConsumerJob(SharedMessageConsumer consumer, SharedMessage message,
                     ConnectionInfo connection_info);
};
} // namespace hive::networking::messaging
