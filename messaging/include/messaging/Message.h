#ifndef MESSAGE_H
#define MESSAGE_H

#include "messaging/Messaging.h"
#include <any>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <map>
#include <memory>
#include <optional>

using namespace boost::uuids;

namespace messaging {
class MESSAGING_API Message {
protected:
  /**
   * @brief Message topic name used to differentiate different kinds of messages
   * and direct them to their subscriber.
   * @note This string will be compared often
   */
  const std::string topic;

  /**
   * @brief Payload of the thrown event that can be used by listeners to perform
   * actions based on context-information of the event.
   */
  std::map<std::string, std::any> m_payload;

  /**
   * @brief Unique id of this event instance.
   */
  uuid m_id;

public:
  Message() = delete;
  explicit Message(std::string topic)
      : topic{std::move(topic)}, m_id{boost::uuids::random_generator()()} {};
  virtual ~Message() = default;

  uuid GetId() const noexcept;
  template <typename T> void SetPayload(const std::string &key, T value);
  template <typename T>
  std::optional<T> GetPayload(const std::string &key) const;
  const std::string &GetTopic() const;
};

inline uuid Message::GetId() const noexcept { return m_id; }
template <typename T>
inline void Message::SetPayload(const std::string &key, T value) {
  m_payload[key] = value;
}

template <typename T>
inline std::optional<T> Message::GetPayload(const std::string &key) const {
  if (m_payload.contains(key)) {
    auto any_container = m_payload.at(key);
    return std::any_cast<T>(any_container);
  } else {
    return {};
  }
}

inline const std::string &Message::GetTopic() const { return topic; }

typedef std::shared_ptr<Message> SharedMessage;
} // namespace messaging

#endif /* MESSAGE_H */
