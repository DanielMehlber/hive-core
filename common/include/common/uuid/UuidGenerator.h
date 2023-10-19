#ifndef UUIDGENERATOR_H
#define UUIDGENERATOR_H

#include <string>

namespace common::uuid {

/**
 * Interface layer for generating UUIDs used to separate usage and
 * implementation details, rendering the underlying UUID generator exchangeable.
 */
class UuidGenerator {
public:
  static std::string Random() noexcept;
};
} // namespace common::uuid

#endif // UUIDGENERATOR_H
