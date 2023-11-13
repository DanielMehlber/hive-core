#ifndef FUNCTIONALMESSAGESUBSCRIBER_H
#define FUNCTIONALMESSAGESUBSCRIBER_H

#include "common/uuid/UuidGenerator.h"
#include "messaging/Message.h"
#include "messaging/Messaging.h"
#include "messaging/subscriber/IMessageSubscriber.h"
#include <functional>
#include <string>

namespace messaging {
class MESSAGING_API FunctionalMessageSubscriber : public IMessageSubscriber {
protected:
  const std::function<void(const SharedMessage)> m_function;
  const std::string m_id;

public:
  FunctionalMessageSubscriber() = delete;
  explicit FunctionalMessageSubscriber(
      const std::function<void(const SharedMessage)> &func)
      : m_function{func}, m_id{common::uuid::UuidGenerator::Random()} {};
  explicit FunctionalMessageSubscriber(
      const std::function<void(const SharedMessage)> &&func)
      : m_function{func}, m_id{common::uuid::UuidGenerator::Random()} {};

  FunctionalMessageSubscriber
  operator=(const std::function<void(const SharedMessage)> &func) {
    return FunctionalMessageSubscriber(func);
  };

  void HandleMessage(const SharedMessage event) override { m_function(event); }
  std::string GetId() const override { return m_id; };
};

} // namespace messaging

#endif /* FUNCTIONALMESSAGESUBSCRIBER_H */
