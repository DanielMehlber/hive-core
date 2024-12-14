#pragma once
#include "networking/messaging/converter/IMessageConverter.h"

namespace hive::networking::messaging {

/**
 * Converts messages to and from multipart/form-data content. This format is
 * recommended if the message contains binary data and must be split into
 * multiple parts, like attachments.
 */
class MultipartMessageConverter : public IMessageConverter {
public:
  std::string GetContentType() override;
  SharedMessage Deserialize(const std::string &data) override;
  std::string Serialize(const SharedMessage &message) override;

  ~MultipartMessageConverter() override = default;
};

inline std::string MultipartMessageConverter::GetContentType() {
  return "multipart/form-data";
}

} // namespace hive::networking::messaging