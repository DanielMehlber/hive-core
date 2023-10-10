#include "networking/websockets/WebSocketMessageConsumerJob.h"
#include <functional>

using namespace networking::websockets;
using namespace std::placeholders;

WebSocketMessageConsumerJob::WebSocketMessageConsumerJob(
    SharedWebSocketMessageConsumer consumer, SharedWebSocketMessage message)
    : jobsystem::job::Job(
          std::bind(&WebSocketMessageConsumerJob::ConsumeMessage, this, _1),
          "consume-web-socket-message-type-" + message->GetType()),
      m_consumer{consumer}, m_message{message} {}

jobsystem::job::JobContinuation
networking::websockets::WebSocketMessageConsumerJob::ConsumeMessage(
    jobsystem::JobContext *context) {
  m_consumer->ProcessReceivedMessage(m_message);
  return jobsystem::job::JobContinuation::DISPOSE;
}