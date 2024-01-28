#ifndef UUIDGENERATOR_H
#define UUIDGENERATOR_H

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

#endif // UUIDGENERATOR_H
