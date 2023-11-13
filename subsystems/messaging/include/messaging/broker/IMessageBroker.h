#ifndef IMESSAGEBROKER_H
#define IMESSAGEBROKER_H

#include "messaging/Message.h"
#include "messaging/subscriber/IMessageSubscriber.h"
#include <memory>

namespace messaging {

class IMessageBroker {
public:
  virtual ~IMessageBroker() = default;
  /**
   * @brief Triggers an event and transfers it to all registered listeners.
   * @param event event that must be triggered
   */
  virtual void PublishMessage(SharedMessage event) = 0;

  virtual bool HasSubscriber(const std::string &listener_id,
                             const std::string &topic) const = 0;

  /**
   * @brief Add a listener for a specific topic of event. The listener will be
   * notified when this event occurs.
   * @param listener listener to add
   * @param topic topic of event the listener is interested in
   */
  virtual void AddSubscriber(std::weak_ptr<IMessageSubscriber> listener,
                             const std::string &topic) = 0;

  /**
   * @brief Remove listener from all event types.
   * @param listener listener to remove
   */
  virtual void RemoveSubscriber(std::weak_ptr<IMessageSubscriber> listener) = 0;

  /**
   * @brief Remove listener from a specific topic of event.
   * @param listener listener to remove
   * @param topic topic the listener is no longer interested in
   */
  virtual void
  RemoveSubscriberFromTopic(std::weak_ptr<IMessageSubscriber> listener,
                            const std::string &topic) = 0;

  /**
   * @brief Removes all listeners from the event manager
   */
  virtual void RemoveAllSubscribers() = 0;
};

typedef std::shared_ptr<IMessageBroker> SharedBroker;

} // namespace messaging

#endif /* IMESSAGEBROKER_H */
