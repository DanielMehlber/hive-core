#ifndef JOBBASEDMESSAGINGMANAGER_H
#define JOBBASEDMESSAGINGMANAGER_H

#include "jobsystem/JobManager.h"
#include "messaging/manager/IMessagingManager.h"
#include "messaging/subscriber/IMessageSubscriber.h"
#include <map>
#include <memory>
#include <mutex>
#include <vector>

using namespace jobsystem;
using namespace messaging;

namespace messaging::impl {
class JobBasedMessagingManager final : public messaging::IMessagingManager {
private:
  /**
   * @brief Maps the topic name (as string) to all of its subscribers.
   * @note Subscribers are kept as weak references (in case the subscriber is
   * destroyed).
   */
  std::map<std::string, std::vector<std::weak_ptr<IMessageSubscriber>>>
      m_topic_subscribers;

  std::shared_ptr<JobManager> m_job_manager;

  /**
   * @brief Goes through all subscribers of all topics and checks if they are
   * still valid and if they still exist.
   */
  void CleanUpSubscribers();

public:
  JobBasedMessagingManager(std::shared_ptr<JobManager> job_manager);
  virtual ~JobBasedMessagingManager();

  virtual void PublishMessage(SharedMessage event) override;
  virtual bool HasSubscriber(const std::string &listener_id,
                             const std::string &type) const override;
  virtual void AddSubscriber(std::weak_ptr<IMessageSubscriber> listener,
                             const std::string &type) override;
  virtual void
  RemoveSubscriber(std::weak_ptr<IMessageSubscriber> listener) override;
  virtual void
  RemoveSubscriberFromTopic(std::weak_ptr<IMessageSubscriber> listener,
                            const std::string &type) override;
  virtual void RemoveAllSubscribers() override;
};
} // namespace messaging::impl

#endif /* JOBBASEDMESSAGINGMANAGER_H */
