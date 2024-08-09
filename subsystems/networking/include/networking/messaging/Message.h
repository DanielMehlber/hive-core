#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

namespace hive::networking::messaging {

/**
 * Message that can be passed between endpoints. It always has a type and
 * contains attributes: This helps messages to be redirected to their designated
 * consumers.
 */
class Message {
protected:
  /** Type of this message. */
  const std::string m_type;

  /** Unique id of this message. */
  const std::string m_uuid;

  /** Attributes containing the payload of this message. */
  std::map<std::string, std::string> m_attributes;

public:
  explicit Message(std::string message_type);
  Message(std::string message_type, std::string id);
  virtual ~Message();

  /**
   * @return unique id of this message
   */
  std::string GetId() const ;

  /**
   * @return type of this message
   * @note this attribute is used to find the according message consumer
   */
  std::string GetType() const ;

  /**
   * Sets or overwrites an attribute of this message.
   * @param attribute_key key of the attribute
   * @param attribute_value new value of the attribute
   */
  void SetAttribute(const std::string &attribute_key,
                    std::string attribute_value);

  /**
   * Returns the value of an attribute (if it exists)
   * @param attribute_key key of the attribute
   * @return value of the attribute (if it exists)
   */
  std::optional<std::string> GetAttribute(const std::string &attribute_key);

  /**
   * Returns a set of attribute names contained in the message
   * @return set of attribute names
   */
  std::set<std::string> GetAttributeNames() const ;

  /**
   * Compares this message with another message for equality.
   * @param other other message
   * @return true if both events are equal in content
   */
  bool EqualsTo(const std::shared_ptr<Message> &other) const ;
};

inline std::string Message::GetId() const  { return m_uuid; }
inline std::string Message::GetType() const  { return m_type; }

typedef std::shared_ptr<Message> SharedMessage;

} // namespace hive::networking::messaging
