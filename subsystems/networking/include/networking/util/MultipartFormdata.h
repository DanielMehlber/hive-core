#pragma once

#include "common/exceptions/ExceptionsBase.h"
#include <map>
#include <string>

namespace networking::util {

DECLARE_EXCEPTION(InvalidMultipartException);

struct Part {
  std::string name;
  std::string content;
};

struct Multipart {
  std::map<std::string, Part> parts;
};

std::string generateMultipartFormData(const Multipart &multipart);

Multipart parseMultipartFormData(const std::string &data);

} // namespace networking::util
