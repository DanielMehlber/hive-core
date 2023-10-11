#ifndef URLPARSER_H
#define URLPARSER_H

#include <optional>
#include <regex>
#include <string>

namespace networking::util {
struct ParsedUrl {
  std::string scheme;
  std::string host;
  std::string port;
  std::string path;
  std::string query;
};

class UrlParser {
public:
  static std::optional<ParsedUrl> parse(const std::string &url);
};
} // namespace networking::util

#endif /* URLPARSER_H */
