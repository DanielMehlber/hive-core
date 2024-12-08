#include "events/broker/impl/JobBasedEventBroker.h"
#include "jobsystem/jobs/TimerJob.h"
#include "logging/LogManager.h"

using namespace hive::events;
using namespace hive::events::brokers;
using namespace std::chrono_literals;
using namespace hive::jobsystem;

JobBasedEventBroker::JobBasedEventBroker(
    const common::memory::Reference<common::subsystems::SubsystemManager>
        &subsystems)
    : m_subsystems(subsystems), m_this_alive_checker{std::make_shared<bool>()} {

  // when this shared pointer expired, this has been destroyed
  std::weak_ptr alive_checker = m_this_alive_checker;

  SharedJob clean_up_job = std::make_shared<TimerJob>(
      [&, alive_checker](JobContext *) {
        if (!alive_checker.expired()) {
          this->CleanUpSubscribers();
          return REQUEUE;
        }

        return DISPOSE;
      },
      "events-listener-clean-up", 5s, INIT);

  auto job_manager = m_subsystems.Borrow()->RequireSubsystem<JobManager>();
  job_manager->KickJob(clean_up_job);
}

JobBasedEventBroker::~JobBasedEventBroker() {
  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    auto job_manager = subsystems->RequireSubsystem<JobManager>();
    job_manager->DetachJob("events-listener-clean-up");
  }

  RemoveAllListeners();
}

void JobBasedEventBroker::CleanUpSubscribers() {
  std::unique_lock subscriber_lock(m_event_listeners_mutex);
  for (auto &[topic, original_subscribers] : m_event_listeners) {
    std::vector<std::weak_ptr<IEventListener>> new_subscribers;
    for (auto &subscriber : original_subscribers) {
      if (!subscriber.expired()) {
        new_subscribers.push_back(subscriber);
      }
    }

    original_subscribers.swap(new_subscribers);
  }
}

void JobBasedEventBroker::FireEvent(SharedEvent event) {
  if (auto maybe_subsystems = m_subsystems.TryBorrow()) {
    auto subsystems = maybe_subsystems.value();
    const auto &topic_name = event->GetTopic();

    std::unique_lock subscriber_lock(m_event_listeners_mutex);
    if (m_event_listeners.contains(topic_name)) {
      auto &subscribers_of_topic = m_event_listeners.at(topic_name);
      for (auto &subscriber : subscribers_of_topic) {
        if (!subscriber.expired()) {
          SharedJob event_job = std::make_shared<Job>(
              [subscriber, event](JobContext *) {
                if (!subscriber.expired()) {
                  subscriber.lock()->HandleEvent(event);
                }
                return JobContinuation::DISPOSE;
              },
              "fire-event-" + event->GetId());

          auto job_manager = subsystems->RequireSubsystem<JobManager>();
          job_manager->KickJob(event_job);
        }
      }

      LOG_DEBUG("event of topic '" << topic_name << "' published to "
                                   << subscribers_of_topic.size()
                                   << " subscribers")
    }
  } else {
    LOG_ERR("cannot fire event of topic '"
            << event->GetTopic()
            << "' because required subsystems are not available")
  }
}

bool JobBasedEventBroker::HasListener(const std::string &subscriber_id,
                                      const std::string &topic) const {
  if (m_event_listeners.contains(topic)) {
    const auto &subscriber_list = m_event_listeners.at(topic);
    for (const auto &subscriber : subscriber_list) {
      if (!subscriber.expired() &&
          subscriber.lock()->GetId() == subscriber_id) {
        return true;
      }
    }
  }

  return false;
}

void JobBasedEventBroker::RegisterListener(
    std::weak_ptr<IEventListener> listener, const std::string &topic) {
  std::unique_lock subscriber_lock(m_event_listeners_mutex);
  if (!HasListener(listener.lock()->GetId(), topic)) {
    if (!m_event_listeners.contains(topic)) {
      std::vector<std::weak_ptr<IEventListener>> vec;
      m_event_listeners[topic] = vec;
    }
    m_event_listeners[topic].push_back(listener);
  }
}

void JobBasedEventBroker::UnregisterListener(
    std::weak_ptr<IEventListener> listener) {
  for (const auto &[topic, listener_set] : m_event_listeners) {
    RemoveListenerFromTopic(listener, topic);
  }
}

void JobBasedEventBroker::RemoveListenerFromTopic(
    std::weak_ptr<IEventListener> subscriber, const std::string &topic) {
  std::unique_lock subscriber_lock(m_event_listeners_mutex);
  if (m_event_listeners.contains(topic)) {
    auto &subscriber_list = m_event_listeners.at(topic);
    for (auto subscriber_iter = subscriber_list.begin();
         subscriber_iter != subscriber_list.end(); subscriber_iter++) {
      // always check if listener is still valid
      if ((*subscriber_iter).expired()) {
        subscriber_list.erase(subscriber_iter);
      } else if ((*subscriber_iter).lock()->GetId() ==
                 subscriber.lock()->GetId()) {
        subscriber_list.erase(subscriber_iter);
        break;
      }
    }
  }
}

void JobBasedEventBroker::RemoveAllListeners() {
  std::unique_lock subscriber_lock(m_event_listeners_mutex);
  m_event_listeners.clear();
}