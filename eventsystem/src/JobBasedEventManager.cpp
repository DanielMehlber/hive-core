#include "eventsystem/manager/impl/JobBasedEventManager.h"

using namespace eventsystem;
using namespace eventsystem::impl;

JobBasedEventManager::JobBasedEventManager(
    std::shared_ptr<JobManager> job_manager)
    : m_job_manager{job_manager} {}

JobBasedEventManager::~JobBasedEventManager() { RemoveAllListeners(); }

void JobBasedEventManager::CleanUpListeners() {
  for (auto &[type, original_listeners] : m_listeners) {
    std::vector<std::weak_ptr<IEventListener>> new_listeners;
    for (auto listener : original_listeners) {
      if (!listener.expired()) {
        new_listeners.push_back(listener);
      }
    }

    original_listeners.swap(new_listeners);
  }
}

void JobBasedEventManager::FireEvent(EventRef event) {}

bool eventsystem::impl::JobBasedEventManager::HasListener(
    const std::string &listener_id, const std::string &type) const {
  if (m_listeners.contains(type)) {
    const auto listener_list = m_listeners.at(type);
    for (auto listener : listener_list) {
      if (!listener.expired() && listener.lock()->GetId() == listener_id) {
        return true;
      }
    }
  }

  return false;
}

void JobBasedEventManager::AddListener(std::weak_ptr<IEventListener> listener,
                                       const std::string &type) {
  if (!HasListener(listener.lock()->GetId(), type)) {
    if (m_listeners.contains(type)) {
      auto listener_set = m_listeners[type];
      listener_set.push_back(listener);
    }
  }
}

void JobBasedEventManager::RemoveListener(
    std::weak_ptr<IEventListener> listener) {
  for (const auto &[type, listener_set] : m_listeners) {
    RemoveListenerFromType(listener, type);
  }
}

void JobBasedEventManager::RemoveListenerFromType(
    std::weak_ptr<IEventListener> listener, const std::string &type) {
  if (m_listeners.contains(type)) {
    auto listener_set = m_listeners[type];
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

void JobBasedEventManager::RemoveAllListeners() { m_listeners.clear(); }