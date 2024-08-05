#pragma once

#include <string>

namespace hive::common::uuid {

/**
 * Generates UUIDs randomly.
 */
class UuidGenerator {
public:
  static std::string Random() noexcept;
};
} // namespace hive::common::uuid
