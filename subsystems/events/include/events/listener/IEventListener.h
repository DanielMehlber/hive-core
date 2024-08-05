#pragma once

#include "events/Event.h"

namespace hive::events {

/**
 * Handles events propagated and managed by an event broker.
 */
class IEventListener {
public:
  /**
   * Handles incoming event of some type that this listener has
   * registered its interest in.
   * @param event event that must be handled
   */
  virtual void HandleEvent(SharedEvent event) = 0;

  /**
   * GetAsInt id of this listener instance
   * @return id (preferably uuid of this listener)
   */
  virtual std::string GetId() const = 0;
};

typedef std::shared_ptr<IEventListener> SharedEventListener;

} // namespace hive::events
