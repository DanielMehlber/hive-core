#ifndef JOBBASEDMESSAGEBROKER_H
#define JOBBASEDMESSAGEBROKER_H

#include "common/subsystems/SubsystemManager.h"
#include "events/broker/IEventBroker.h"
#include "events/listener/IEventListener.h"
#include "jobsystem/manager/JobManager.h"
#include <map>
#include <memory>
#include <mutex>
#include <vector>

using namespace jobsystem;
using namespace events;

namespace events::brokers {
class JobBasedEventBroker final : public events::IEventBroker {
private:
  /**
   * @brief Maps the topic name (as string) to all of its subscribers.
   * @note Subscribers are kept as weak references (in case the listener is
   * destroyed).
   */
  std::map<std::string, std::vector<std::weak_ptr<IEventListener>>>
      m_topic_subscribers;
  mutable std::mutex m_topic_subscribers_mutex;

  /** Contains required subsystems */
  std::weak_ptr<common::subsystems::SubsystemManager> m_subsystems;

  /**
   * @brief Goes through all subscribers of all topics and checks if they are
   * still valid and if they still exist.
   */
  void CleanUpSubscribers();

public:
  explicit JobBasedEventBroker(
      const common::subsystems::SharedSubsystemManager &subsystems);
  ~JobBasedEventBroker() override;

  void FireEvent(SharedEvent event) override;
  bool HasListener(const std::string &subscriber_id,
                   const std::string &topic) const override;
  void RegisterListener(std::weak_ptr<IEventListener> listener,
                        const std::string &topic) override;
  void UnregisterListener(std::weak_ptr<IEventListener> listener) override;
  void RemoveListenerFromTopic(std::weak_ptr<IEventListener> subscriber,
                               const std::string &topic) override;
  void RemoveAllListeners() override;
};
} // namespace events::impl

#endif /* JOBBASEDMESSAGEBROKER_H */
