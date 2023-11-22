#include "graphics/service/encoders/impl/CharEscapeRenderResultEncoder.h"

using namespace graphics;

std::string CharEscapeRenderResultEncoder::Encode(
    const std::vector<unsigned char> &buffer) {
  std::string str(buffer.begin(), buffer.end());

  // escape some special chars by incrementing them
  for (char &i : str) {
    char c = i;
    switch (c) {
    case '\"':
    case '\'':
    case '\n':
    case '\r':
      c = c++;
      break;
    default:
      break;
    }
    i = c;
  }

  return str;
}

std::vector<unsigned char>
CharEscapeRenderResultEncoder::Decode(const std::string &str) {
  return std::vector<unsigned char>(str.begin(), str.end());
}
