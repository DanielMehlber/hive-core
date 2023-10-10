#ifndef PROPERTYFACTORY_H
#define PROPERTYFACTORY_H

#include "PropertyProvider.h"

namespace props {

/**
 * @brief Factory for all property-system related creation of instances.
 */
class PropertyFactory {
public:
  static SharedPropertyProvider
  CreatePropertyProvider(messaging::SharedBroker message_broker);
};

inline SharedPropertyProvider props::PropertyFactory::CreatePropertyProvider(
    messaging::SharedBroker message_broker) {
  return std::make_shared<PropertyProvider>(message_broker);
}

} // namespace props

#endif /* PROPERTYFACTORY_H */
