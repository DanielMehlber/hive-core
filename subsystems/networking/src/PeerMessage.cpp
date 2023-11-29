#include "networking/websockets/PeerMessage.h"
#include "common/uuid/UuidGenerator.h"

using namespace networking::websockets;

PeerMessage::PeerMessage(std::string message_type)
    : m_type{std::move(message_type)},
      m_uuid{common::uuid::UuidGenerator::Random()} {}

PeerMessage::PeerMessage(std::string message_type, std::string id)
    : m_type{std::move(message_type)}, m_uuid{std::move(id)} {}

PeerMessage::~PeerMessage() = default;

void PeerMessage::SetAttribute(const std::string &attribute_key,
                               std::string attribute_value) {
  m_attributes[attribute_key] = std::move(attribute_value);
}

std::optional<std::string>
PeerMessage::GetAttribute(const std::string &attribute_key) {
  if (m_attributes.contains(attribute_key)) {
    return m_attributes.at(attribute_key);
  } else {
    return {};
  }
}

std::set<std::string> PeerMessage::GetAttributeNames() const noexcept {
  std::set<std::string> attribute_names;
  std::map<std::string, std::string>::const_iterator it;

  for (it = m_attributes.begin(); it != m_attributes.end(); it++) {
    attribute_names.insert(it->first);
  }

  return attribute_names;
}

bool PeerMessage::EqualsTo(std::shared_ptr<PeerMessage> other) const noexcept {
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