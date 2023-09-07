#ifndef MESSAGEBROKERFACTORY_H
#define MESSAGEBROKERFACTORY_H

#include "messaging/broker/IMessageBroker.h"
#include "messaging/broker/impl/JobBasedMessageBroker.h"
#include "messaging/subscriber/IMessageSubscriber.h"
#include "messaging/subscriber/impl/FunctionalMessageSubscriber.h"

namespace messaging {

typedef impl::JobBasedMessageBroker DefaultBrokerImpl;
typedef FunctionalMessageSubscriber DefaultSubscriberImpl;

class MessagingFactory {
public:
  template <typename BrokerImpl = DefaultBrokerImpl, typename... Args>
  static SharedBroker CreateBroker(Args... arguments);

  template <typename SubscriberImpl = DefaultSubscriberImpl, typename... Args>
  static SharedSubscriber CreateSubscriber(Args... arguments);
};

template <typename BrokerImpl, typename... Args>
inline SharedBroker MessagingFactory::CreateBroker(Args... arguments) {
  return std::make_shared<BrokerImpl>(arguments...);
}

template <typename SubscriberImpl, typename... Args>
inline SharedSubscriber MessagingFactory::CreateSubscriber(Args... arguments) {
  return std::make_shared<SubscriberImpl>(arguments...);
}

} // namespace messaging

#endif /* MESSAGEBROKERFACTORY_H */
