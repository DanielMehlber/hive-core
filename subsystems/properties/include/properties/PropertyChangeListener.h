#ifndef PROPERTYCHANGELISTENER_H
#define PROPERTYCHANGELISTENER_H

#include "logging/LogManager.h"
#include "messaging/subscriber/IMessageSubscriber.h"
#include "properties/Properties.h"
#include <memory>

namespace props {

/**
 * @brief Listens to changes of a specific property or a subset of properties
 * based on their path in the property tree. Under the hood, updating properties
 * emits messages to topics that instances of this class subscribed. Therefore,
 * the property system is founded on the messaging system and makes heavy use of
 * it.
 */
class PROPERTIES_API PropertyChangeListener
    : public messaging::IMessageSubscriber {
protected:
  const std::string m_property_listener_id;

  void HandleMessage(messaging::SharedMessage message) override;
  std::string GetId() const override;

public:
  PropertyChangeListener();
  virtual ~PropertyChangeListener();

  /**
   * @brief Called when the listened to property or other subsequent properties
   * have changed. Each listener can implement its own handling operations.
   * @param key absolute path in the property tree of the property that has been
   * changed.
   */
  virtual void ProcessPropertyChange(const std::string &key) noexcept = 0;
};

typedef std::shared_ptr<PropertyChangeListener> SharedPropertyListener;

} // namespace props

#endif /* PROPERTYCHANGELISTENER_H */
