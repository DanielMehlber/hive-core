#ifndef IMESSAGESUBSCRIBER_H
#define IMESSAGESUBSCRIBER_H

#include "events/Event.h"

namespace events {

class IEventListener {
public:
  /**
   * @brief Handles incoming event of some type that this listener has
   * registered its interest in.
   * @param event event that must be handled
   */
  virtual void HandleMessage(SharedEvent event) = 0;

  /**
   * @brief Get id of this listener instance
   * @return id (preferably uuid of this listener)
   */
  virtual std::string GetId() const = 0;
};

typedef std::shared_ptr<IEventListener> SharedEventListener;

} // namespace events

#endif /* IMESSAGESUBSCRIBER_H */
