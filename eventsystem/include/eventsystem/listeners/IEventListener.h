#ifndef IEVENTLISTENER_H
#define IEVENTLISTENER_H

#include "eventsystem/Event.h"
#include <memory>
#include <set>

namespace eventsystem {

class IEventListener {
public:
  /**
   * @brief Handles incoming event of some type that this listener has
   * registered its interest in.
   * @param event event that must be handled
   */
  virtual void HandleEvent(const EventRef event) = 0;

  /**
   * @brief Get id of this listener instance
   * @return id (preferably uuid of this listener)
   */
  virtual const std::string &GetId() const = 0;
};
} // namespace eventsystem

#endif /* IEVENTLISTENER_H */
