#ifndef EVENTCONTEXT_H
#define EVENTCONTEXT_H

#include <any>
#include <chrono>
#include <map>
#include <optional>
#include <string>

namespace eventsystem {
class EventContext {
private:
  /**
   * @brief Payload of the thrown event that can be used by listeners to perform
   * actions based on context-information of the event.
   */
  std::map<std::string, std::any> m_payload;

public:
  template <typename T> void SetPayload(const std::string &key, T value);
  template <typename T>
  std::optional<T> GetPayload(const std::string &key) const;
};

template <typename T>
inline void EventContext::SetPayload(const std::string &key, T value) {
  m_payload[key] = value;
}

template <typename T>
inline std::optional<T> EventContext::GetPayload(const std::string &key) const {
  if (m_payload.contains(key)) {
    auto any_container = m_payload.at(key);
    return std::any_cast<T>(any_container);
  } else {
    {};
  }
}
} // namespace eventsystem

#endif /* EVENTCONTEXT_H */
