#pragma once

#include <string>
#include <vector>

namespace hive::graphics {
class IRenderResultEncoder {
public:
  virtual std::string GetName() = 0;
  virtual std::string Encode(const std::vector<unsigned char> &buffer) = 0;
  virtual std::vector<unsigned char> Decode(const std::string &str) = 0;
  virtual ~IRenderResultEncoder() = default;
};
} // namespace hive::graphics
