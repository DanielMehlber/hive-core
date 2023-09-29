#ifndef LOGGINGAPI_H
#define LOGGINGAPI_H

#include "logger/ILogger.h"
#include "logger/impl/BoostLogger.h"
#include <memory>
#include <sstream>

#ifndef EXPORT_MACRO_H
#define EXPORT_MACRO_H

#ifdef _WIN32
// For Windows (MSVC compiler)
#ifdef EXPORT_DLL
// When building the library
#define EXPORT_API __declspec(dllexport)
#else
// When using the library
#define EXPORT_API __declspec(dllimport)
#endif
#else
// For other platforms (Linux, macOS)
#define EXPORT_API
#endif

#endif

namespace logging {

/**
 * @brief Provides a logger implementations
 */
class EXPORT_API Logging {
private:
  static std::shared_ptr<logger::ILogger> m_logger;

public:
  /**
   * @brief Constructs the default logger implementation (if logger does not
   * exist yet).
   * @return the default logger implementation
   */
  static std::shared_ptr<logger::ILogger> GetLogger() noexcept;
};

inline std::shared_ptr<logging::logger::ILogger> GetLogger() {
  return Logging::GetLogger();
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

#endif /* LOGGINGAPI_H */
