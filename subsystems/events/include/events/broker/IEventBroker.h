#ifndef IMESSAGEBROKER_H
#define IMESSAGEBROKER_H

#include "events/Event.h"
#include "events/listener/IEventListener.h"
#include <memory>

namespace events {

/**
 * Manages events and propagates them from emitters to event listeners.
 */
class IEventBroker {
public:
  virtual ~IEventBroker() = default;
  /**
   * Triggers an event and transfers it to all registered listeners.
   * @param event event that must be triggered
   */
  virtual void FireEvent(SharedEvent event) = 0;

  /**
   * Checks if the broker has registered a listener with an id for a certain
   * topic.
   * @param listener_id id of listener.
   * @param topic topic the listener should listen to.
   * @return true if there is a listener with requested topic and id.
   */
  virtual bool HasListener(const std::string &listener_id,
                           const std::string &topic) const = 0;

  /**
   * Add a listener for a specific topic of event. The listener will be
   * notified when this event occurs.
   * @param listener listener to add
   * @param topic topic of event the listener is interested in
   * @attention The broker does not own any listeners (see weak pointer)
   */
  virtual void RegisterListener(std::weak_ptr<IEventListener> listener,
                                const std::string &topic) = 0;

  /**
   * Remove listener from all event types.
   * @param listener listener to remove
   */
  virtual void UnregisterListener(std::weak_ptr<IEventListener> listener) = 0;

  /**
   * Remove listener from a specific topic of event.
   * @param listener listener to remove
   * @param topic topic the listener is no longer interested in
   */
  virtual void RemoveListenerFromTopic(std::weak_ptr<IEventListener> listener,
                                       const std::string &topic) = 0;

  /**
   * Removes all listeners from the event manager
   */
  virtual void RemoveAllListeners() = 0;
};

typedef std::shared_ptr<IEventBroker> SharedEventBroker;

} // namespace events

#endif /* IMESSAGEBROKER_H */
