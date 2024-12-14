#pragma once

#include "networking/messaging/converter/MultipartMessageConverter.h"
#include <gtest/gtest.h>
#include <memory>

using namespace hive;
using namespace hive::networking;
using namespace hive::networking::messaging;

TEST(WebSockets, multipart_converter_valid) {

  SharedMessage message = std::make_shared<Message>("some-type");
  message->SetAttribute("attr1", "value1");
  message->SetAttribute("attr2", "value2");

  MultipartMessageConverter converter;
  std::string payload = converter.Serialize(message);
  SharedMessage same_message = converter.Deserialize(payload);

  ASSERT_TRUE(message->EqualsTo(same_message));
}

TEST(WebSockets, multipart_converter_invalid) {
  MultipartMessageConverter converter;

  SharedMessage message = std::make_shared<Message>("some-type");
  message->SetAttribute("attr1", "value1");
  message->SetAttribute("attr2", "value2");

  std::string payload = converter.Serialize(message);

  // now half of the message gets lost
  auto invalid_payload = payload.substr(0, payload.size() / 2);

  ASSERT_THROW(converter.Deserialize(invalid_payload),
               MessagePayloadInvalidException);
}
