#ifndef IMESSAGINGMANAGER_H
#define IMESSAGINGMANAGER_H

#include "messaging/Message.h"
#include "messaging/subscriber/IMessageSubscriber.h"
#include <memory>

namespace messaging {

class IMessagingManager {
public:
  virtual ~IMessagingManager() {}
  /**
   * @brief Triggers an event and transfers it to all registered listeners.
   * @param event event that must be triggered
   */
  virtual void PublishMessage(SharedMessage event) = 0;

  virtual bool HasSubscriber(const std::string &listener_id,
                             const std::string &type) const = 0;

  /**
   * @brief Add a listener for a specific type of event. The listener will be
   * notified when this event occurs.
   * @param listener listener to add
   * @param type type of event the listener is interested in
   */
  virtual void AddSubscriber(std::weak_ptr<IMessageSubscriber> listener,
                             const std::string &type) = 0;

  /**
   * @brief Remove listener from all event types.
   * @param listener listener to remove
   */
  virtual void RemoveSubscriber(std::weak_ptr<IMessageSubscriber> listener) = 0;

  /**
   * @brief Remove listener from a specific type of event.
   * @param listener listener to remove
   * @param type type the listener is no longer interested in
   */
  virtual void
  RemoveSubscriberFromTopic(std::weak_ptr<IMessageSubscriber> listener,
                            const std::string &type) = 0;

  /**
   * @brief Removes all listeners from the event manager
   */
  virtual void RemoveAllSubscribers() = 0;
};
} // namespace messaging

#endif /* IMESSAGINGMANAGER_H */
