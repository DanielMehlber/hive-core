#include "messaging/broker/impl/JobBasedMessageBroker.h"

using namespace messaging;
using namespace messaging::impl;
using namespace std::chrono_literals;

JobBasedMessageBroker::JobBasedMessageBroker(
    std::shared_ptr<JobManager> job_manager)
    : m_job_manager{job_manager} {

  SharedJob clean_up_job = std::make_shared<TimerJob>(
      [&](JobContext *) {
        this->CleanUpSubscribers();
        return JobContinuation::REQUEUE;
      },
      "messaging-subscriber-clean-up", 5s, JobExecutionPhase::INIT);
  m_job_manager->KickJob(clean_up_job);
}

JobBasedMessageBroker::~JobBasedMessageBroker() {
  m_job_manager->DetachJob("messaging-subscriber-clean-up");
  RemoveAllSubscribers();
}

void JobBasedMessageBroker::CleanUpSubscribers() {
  for (auto &[topic, original_listeners] : m_topic_subscribers) {
    std::vector<std::weak_ptr<IMessageSubscriber>> new_listeners;
    for (auto &listener : original_listeners) {
      if (!listener.expired()) {
        new_listeners.push_back(listener);
      }
    }

    original_listeners.swap(new_listeners);
  }
}

void JobBasedMessageBroker::PublishMessage(SharedMessage event) {
  const auto &topic_name = event->GetTopic();
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
        m_job_manager->KickJob(message_job);
      }
    }
  }
}

bool messaging::impl::JobBasedMessageBroker::HasSubscriber(
    const std::string &listener_id, const std::string &topic) const {
  if (m_topic_subscribers.contains(topic)) {
    const auto &listener_list = m_topic_subscribers.at(topic);
    for (auto listener : listener_list) {
      if (!listener.expired() &&
          listener.lock()->GetId().compare(listener_id) == 0) {
        return true;
      }
    }
  }

  return false;
}

void JobBasedMessageBroker::AddSubscriber(
    std::weak_ptr<IMessageSubscriber> listener, const std::string &topic) {
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
    std::weak_ptr<IMessageSubscriber> listener, const std::string &topic) {
  if (m_topic_subscribers.contains(topic)) {
    auto &listener_set = m_topic_subscribers.at(topic);
    for (auto listener_iter = listener_set.begin();
         listener_iter != listener_set.end(); listener_iter++) {
      // always check if listener is still valid
      if ((*listener_iter).expired()) {
        listener_set.erase(listener_iter);
      } else if ((*listener_iter).lock()->GetId() == listener.lock()->GetId()) {
        listener_set.erase(listener_iter);
        break;
      }
    }
  }
}

void JobBasedMessageBroker::RemoveAllSubscribers() {
  m_topic_subscribers.clear();
}