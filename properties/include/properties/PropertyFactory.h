#ifndef PROPERTYFACTORY_H
#define PROPERTYFACTORY_H

#include "PropertyProvider.h"
#include "properties/Properties.h"

namespace props {

/**
 * @brief Factory for all property-system related creation of instances.
 */
class PROPERTIES_API PropertyFactory {
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
