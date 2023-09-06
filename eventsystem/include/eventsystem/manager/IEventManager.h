#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "../Event.h"
#include "../EventContext.h"
#include "eventsystem/listeners/IEventListener.h"
#include <memory>

namespace eventsystem {
class IEventManager {
public:
  virtual ~IEventManager() {}
  /**
   * @brief Triggers an event and transfers it to all registered listeners.
   * @param event event that must be triggered
   */
  virtual void FireEvent(EventRef event) = 0;

  virtual bool HasListener(const std::string &listener_id,
                           const std::string &type) const = 0;

  /**
   * @brief Add a listener for a specific type of event. The listener will be
   * notified when this event occurs.
   * @param listener listener to add
   * @param type type of event the listener is interested in
   */
  virtual void AddListener(std::weak_ptr<IEventListener> listener,
                           const std::string &type) = 0;

  /**
   * @brief Remove listener from all event types.
   * @param listener listener to remove
   */
  virtual void RemoveListener(std::weak_ptr<IEventListener> listener) = 0;

  /**
   * @brief Remove listener from a specific type of event.
   * @param listener listener to remove
   * @param type type the listener is no longer interested in
   */
  virtual void RemoveListenerFromType(std::weak_ptr<IEventListener> listener,
                                      const std::string &type) = 0;

  /**
   * @brief Removes all listeners from the event manager
   */
  virtual void RemoveAllListeners() = 0;
};
} // namespace eventsystem

#endif /* EVENTMANAGER_H */
