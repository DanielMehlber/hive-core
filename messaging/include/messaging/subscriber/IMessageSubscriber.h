#ifndef IMESSAGESUBSCRIBER_H
#define IMESSAGESUBSCRIBER_H

#include "messaging/Message.h"

namespace messaging {

class IMessageSubscriber
    : public std::enable_shared_from_this<IMessageSubscriber> {
public:
  /**
   * @brief Handles incoming event of some type that this listener has
   * registered its interest in.
   * @param event event that must be handled
   */
  virtual void HandleMessage(const SharedMessage event) = 0;

  /**
   * @brief Get id of this listener instance
   * @return id (preferably uuid of this listener)
   */
  virtual std::string GetId() const = 0;
};

typedef std::shared_ptr<IMessageSubscriber> SharedSubscriber;

} // namespace messaging

#endif /* IMESSAGESUBSCRIBER_H */
