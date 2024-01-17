#include "events/broker/impl/JobBasedEventBroker.h"

using namespace events;
using namespace events::brokers;
using namespace std::chrono_literals;

JobBasedEventBroker::JobBasedEventBroker(
    const common::subsystems::SharedSubsystemManager &subsystems)
    : m_subsystems(subsystems) {

  SharedJob clean_up_job = std::make_shared<TimerJob>(
      [&](JobContext *) {
        this->CleanUpSubscribers();
        return JobContinuation::REQUEUE;
      },
      "events-listener-clean-up", 5s, JobExecutionPhase::INIT);

  auto job_manager = m_subsystems.lock()->RequireSubsystem<JobManager>();
  job_manager->KickJob(clean_up_job);
}

JobBasedEventBroker::~JobBasedEventBroker() {
  if (!m_subsystems.expired()) {
    auto job_manager = m_subsystems.lock()->RequireSubsystem<JobManager>();
    job_manager->DetachJob("events-listener-clean-up");
  }
  RemoveAllListeners();
}

void JobBasedEventBroker::CleanUpSubscribers() {
  std::unique_lock subscriber_lock(m_topic_subscribers_mutex);
  for (auto &[topic, original_subscribers] : m_topic_subscribers) {
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
  const auto &topic_name = event->GetTopic();

  std::unique_lock subscriber_lock(m_topic_subscribers_mutex);
  if (m_topic_subscribers.contains(topic_name)) {
    auto &subscribers_of_topic = m_topic_subscribers.at(topic_name);
    for (auto &subscriber : subscribers_of_topic) {
      if (!subscriber.expired()) {
        SharedJob message_job =
            std::make_shared<Job>([subscriber, event](JobContext *) {
              if (!subscriber.expired()) {
                subscriber.lock()->HandleMessage(event);
              }
              return JobContinuation::DISPOSE;
            });
        auto job_manager = m_subsystems.lock()->RequireSubsystem<JobManager>();
        job_manager->KickJob(message_job);
      }
    }

    LOG_DEBUG("message of topic '" << topic_name << "' published to "
                                   << subscribers_of_topic.size()
                                   << " subscribers")
  }
}

bool JobBasedEventBroker::HasListener(
    const std::string &subscriber_id, const std::string &topic) const {
  if (m_topic_subscribers.contains(topic)) {
    const auto &subscriber_list = m_topic_subscribers.at(topic);
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
  std::unique_lock subscriber_lock(m_topic_subscribers_mutex);
  if (!HasListener(listener.lock()->GetId(), topic)) {
    if (!m_topic_subscribers.contains(topic)) {
      std::vector<std::weak_ptr<IEventListener>> vec;
      m_topic_subscribers[topic] = vec;
    }
    m_topic_subscribers[topic].push_back(listener);
  }
}

void JobBasedEventBroker::UnregisterListener(
    std::weak_ptr<IEventListener> listener) {
  for (const auto &[topic, listener_set] : m_topic_subscribers) {
    RemoveListenerFromTopic(listener, topic);
  }
}

void JobBasedEventBroker::RemoveListenerFromTopic(
    std::weak_ptr<IEventListener> subscriber, const std::string &topic) {
  std::unique_lock subscriber_lock(m_topic_subscribers_mutex);
  if (m_topic_subscribers.contains(topic)) {
    auto &subscriber_list = m_topic_subscribers.at(topic);
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
  std::unique_lock subscriber_lock(m_topic_subscribers_mutex);
  m_topic_subscribers.clear();
}