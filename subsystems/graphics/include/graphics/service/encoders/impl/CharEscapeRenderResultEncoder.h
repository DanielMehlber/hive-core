#ifndef CHARESCAPERENDERRESULTENCODER_H
#define CHARESCAPERENDERRESULTENCODER_H

#include "graphics/service/encoders/IRenderResultEncoder.h"

namespace graphics {

class CharEscapeRenderResultEncoder : public IRenderResultEncoder {
  std::string GetName() override { return "char-escape"; };
  std::string Encode(const std::vector<unsigned char> &buffer);
  std::vector<unsigned char> Decode(const std::string &str);
};

} // namespace graphics

#endif // CHARESCAPERENDERRESULTENCODER_H
