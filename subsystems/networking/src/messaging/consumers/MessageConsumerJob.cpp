#include "networking/messaging/consumer/MessageConsumerJob.h"
#include <functional>

using namespace hive::networking::messaging;
using namespace std::placeholders;
using namespace hive::jobsystem;

MessageConsumerJob::MessageConsumerJob(SharedMessageConsumer consumer,
                                       SharedMessage message,
                                       ConnectionInfo connection_info)
    : Job(std::bind(&MessageConsumerJob::ConsumeMessage, this, _1),
          "consume-web-socket-message-type-" + message->GetType(),
          JobExecutionPhase::MAIN),
      m_consumer{std::move(consumer)}, m_message{message},
      m_connection_info(std::move(connection_info)) {}

JobContinuation MessageConsumerJob::ConsumeMessage(
    [[maybe_unused]] jobsystem::JobContext *context) {
  m_consumer->ProcessReceivedMessage(m_message, m_connection_info);
  return jobsystem::JobContinuation::DISPOSE;
}