#include "logging/LogManager.h"

std::shared_ptr<logging::logger::ILogger>
logging::LogManager::GetLogger() noexcept {
  if (!m_logger) {
    m_logger = std::make_shared<logger::impl::BoostLogger>();
  }

  return m_logger;
}

std::shared_ptr<logging::logger::ILogger> logging::LogManager::m_logger;