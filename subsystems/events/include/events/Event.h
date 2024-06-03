#pragma once

#include "common/uuid/UuidGenerator.h"
#include <any>
#include <map>
#include <memory>
#include <optional>

namespace events {

/**
 * Encapsulates an event, its meta-data, and payload.
 */
class Event {
protected:
  /**
   * Event topic name used to differentiate different kinds of events
   * and direct them to their listener.
   * @note This string will be compared often
   */
  const std::string topic;

  /**
   * Payload of the thrown event that can be used by listeners to perform
   * actions based on context-information of the event.
   */
  std::map<std::string, std::any> m_payload;

  /** Unique id of this event instance.*/
  std::string m_id;

public:
  Event() = delete;
  explicit Event(std::string topic)
      : topic{std::move(topic)}, m_id{common::uuid::UuidGenerator::Random()} {};
  virtual ~Event() = default;

  std::string GetId() const noexcept;
  template <typename T> void SetPayload(const std::string &key, T value);
  template <typename T>
  std::optional<T> GetPayload(const std::string &key) const;
  const std::string &GetTopic() const;
};

inline std::string Event::GetId() const noexcept { return m_id; }

template <typename T>
inline void Event::SetPayload(const std::string &key, T value) {
  m_payload[key] = value;
}

template <typename T>
inline std::optional<T> Event::GetPayload(const std::string &key) const {
  if (m_payload.contains(key)) {
    auto &any_container = m_payload.at(key);
    return std::any_cast<T>(any_container);
  } else {
    return {};
  }
}

inline const std::string &Event::GetTopic() const { return topic; }

typedef std::shared_ptr<Event> SharedEvent;
} // namespace events
