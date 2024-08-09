#pragma once

#include "events/listener/IEventListener.h"
#include "logging/LogManager.h"
#include <memory>

namespace hive::data {

/**
 * Listens to changes of a specific property or a subset of properties
 * based on their path in the property tree. Under the hood, updating properties
 * emits events to topics that instances of this class subscribed. Therefore,
 * the property system is founded on the events system and makes heavy use of
 * it.
 */
class PropertyChangeListener : public events::IEventListener {
protected:
  const std::string m_property_listener_id;

  void HandleEvent(events::SharedEvent message) override;
  std::string GetId() const override;

public:
  PropertyChangeListener();
  virtual ~PropertyChangeListener();

  /**
   * Called when the listened to property or other subsequent properties
   * have changed. Each listener can implement its own handling operations.
   * @param key absolute path in the property tree of the property that has been
   * changed.
   */
  virtual void ProcessPropertyChange(const std::string &key)  = 0;
};

typedef std::shared_ptr<PropertyChangeListener> SharedPropertyListener;

} // namespace hive::data
