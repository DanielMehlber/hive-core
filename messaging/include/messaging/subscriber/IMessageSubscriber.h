#ifndef IMESSAGESUBSCRIBER_H
#define IMESSAGESUBSCRIBER_H

#include "messaging/Message.h"

namespace messaging {

class IMessageSubscriber {
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
  virtual const std::string &GetId() const = 0;
};
} // namespace messaging

#endif /* IMESSAGESUBSCRIBER_H */
