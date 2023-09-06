#ifndef FUNCTIONALEVENTLISTENER_H
#define FUNCTIONALEVENTLISTENER_H

#include "IEventListener.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <functional>
#include <string>

namespace eventsystem {
class FunctionalEventListener : public IEventListener {
protected:
  const std::function<void(const EventRef)> m_function;
  const std::string m_id;

public:
  FunctionalEventListener() = delete;
  FunctionalEventListener(const std::function<void(const EventRef)> &func)
      : m_function{func},
        m_id{boost::uuids::to_string(boost::uuids::random_generator()())} {};
  virtual void HandleEvent(const EventRef event) { m_function(event); }
  virtual const std::string &GetId() const override { return m_id; };
};
} // namespace eventsystem

#endif /* FUNCTIONALEVENTLISTENER_H */
