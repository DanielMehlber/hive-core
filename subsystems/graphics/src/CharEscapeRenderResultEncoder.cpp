#include "graphics/service/encoders/impl/CharEscapeRenderResultEncoder.h"

using namespace graphics;

std::string CharEscapeRenderResultEncoder::Encode(
    const std::vector<unsigned char> &buffer) {
  std::string str(buffer.begin(), buffer.end());

  // escape some special chars by incrementing them
  for (char &i : str) {
    switch (i) {
    case '\"':
    case '\'':
    case '\n':
    case '\r':
      i = i + ((char)1);
      break;
    default:
      break;
    }
  }

  return str;
}

std::vector<unsigned char>
CharEscapeRenderResultEncoder::Decode(const std::string &str) {
  return std::vector<unsigned char>(str.begin(), str.end());
}
