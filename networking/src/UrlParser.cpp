#include "networking/util/UrlParser.h"

using namespace networking::util;

std::optional<ParsedUrl> UrlParser::parse(const std::string &url) {
  ParsedUrl result;

  // Regular expression for parsing URI components
  std::regex uriRegex(
      R"(^(([a-z.]*):\/\/)?([^\/:?\s#]+)(:(\d+))?([^?\s]*)?(\?([^\s]*))?$)");
  std::smatch match;

  if (std::regex_match(url, match, uriRegex)) {
    result.scheme = match[2].str();
    result.host = match[3].str();
    result.port = match[5].str();
    result.path = match[6].str();
    result.query = match[8].str();
  } else {
    return {};
  }

  return result;
}