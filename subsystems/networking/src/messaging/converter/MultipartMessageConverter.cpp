#include "networking/messaging/converter/MultipartMessageConverter.h"
#include "logging/LogManager.h"
#include <boost/json.hpp>
#include <sstream>

using namespace hive::networking::messaging;
using namespace boost;

#define BOUNDARY "boundary-4f20310a-8ea0-4fa7-aebb-1d8bf9e58f66"

struct Part {
  std::string name;
  std::string content;
};

struct Multipart {
  std::map<std::string, Part> parts;
};

std::string serializeMultipart(const Multipart &multipart) {
  std::stringstream ss;

  for (const auto &[name, part] : multipart.parts) {
    ss << "--" << BOUNDARY << "\r\n";
    ss << "Content-Disposition: form-data; name=\"" << name << "\"\r\n\r\n";
    ss << part.content << "\r\n";
  }

  ss << "--" << BOUNDARY << "--";

  return ss.str();
}

std::optional<Part> parsePart(const std::string &part_str) {
  size_t headerEnd = part_str.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    LOG_ERR("skipped part while parsing multipart/form-data: no header found")
    return {};
  }

  std::string header = part_str.substr(0, headerEnd);
  size_t namePos = header.find("name=\"");
  if (namePos == std::string::npos) {
    LOG_ERR(
        "skipped part while parsing multipart/form-data: no part name provided")
    return {};
  }

  size_t nameStart = namePos + 6;
  size_t nameEnd = header.find("\"", nameStart);
  if (nameEnd == std::string::npos) {
    LOG_ERR(
        "skipped part while parsing multipart/form-data: no part name provided")
    return {};
  }

  std::string name = header.substr(nameStart, nameEnd - nameStart);
  std::string value = part_str.substr(headerEnd + 4, std::string::npos);

  Part part{name, std::move(value)};
  return part;
}

Multipart parseMultipart(const std::string &data) {
  Multipart result;

  std::string boundary = BOUNDARY;

  // Split the formData into parts using the boundary
  size_t pos = 0;
  while ((pos = data.find("--" + boundary, pos)) != std::string::npos) {
    pos += 2 + boundary.length(); // Move past the boundary

    // Find the end of this part
    size_t endPos = data.find("\r\n--" + boundary, pos);
    if (endPos == std::string::npos) {
      break;
    }

    // Extract the part and add it to the result
    std::string part_str = data.substr(pos, endPos - pos);
    auto maybe_part = parsePart(part_str);

    if (maybe_part.has_value()) {
      Part part = maybe_part.value();
      result.parts[part.name] = std::move(part);
    }

    pos = endPos;
  }

  return result;
}

SharedMessage
MultipartMessageConverter::Deserialize(const std::string &multipart_string) {

  auto multipart = parseMultipart(multipart_string);

  SharedMessage parsed_message;

  // extract and parse json message meta information
  if (!multipart.parts.contains("msg-meta")) {
    LOG_ERR("cannot parse multipart/form-data message: no meta part found")
    THROW_EXCEPTION(
        MessagePayloadInvalidException,
        "cannot parse multipart/form-data message: no meta part found")
  }

  const std::string &meta_part_content = multipart.parts.at("msg-meta").content;

  {
    system::error_code err;
    json::value message_body = json::parse(meta_part_content, err);

    if (err) {
      LOG_ERR("cannot parse meta part of multipart/form-data message due to "
              "syntax errors: "
              << err.message())
      THROW_EXCEPTION(
          MessagePayloadInvalidException,
          "cannot parse meta part of multipart/form-data message due to "
          "syntax errors: "
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
    // skip msg-meta part
    if (attribute_name == "msg-meta") {
      continue;
    }

    parsed_message->SetAttribute(attribute_name,
                                 std::move(attribute_part.content));
  }

  return parsed_message;
}

std::string MultipartMessageConverter::Serialize(const SharedMessage &message) {

  Multipart multipart;

  // encode metadata of message (id, type, ...) in json format
  json::object message_meta;
  message_meta["id"] = message->GetId();
  message_meta["type"] = message->GetType();
  auto json_meta = json::serialize(message_meta);

  Part meta_part{"msg-meta", json_meta};
  multipart.parts[meta_part.name] = meta_part;

  // attach all attributes as parts
  for (const auto &attribute_name : message->GetAttributeNames()) {
    Part attribute_part{
        attribute_name,
        std::move(message->GetAttribute(attribute_name).value())};

    multipart.parts[attribute_name] = std::move(attribute_part);
  }

  return serializeMultipart(multipart);
}
