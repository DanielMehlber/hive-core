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
  FunctionalMessageSubscriber(
      const std::function<void(const SharedMessage)> &func)
      : m_function{func},
        m_id{boost::uuids::to_string(boost::uuids::random_generator()())} {};
  FunctionalMessageSubscriber(
      const std::function<void(const SharedMessage)> &&func)
      : m_function{func},
        m_id{boost::uuids::to_string(boost::uuids::random_generator()())} {};

  FunctionalMessageSubscriber
  operator=(std::function<void(const SharedMessage)> func) {
    return FunctionalMessageSubscriber(func);
  };

  virtual void HandleMessage(const SharedMessage event) { m_function(event); }
  virtual const std::string &GetId() const override { return m_id; };
};

} // namespace messaging

#endif /* FUNCTIONALMESSAGESUBSCRIBER_H */
