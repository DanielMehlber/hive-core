#include "logging/LogManager.h"

using namespace hive::logging;

std::shared_ptr<logger::ILogger> LogManager::GetLogger() noexcept {
  if (!m_logger) {
    m_logger = std::make_shared<logger::impl::BoostLogger>();
  }

  return m_logger;
}

std::shared_ptr<logger::ILogger> LogManager::m_logger;