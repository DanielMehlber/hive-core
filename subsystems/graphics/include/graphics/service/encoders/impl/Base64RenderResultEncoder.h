#pragma once

#include "common/encoding/Base64.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"

namespace graphics {
class Base64RenderResultEncoder : public IRenderResultEncoder {
public:
  std::string GetName() override { return "base-64"; }
  std::string Encode(const std::vector<unsigned char> &buffer) override;
  std::vector<unsigned char> Decode(const std::string &str) override;
};

inline std::string
Base64RenderResultEncoder::Encode(const std::vector<unsigned char> &buffer) {
  return common::coding::base64_encode(buffer);
}

inline std::vector<unsigned char>
Base64RenderResultEncoder::Decode(const std::string &str) {
  return common::coding::base64_decode(str);
}

} // namespace graphics
