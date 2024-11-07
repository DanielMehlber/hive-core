#pragma once

#include "networking/messaging/MessageConverter.h"
#include <gtest/gtest.h>
#include <memory>

using namespace hive;
using namespace hive::networking;
using namespace hive::networking::messaging;

TEST(WebSockets, message_converter_serializing) {

  SharedMessage message = std::make_shared<Message>("some-type");
  message->SetAttribute("attr1", "value1");
  message->SetAttribute("attr2", "value2");

  std::string payload = MessageConverter::ToMultipartFormData(message);
  SharedMessage same_message = MessageConverter::FromMultipartFormData(payload);

  ASSERT_TRUE(message->EqualsTo(same_message));
}

TEST(WebSockets, message_converter_invalid) {
  MessageConverter converter;

  SharedMessage message = std::make_shared<Message>("some-type");
  message->SetAttribute("attr1", "value1");
  message->SetAttribute("attr2", "value2");

  std::string payload = MessageConverter::ToMultipartFormData(message);

  // now half of the message gets lost
  auto invalid_payload = payload.substr(0, payload.size() / 2);

  ASSERT_THROW(converter.FromMultipartFormData(invalid_payload),
               MessagePayloadInvalidException);
}
