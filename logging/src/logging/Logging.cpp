#include "logging/Logging.h"

std::shared_ptr<logging::logger::ILogger>
logging::Logging::GetLogger() noexcept {
  if (!m_logger) {
    m_logger = std::make_shared<logger::impl::BoostLogger>();
  }

  return m_logger;
}

std::shared_ptr<logging::logger::ILogger> logging::Logging::m_logger;