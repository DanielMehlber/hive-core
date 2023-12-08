#ifndef MESSAGEBROKERFACTORY_H
#define MESSAGEBROKERFACTORY_H

#include "events/Eventing.h"
#include "events/broker/IEventBroker.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "events/listener/IEventListener.h"
#include "events/listener/impl/FunctionalEventListener.h"

namespace events {

typedef impl::JobBasedEventBroker DefaultBrokerImpl;
typedef FunctionalEventListener DefaultSubscriberImpl;

class EVENT_API EventFactory {
public:
  template <typename BrokerImpl = DefaultBrokerImpl, typename... Args>
  static SharedEventBroker CreateBroker(Args... arguments);
};

template <typename BrokerImpl, typename... Args>
inline SharedEventBroker EventFactory::CreateBroker(Args... arguments) {
  return std::make_shared<BrokerImpl>(arguments...);
}

} // namespace events

#endif /* MESSAGEBROKERFACTORY_H */
