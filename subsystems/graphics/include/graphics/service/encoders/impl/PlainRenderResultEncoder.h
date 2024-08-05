#pragma once

#include "common/profiling/Timer.h"
#include "graphics/service/encoders/IRenderResultEncoder.h"

namespace hive::graphics {
class PlainRenderResultEncoder : public IRenderResultEncoder {
public:
  std::string GetName() override { return "plain"; }
  std::string Encode(const std::vector<unsigned char> &buffer) override;
  std::vector<unsigned char> Decode(const std::string &str) override;
};

inline std::string
PlainRenderResultEncoder::Encode(const std::vector<unsigned char> &buffer) {
#ifdef ENABLE_PROFILING
  common::profiling::Timer timer("plain-encode-" +
                                 std::to_string(buffer.size()));
#endif
  std::string result;
  result.reserve(buffer.size());
  result.assign(buffer.begin(), buffer.end());
  return result;
}

inline std::vector<unsigned char>
PlainRenderResultEncoder::Decode(const std::string &str) {
#ifdef ENABLE_PROFILING
  common::profiling::Timer timer("plain-decode-" +
                                 std::to_string(str.length()));
#endif
  return std::vector<unsigned char>(str.begin(), str.end());
}

} // namespace hive::graphics
