#pragma once

#include "logger/ILogger.h"
#include "logger/impl/BoostLogger.h"
#include <memory>
#include <sstream>

namespace logging {

/**
 * Provides a logger implementations
 */
class LogManager {
private:
  static std::shared_ptr<logger::ILogger> m_logger;

public:
  /**
   * Constructs the default logger implementation (if logger does not
   * exist yet).
   * @return the default logger implementation
   */
  static std::shared_ptr<logger::ILogger> GetLogger() noexcept;
};

inline std::shared_ptr<logging::logger::ILogger> GetLogger() {
  return LogManager::GetLogger();
}

#define LOG_INFO(x)                                                            \
  {                                                                            \
    std::stringstream __ss;                                                    \
    __ss << x;                                                                 \
    logging::GetLogger()->Info(__ss.str());                                    \
  }

#define LOG_WARN(x)                                                            \
  {                                                                            \
    std::stringstream __ss;                                                    \
    __ss << x;                                                                 \
    logging::GetLogger()->Warn(__ss.str());                                    \
  }

#define LOG_ERR(x)                                                             \
  {                                                                            \
    std::stringstream __ss;                                                    \
    __ss << x;                                                                 \
    logging::GetLogger()->Error(__ss.str());                                   \
  }

#ifdef NDEBUG
#define LOG_DEBUG(x)
#else
#define LOG_DEBUG(x)                                                           \
  {                                                                            \
    std::stringstream __ss;                                                    \
    __ss << x;                                                                 \
    logging::GetLogger()->Debug(__ss.str());                                   \
  }
#endif

} // namespace logging
