#ifndef FUNCTIONALMESSAGESUBSCRIBER_H
#define FUNCTIONALMESSAGESUBSCRIBER_H

#include "messaging/Message.h"
#include "messaging/subscriber/IMessageSubscriber.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <functional>
#include <string>

namespace messaging {
class FunctionalMessageSubscriber : public IMessageSubscriber {
protected:
  const std::function<void(const SharedMessage)> m_function;
  const std::string m_id;

public:
  FunctionalMessageSubscriber() = delete;
  explicit FunctionalMessageSubscriber(
      const std::function<void(const SharedMessage)> &func)
      : m_function{func},
        m_id{boost::uuids::to_string(boost::uuids::random_generator()())} {};
  explicit FunctionalMessageSubscriber(
      const std::function<void(const SharedMessage)> &&func)
      : m_function{func},
        m_id{boost::uuids::to_string(boost::uuids::random_generator()())} {};

  FunctionalMessageSubscriber
  operator=(const std::function<void(const SharedMessage)> &func) {
    return FunctionalMessageSubscriber(func);
  };

  void HandleMessage(const SharedMessage event) override { m_function(event); }
  std::string GetId() const override { return m_id; };
};

} // namespace messaging

#endif /* FUNCTIONALMESSAGESUBSCRIBER_H */
