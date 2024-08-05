#pragma once

#include "common/uuid/UuidGenerator.h"
#include "events/Event.h"
#include "events/listener/IEventListener.h"
#include <functional>
#include <string>

namespace hive::events {

/**
 * Allows handling events using a simple lambda functions instead of
 * implementing the interface.
 */
class FunctionalEventListener : public IEventListener {
protected:
  const std::function<void(const SharedEvent)> m_function;
  const std::string m_id;

public:
  FunctionalEventListener() = delete;
  explicit FunctionalEventListener(
      const std::function<void(const SharedEvent)> &func)
      : m_function{func}, m_id{common::uuid::UuidGenerator::Random()} {};
  explicit FunctionalEventListener(
      const std::function<void(const SharedEvent)> &&func)
      : m_function{func}, m_id{common::uuid::UuidGenerator::Random()} {};

  FunctionalEventListener
  operator=(const std::function<void(const SharedEvent)> &func) {
    return FunctionalEventListener(func);
  };

  void HandleEvent(SharedEvent event) override { m_function(event); }
  std::string GetId() const override { return m_id; };
};

} // namespace hive::events
