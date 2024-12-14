#pragma once
#include "networking/messaging/converter/IMessageConverter.h"

namespace hive::networking::messaging {

/**
 * Converts messages to and from JSON content. This format is not suited for
 * transporting binary data because it can corrupt the encoding and JSON syntax.
 * @attention Do not use for binary data.
 */
class JsonMessageConverter : public IMessageConverter {
public:
  std::string GetContentType() override;
  SharedMessage Deserialize(const std::string &data) override;
  std::string Serialize(const SharedMessage &message) override;

  ~JsonMessageConverter() override = default;
};

inline std::string JsonMessageConverter::GetContentType() {
  return "application/json";
}

} // namespace hive::networking::messaging