#pragma once

#include "networking/util/UrlParser.h"
#include <gtest/gtest.h>

using namespace hive::networking::util;

void testParser(const std::string &scheme, const std::string &host,
                const std::string &port = "", const std::string &path = "",
                const std::string &query = "") {

  std::string final_scheme = scheme.empty() ? "" : scheme + "://";
  std::string final_port = port.empty() ? "" : ":" + port;
  std::string final_query = query.empty() ? "" : "?" + query;

  std::stringstream ss;
  ss << final_scheme << host << final_port << path << final_query;

  auto opt_parsed_url = UrlParser::parse(ss.str());
  ASSERT_TRUE(opt_parsed_url.has_value())
      << ss.str() << " was not accepted by parser";

  auto parsed_url = opt_parsed_url.value();
  ASSERT_STREQ(parsed_url.scheme.c_str(), scheme.c_str());
  ASSERT_STREQ(parsed_url.host.c_str(), host.c_str());
  ASSERT_STREQ(parsed_url.port.c_str(), port.c_str());
  ASSERT_STREQ(parsed_url.path.c_str(), path.c_str());
  ASSERT_STREQ(parsed_url.query.c_str(), query.c_str());
}

TEST(UrlParser, parse_valid) {
  testParser("http", "test.com", "23", "/hallo", "query=true");
  testParser("", "test.com", "", "/hallo", "query=true");
  testParser("https", "test.com", "23");
  testParser("ws", "hallo.test.com", "9999", "", "query=true");
}
