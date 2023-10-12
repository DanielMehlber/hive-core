#include "networking/websockets/WebSocketMessage.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace networking::websockets;

WebSocketMessage::WebSocketMessage(const std::string &message_type)
    : m_type{message_type},
      m_uuid{boost::uuids::to_string(boost::uuids::random_generator()())} {}

WebSocketMessage::WebSocketMessage(const std::string &message_type,
                                   const std::string &id)
    : m_type{message_type}, m_uuid{id} {}

WebSocketMessage::~WebSocketMessage() = default;

void WebSocketMessage::SetAttribute(const std::string &attribute_key,
                                    const std::string &attribute_value) {
  m_attributes[attribute_key] = attribute_value;
}

std::optional<std::string>
WebSocketMessage::GetAttribute(const std::string &attribute_key) {
  if (m_attributes.contains(attribute_key)) {
    return m_attributes.at(attribute_key);
  } else {
    return {};
  }
}

std::set<std::string> WebSocketMessage::GetAttributeNames() const noexcept {
  std::set<std::string> attribute_names;
  std::map<std::string, std::string>::const_iterator it;

  for (it = m_attributes.begin(); it != m_attributes.end(); it++) {
    attribute_names.insert(it->first);
  }

  return attribute_names;
}

bool WebSocketMessage::EqualsTo(
    std::shared_ptr<WebSocketMessage> other) const noexcept {
  if (m_uuid != other->m_uuid) {
    return false;
  } else if (m_type != other->m_type) {
    return false;
  } else if (m_attributes.size() != other->m_attributes.size()) {
    return false;
  }

  std::map<std::string, std::string>::const_iterator it;
  for (it = m_attributes.begin(); it != m_attributes.end(); it++) {
    const std::string &name = it->first;
    const std::string &value = it->second;
    if (!other->m_attributes.contains(name)) {
      return false;
    }

    if (value != other->m_attributes.at(name)) {
      return false;
    }
  }

  return true;
}