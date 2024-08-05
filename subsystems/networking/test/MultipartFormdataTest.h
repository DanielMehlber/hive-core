#pragma once

#include "networking/util/MultipartFormdata.h"
#include <gtest/gtest.h>
#include <iostream>
#include <random>

using namespace hive::networking::util;

std::string generateRandomBinaryData(size_t size) {
  // Use a random_device to seed the random number generator
  std::random_device rd;
  std::default_random_engine generator(rd());
  std::uniform_int_distribution<unsigned int> distribution(0, 255);

  // Generate random bytes and construct the binary string
  std::string binaryData;
  binaryData.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    binaryData.push_back(static_cast<char>(distribution(generator)));
  }

  return binaryData;
}

TEST(MultipartFormdata, generate_and_parse) {
  Multipart multipart;

  auto random_binary_data = generateRandomBinaryData(10000);

  Part a{"a", "ich bin a"};
  Part b{"b", "ich bin b"};
  Part binary{"binary", std::move(random_binary_data)};

  multipart.parts[a.name] = a;
  multipart.parts[b.name] = b;
  multipart.parts[binary.name] = binary;

  std::string str = generateMultipartFormData(multipart);

  Multipart parsed_multipart = parseMultipartFormData(str);

  ASSERT_TRUE(parsed_multipart.parts.contains(a.name));
  ASSERT_TRUE(parsed_multipart.parts[a.name].content == a.content);

  ASSERT_TRUE(parsed_multipart.parts.contains(b.name));
  ASSERT_TRUE(parsed_multipart.parts[b.name].content == b.content);

  ASSERT_TRUE(parsed_multipart.parts.contains(binary.name));
  ASSERT_TRUE(parsed_multipart.parts[binary.name].content == binary.content);
}
