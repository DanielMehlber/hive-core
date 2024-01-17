#ifndef MESSAGEBROKERFACTORY_H
#define MESSAGEBROKERFACTORY_H

#include "events/broker/IEventBroker.h"
#include "events/broker/impl/JobBasedEventBroker.h"
#include "events/listener/IEventListener.h"
#include "events/listener/impl/FunctionalEventListener.h"

namespace events {

typedef brokers::JobBasedEventBroker DefaultBrokerImpl;
typedef FunctionalEventListener DefaultSubscriberImpl;

class EventFactory {
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
