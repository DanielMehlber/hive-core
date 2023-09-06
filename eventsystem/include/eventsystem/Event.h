#ifndef EVENT_H
#define EVENT_H

#include "EventContext.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <memory>

using namespace boost::uuids;

namespace eventsystem {
class Event {
protected:
  /**
   * @brief Event type used to differentiate different kinds of events and
   * direct them to their listener.
   * @note This string will be compared often
   */
  const std::string m_type;

  /**
   * @brief Context information that will be passed to the listener.
   */
  EventContext m_context;

  /**
   * @brief Unique id of this event instance.
   */
  uuid m_id;

public:
  Event() = delete;
  Event(const std::string &type, EventContext &context)
      : m_type{type}, m_context{context},
        m_id{boost::uuids::random_generator()()} {};
  virtual ~Event(){};

  uuid GetId() const noexcept;
  EventContext &GetContext() noexcept;
};

inline uuid Event::GetId() const noexcept { return m_id; }
inline EventContext &eventsystem::Event::GetContext() noexcept {
  return m_context;
}
} // namespace eventsystem

#endif /* EVENT_H */

typedef std::shared_ptr<eventsystem::Event> EventRef;
