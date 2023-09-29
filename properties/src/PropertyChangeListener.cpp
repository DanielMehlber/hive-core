#include "properties/PropertyChangeListener.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace props;

void PropertyChangeListener::HandleMessage(
    const messaging::SharedMessage message) {
  std::optional<std::string> optional_key =
      message->GetPayload<std::string>("property-key");

  if (optional_key.has_value()) {
    ProcessPropertyChange(optional_key.value());
  } else {
    LOG_ERR("property change notification received, but did not contain any "
            "property key. Ignored.");
  }
}

std::string props::PropertyChangeListener::GetId() const {
  return m_property_listener_id;
}

PropertyChangeListener::PropertyChangeListener()
    : m_property_listener_id{
          boost::uuids::to_string(boost::uuids::random_generator()())} {};

PropertyChangeListener::~PropertyChangeListener() {}