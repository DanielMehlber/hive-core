#pragma once

#include <string>

namespace common::uuid {

/**
 * Generates UUIDs randomly.
 */
class UuidGenerator {
public:
  static std::string Random() noexcept;
};
} // namespace common::uuid
