#include "logging/LogManager.h"
#include "networking/peers/PeerMessageConverter.h"
#include <boost/json.hpp>

using namespace networking::websockets;
using namespace boost;

SharedWebSocketMessage networking::websockets::PeerMessageConverter::FromJson(
    const std::string &json) {
  json::error_code err;
  json::value message_body = boost::json::parse(json, err);

  if (err) {
    LOG_ERR("cannot parse JSON web-socket message due to syntax errors: "
            << err.message());
    THROW_EXCEPTION(
        MessagePayloadInvalidException,
        "cannot parse json message due to syntax errors: " << err.message());
  }

  if (message_body.is_object()) {
    // Extract 'id' and 'type' as strings
    std::string id = message_body.at("id").as_string().c_str();
    std::string type = message_body.at("type").as_string().c_str();

    SharedWebSocketMessage parsed_message =
        std::make_shared<PeerMessage>(type, id);

    // Extract 'attributes' as a map
    std::map<std::string, std::string> attributes;
    json::object attrObj = message_body.at("attributes").as_object();
    for (auto &it : attrObj) {
      std::string key = it.key();
      std::string value = it.value().as_string().c_str();
      parsed_message->SetAttribute(key, value);
    }

    return parsed_message;
  } else {
    THROW_EXCEPTION(MessagePayloadInvalidException, "message is not an object");
  }
}

std::string
PeerMessageConverter::ToJson(const SharedWebSocketMessage &message) {
  auto id = message->GetId();
  json::object message_body;
  message_body["id"] = message->GetId();
  message_body["type"] = message->GetType();

  json::object attributes;
  auto attribute_set = message->GetAttributeNames();
  for (const std::string &attribute_name : attribute_set) {
    attributes[attribute_name] =
        message->GetAttribute(attribute_name).value_or("");
  }

  message_body["attributes"] = attributes;

  return json::serialize(message_body);
}