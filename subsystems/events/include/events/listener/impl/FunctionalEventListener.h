#ifndef FUNCTIONALMESSAGESUBSCRIBER_H
#define FUNCTIONALMESSAGESUBSCRIBER_H

#include "common/uuid/UuidGenerator.h"
#include "events/Event.h"
#include "events/Eventing.h"
#include "events/listener/IEventListener.h"
#include <functional>
#include <string>

namespace events {
class EVENT_API FunctionalEventListener : public IEventListener {
protected:
  const std::function<void(const SharedEvent)> m_function;
  const std::string m_id;

public:
  FunctionalEventListener() = delete;
  explicit FunctionalEventListener(
      const std::function<void(const SharedEvent)> &func)
      : m_function{func}, m_id{common::uuid::UuidGenerator::Random()} {};
  explicit FunctionalEventListener(
      const std::function<void(const SharedEvent)> &&func)
      : m_function{func}, m_id{common::uuid::UuidGenerator::Random()} {};

  FunctionalEventListener
  operator=(const std::function<void(const SharedEvent)> &func) {
    return FunctionalEventListener(func);
  };

  void HandleMessage(const SharedEvent event) override { m_function(event); }
  std::string GetId() const override { return m_id; };
};

} // namespace events

#endif /* FUNCTIONALMESSAGESUBSCRIBER_H */
