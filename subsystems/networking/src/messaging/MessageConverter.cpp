#include "networking/messaging/MessageConverter.h"
#include "logging/LogManager.h"
#include "networking/util/MultipartFormdata.h"
#include <boost/json.hpp>

using namespace hive::networking::util;
using namespace hive::networking::messaging;
using namespace boost;

SharedMessage MessageConverter::FromJson(const std::string &json) {
  system::error_code err;
  json::value message_body = json::parse(json, err);

  if (err) {
    LOG_ERR("cannot parse JSON web-socket message due to syntax errors: "
            << err.message())
    THROW_EXCEPTION(
        MessagePayloadInvalidException,
        "cannot parse json message due to syntax errors: " << err.message())
  }

  if (message_body.is_object()) {
    // Extract 'id' and 'type' as strings
    std::string id = message_body.at("id").as_string().c_str();
    std::string type = message_body.at("type").as_string().c_str();

    auto parsed_message = std::make_shared<Message>(type, id);

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
    THROW_EXCEPTION(MessagePayloadInvalidException, "message is not an object")
  }
}

std::string MessageConverter::ToJson(const SharedMessage &message) {
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

std::string
MessageConverter::ToMultipartFormData(const SharedMessage &message) {

  util::Multipart multipart;

  // encode metadata of message (id, type, ...) in json format
  json::object message_meta;
  message_meta["id"] = message->GetId();
  message_meta["type"] = message->GetType();
  auto json_meta = json::serialize(message_meta);

  util::Part meta_part{"meta", json_meta};
  multipart.parts[meta_part.name] = meta_part;

  // attach all attributes as parts
  for (const auto &attribute_name : message->GetAttributeNames()) {
    util::Part attribute_part{
        attribute_name,
        std::move(message->GetAttribute(attribute_name).value())};

    multipart.parts[attribute_name] = std::move(attribute_part);
  }

  return generateMultipartFormData(multipart);
}

std::string extractPartContent(Multipart &multipart, const std::string &name) {
  if (multipart.parts.contains(name)) {
    auto part = multipart.parts.at(name);
    multipart.parts.erase(name);
    return part.content;
  } else {
    THROW_EXCEPTION(MessagePayloadInvalidException,
                    "required part " << name
                                     << " is missing in multipart message")
  }
}

SharedMessage
MessageConverter::FromMultipartFormData(const std::string &multipart_string) {

  auto multipart = util::parseMultipartFormData(multipart_string);

  SharedMessage parsed_message;

  // extract and parse json message meta information
  auto meta_part_content = extractPartContent(multipart, "meta");

  {
    system::error_code err;
    json::value message_body = json::parse(meta_part_content, err);

    if (err) {
      LOG_ERR("cannot parse message meta part (JSON) due to syntax errors: "
              << err.message())
      THROW_EXCEPTION(MessagePayloadInvalidException,
                      "cannot parse message meta part (JSON) to syntax errors: "
                          << err.message())
    }

    if (message_body.is_object()) {
      // Extract 'id' and 'type' as strings
      std::string id = message_body.at("id").as_string().c_str();
      std::string type = message_body.at("type").as_string().c_str();

      parsed_message = std::make_shared<Message>(type, id);
    }
  }

  // extract attributes
  for (auto &[attribute_name, attribute_part] : multipart.parts) {
    parsed_message->SetAttribute(attribute_name,
                                 std::move(attribute_part.content));
  }

  return parsed_message;
}
