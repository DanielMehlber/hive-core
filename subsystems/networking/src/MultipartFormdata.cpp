#include "networking/util/MultipartFormdata.h"
#include "common/uuid/UuidGenerator.h"

#define BOUNDARY "boundary-4f20310a-8ea0-4fa7-aebb-1d8bf9e58f66"

using namespace networking::util;

std::string
networking::util::generateMultipartFormData(const Multipart &multipart) {
  std::stringstream ss;

  for (const auto &[name, part] : multipart.parts) {
    ss << "--" << BOUNDARY << "\r\n";
    ss << "Content-Disposition: form-data; name=\"" << name << "\"\r\n\r\n";
    ss << part.content << "\r\n";
  }

  ss << "--" << BOUNDARY << "--";

  return ss.str();
}

void parseFormPart(const std::string &part_str, Multipart &result) {
  size_t headerEnd = part_str.find("\r\n\r\n");
  if (headerEnd == std::string::npos) {
    return;
  }

  std::string header = part_str.substr(0, headerEnd);
  size_t namePos = header.find("name=\"");
  if (namePos == std::string::npos) {
    return;
  }

  size_t nameStart = namePos + 6;
  size_t nameEnd = header.find("\"", nameStart);
  if (nameEnd == std::string::npos) {
    return;
  }

  std::string name = header.substr(nameStart, nameEnd - nameStart);
  std::string value = part_str.substr(headerEnd + 4, std::string::npos);

  Part part{name, std::move(value)};
  result.parts[name] = std::move(part);
}

Multipart networking::util::parseMultipartFormData(const std::string &data) {
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
    std::string part = data.substr(pos, endPos - pos);
    parseFormPart(part, result);

    pos = endPos;
  }

  return result;
}
