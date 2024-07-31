#pragma once

#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "events/broker/IEventBroker.h"
#include "events/listener/IEventListener.h"
#include "jobsystem/manager/JobManager.h"
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace events::brokers {

/**
 * Using the jobsystem queue by wrapping the task of notifying events into jobs
 * instead of employing its own event queue.
 */
class JobBasedEventBroker final : public events::IEventBroker {
private:
  /**
   * Maps the topic name (as string) to all of its subscribers.
   * @note Subscribers are kept as weak references (in case the listener is
   * destroyed).
   */
  std::map<std::string, std::vector<std::weak_ptr<IEventListener>>>
      m_event_listeners;
  mutable jobsystem::mutex m_event_listeners_mutex;

  /** Contains required subsystems */
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

  /**
   * Goes through all subscribers of all topics and checks if they are
   * still valid and if they still exist.
   */
  void CleanUpSubscribers();
  std::shared_ptr<bool> m_this_alive_checker;

public:
  explicit JobBasedEventBroker(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems);
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
} // namespace events::brokers
