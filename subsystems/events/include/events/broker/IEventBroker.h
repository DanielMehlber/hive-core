#ifndef IMESSAGEBROKER_H
#define IMESSAGEBROKER_H

#include "events/Event.h"
#include "events/listener/IEventListener.h"
#include <memory>

namespace events {

class IEventBroker {
public:
  virtual ~IEventBroker() = default;
  /**
   * @brief Triggers an event and transfers it to all registered listeners.
   * @param event event that must be triggered
   */
  virtual void FireEvent(SharedEvent event) = 0;

  virtual bool HasListener(const std::string &listener_id,
                           const std::string &topic) const = 0;

  /**
   * @brief Add a listener for a specific topic of event. The listener will be
   * notified when this event occurs.
   * @param listener listener to add
   * @param topic topic of event the listener is interested in
   * @attention The broker does not own any listeners (see weak pointer)
   */
  virtual void RegisterListener(std::weak_ptr<IEventListener> listener,
                                const std::string &topic) = 0;

  /**
   * @brief Remove listener from all event types.
   * @param listener listener to remove
   */
  virtual void UnregisterListener(std::weak_ptr<IEventListener> listener) = 0;

  /**
   * @brief Remove listener from a specific topic of event.
   * @param listener listener to remove
   * @param topic topic the listener is no longer interested in
   */
  virtual void RemoveListenerFromTopic(std::weak_ptr<IEventListener> listener,
                                       const std::string &topic) = 0;

  /**
   * @brief Removes all listeners from the event manager
   */
  virtual void RemoveAllListeners() = 0;
};

typedef std::shared_ptr<IEventBroker> SharedEventBroker;

} // namespace events

#endif /* IMESSAGEBROKER_H */
