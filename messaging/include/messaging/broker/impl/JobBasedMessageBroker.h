#ifndef JOBBASEDMESSAGEBROKER_H
#define JOBBASEDMESSAGEBROKER_H

#include "jobsystem/manager/JobManager.h"
#include "messaging/broker/IMessageBroker.h"
#include "messaging/subscriber/IMessageSubscriber.h"
#include <map>
#include <memory>
#include <mutex>
#include <vector>

using namespace jobsystem;
using namespace messaging;

namespace messaging::impl {
class JobBasedMessageBroker final : public messaging::IMessageBroker {
private:
  /**
   * @brief Maps the topic name (as string) to all of its subscribers.
   * @note Subscribers are kept as weak references (in case the subscriber is
   * destroyed).
   */
  std::map<std::string, std::vector<std::weak_ptr<IMessageSubscriber>>>
      m_topic_subscribers;
  mutable std::mutex m_topic_subscribers_mutex;

  std::shared_ptr<JobManager> m_job_manager;

  /**
   * @brief Goes through all subscribers of all topics and checks if they are
   * still valid and if they still exist.
   */
  void CleanUpSubscribers();

public:
  explicit JobBasedMessageBroker(std::shared_ptr<JobManager> job_manager);
  ~JobBasedMessageBroker() override;

  void PublishMessage(SharedMessage event) override;
  bool HasSubscriber(const std::string &subscriber_id,
                     const std::string &topic) const override;
  void AddSubscriber(std::weak_ptr<IMessageSubscriber> listener,
                     const std::string &topic) override;
  void RemoveSubscriber(std::weak_ptr<IMessageSubscriber> listener) override;
  void RemoveSubscriberFromTopic(std::weak_ptr<IMessageSubscriber> subscriber,
                                 const std::string &topic) override;
  void RemoveAllSubscribers() override;
};
} // namespace messaging::impl

#endif /* JOBBASEDMESSAGEBROKER_H */
