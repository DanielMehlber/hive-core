#ifndef URLPARSER_H
#define URLPARSER_H

#include "networking/Networking.h"
#include <optional>
#include <regex>
#include <string>

namespace networking::util {

/**
 * @brief Extracted tokens of an URL
 */
struct NETWORKING_API ParsedUrl {
  std::string scheme;
  std::string host;
  std::string port;
  std::string path;
  std::string query;
};

/**
 * @brief Parses URLs used in the network module
 */
class NETWORKING_API UrlParser {
public:
  /**
   * @brief Parses url string if valid
   * @param url url string that will be parsed
   * @return an object containing parsed url tokens if the given url is valid
   */
  static std::optional<ParsedUrl> parse(const std::string &url);
};
} // namespace networking::util

#endif /* URLPARSER_H */
