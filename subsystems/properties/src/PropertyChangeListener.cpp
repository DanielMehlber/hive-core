#include "properties/PropertyChangeListener.h"
#include "common/uuid/UuidGenerator.h"

using namespace props;

void PropertyChangeListener::HandleMessage(const events::SharedEvent message) {
  std::optional<std::string> maybe_property_key =
      message->GetPayload<std::string>("property-key");

  if (maybe_property_key.has_value()) {
    ProcessPropertyChange(maybe_property_key.value());
  } else {
    LOG_ERR("property change notification received, but did not contain any "
            "property key. Ignored.")
  }
}

std::string props::PropertyChangeListener::GetId() const {
  return m_property_listener_id;
}

PropertyChangeListener::PropertyChangeListener()
    : m_property_listener_id{common::uuid::UuidGenerator::Random()} {}

PropertyChangeListener::~PropertyChangeListener() = default;