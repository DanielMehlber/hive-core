#include "messaging/broker/impl/JobBasedMessageBroker.h"

using namespace messaging;
using namespace messaging::impl;
using namespace std::chrono_literals;

JobBasedMessageBroker::JobBasedMessageBroker(
    const common::subsystems::SharedSubsystemManager &subsystems)
    : m_subsystems(subsystems) {

  SharedJob clean_up_job = std::make_shared<TimerJob>(
      [&](JobContext *) {
        this->CleanUpSubscribers();
        return JobContinuation::REQUEUE;
      },
      "messaging-subscriber-clean-up", 5s, JobExecutionPhase::INIT);

  auto job_manager = m_subsystems.lock()->RequireSubsystem<JobManager>();
  job_manager->KickJob(clean_up_job);
}

JobBasedMessageBroker::~JobBasedMessageBroker() {
  if (!m_subsystems.expired()) {
    auto job_manager = m_subsystems.lock()->RequireSubsystem<JobManager>();
    job_manager->DetachJob("messaging-subscriber-clean-up");
  }
  RemoveAllSubscribers();
}

void JobBasedMessageBroker::CleanUpSubscribers() {
  std::unique_lock subscriber_lock(m_topic_subscribers_mutex);
  for (auto &[topic, original_subscribers] : m_topic_subscribers) {
    std::vector<std::weak_ptr<IMessageSubscriber>> new_subscribers;
    for (auto &subscriber : original_subscribers) {
      if (!subscriber.expired()) {
        new_subscribers.push_back(subscriber);
      }
    }

    original_subscribers.swap(new_subscribers);
  }
}

void JobBasedMessageBroker::PublishMessage(SharedMessage event) {
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

bool messaging::impl::JobBasedMessageBroker::HasSubscriber(
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

void JobBasedMessageBroker::AddSubscriber(
    std::weak_ptr<IMessageSubscriber> listener, const std::string &topic) {
  std::unique_lock subscriber_lock(m_topic_subscribers_mutex);
  if (!HasSubscriber(listener.lock()->GetId(), topic)) {
    if (!m_topic_subscribers.contains(topic)) {
      std::vector<std::weak_ptr<IMessageSubscriber>> vec;
      m_topic_subscribers[topic] = vec;
    }
    m_topic_subscribers[topic].push_back(listener);
  }
}

void JobBasedMessageBroker::RemoveSubscriber(
    std::weak_ptr<IMessageSubscriber> listener) {
  for (const auto &[topic, listener_set] : m_topic_subscribers) {
    RemoveSubscriberFromTopic(listener, topic);
  }
}

void JobBasedMessageBroker::RemoveSubscriberFromTopic(
    std::weak_ptr<IMessageSubscriber> subscriber, const std::string &topic) {
  std::unique_lock subscriber_lock(m_topic_subscribers_mutex);
  if (m_topic_subscribers.contains(topic)) {
    auto &subscriber_list = m_topic_subscribers.at(topic);
    for (auto subscriber_iter = subscriber_list.begin();
         subscriber_iter != subscriber_list.end(); subscriber_iter++) {
      // always check if subscriber is still valid
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

void JobBasedMessageBroker::RemoveAllSubscribers() {
  std::unique_lock subscriber_lock(m_topic_subscribers_mutex);
  m_topic_subscribers.clear();
}