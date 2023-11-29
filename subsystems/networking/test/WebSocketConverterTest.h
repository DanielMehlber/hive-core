#ifndef WEBSOCKETCONVERTERTEST_H
#define WEBSOCKETCONVERTERTEST_H

#include "networking/peers/PeerMessageConverter.h"
#include <gtest/gtest.h>
#include <memory>

using namespace networking::websockets;

TEST(WebSockets, message_converter_serializing) {

  SharedWebSocketMessage message = std::make_shared<PeerMessage>("some-type");
  message->SetAttribute("attr1", "value1");
  message->SetAttribute("attr2", "value2");

  std::string payload =
      networking::websockets::PeerMessageConverter::ToJson(message);
  SharedWebSocketMessage same_message =
      networking::websockets::PeerMessageConverter::FromJson(payload);

  ASSERT_TRUE(message->EqualsTo(same_message));
}

TEST(WebSockets, message_converter_invalid) {
  PeerMessageConverter converter;

  SharedWebSocketMessage message = std::make_shared<PeerMessage>("some-type");
  message->SetAttribute("attr1", "value1");
  message->SetAttribute("attr2", "value2");

  std::string payload =
      networking::websockets::PeerMessageConverter::ToJson(message);

  // now half of the message gets lost
  auto invalid_payload = payload.substr(0, payload.size() / 2);

  ASSERT_THROW(converter.FromJson(invalid_payload),
               MessagePayloadInvalidException);
}

#endif /* WEBSOCKETCONVERTERTEST_H */
