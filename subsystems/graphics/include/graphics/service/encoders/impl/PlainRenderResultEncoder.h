#ifndef PLAINRENDERRESULTENCODER_H
#define PLAINRENDERRESULTENCODER_H

#include "graphics/service/encoders/IRenderResultEncoder.h"

namespace graphics {
class PlainRenderResultEncoder : public IRenderResultEncoder {
public:
  std::string GetName() override { return "plain"; }
  std::string Encode(const std::vector<unsigned char> &buffer) override;
  std::vector<unsigned char> Decode(const std::string &str) override;
};

inline std::string
PlainRenderResultEncoder::Encode(const std::vector<unsigned char> &buffer) {
  return std::string(buffer.begin(), buffer.end());
}

inline std::vector<unsigned char>
PlainRenderResultEncoder::Decode(const std::string &str) {
  return std::vector<unsigned char>(str.begin(), str.end());
}

} // namespace graphics

#endif // PLAINRENDERRESULTENCODER_H
