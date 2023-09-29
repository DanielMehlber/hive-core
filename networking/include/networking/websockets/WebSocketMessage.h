#ifndef WEBSOCKETMESSAGE_H
#define WEBSOCKETMESSAGE_H

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

namespace networking::websockets {

/**
 * @brief Message that can be sent or received via web sockets. The message
 * always has a type and according attributes: This helps messages to be
 * multiplexed to different consumers that handle each message type. This also
 * renders messages extendable because new types with custom attributes can be
 * defined anytime.
 */
class WebSocketMessage {
protected:
  /**
   * @brief Type of this message
   */
  const std::string m_type;

  /**
   * @brief UUID of this message
   */
  const std::string m_uuid;

  /**
   * @brief Attributes containing the payload of this message
   */
  std::map<std::string, std::string> m_attributes;

public:
  WebSocketMessage(const std::string &message_type);
  WebSocketMessage(const std::string &message_type, const std::string &id);
  virtual ~WebSocketMessage();

  std::string GetId() const noexcept;

  std::string GetType() const noexcept;

  /**
   * @brief Sets or overwrites an attribute of this message.
   * @param attribute_key key of the attribute
   * @param attribute_value new value of the attribute
   */
  void SetAttribute(const std::string &attribute_key,
                    const std::string &attribute_value);

  /**
   * @brief Returns the value of an attribute (if it exists)
   * @param attribute_key key of the attribute
   * @return value of the attribute (if it exists)
   */
  std::optional<std::string> GetAttribute(const std::string &attribute_key);

  /**
   * @brief Returns a set of attribute names contained in the message
   * @return set of attribute names
   */
  std::set<std::string> GetAttributeNames() const noexcept;

  /**
   * @brief Compares this message with another message for equality.
   * @param other other message
   * @return true if both messages are equal in content
   */
  bool EqualsTo(std::shared_ptr<WebSocketMessage> other) const noexcept;
};

inline std::string WebSocketMessage::GetId() const noexcept { return m_uuid; }
inline std::string WebSocketMessage::GetType() const noexcept { return m_type; }

typedef std::shared_ptr<WebSocketMessage> SharedWebSocketMessage;

} // namespace networking::websockets

#endif /* WEBSOCKETMESSAGE_H */
