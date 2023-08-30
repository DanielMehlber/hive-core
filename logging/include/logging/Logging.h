#ifndef LOGGINGAPI_H
#define LOGGINGAPI_H

#include "logger/ILogger.h"
#include "logger/impl/BoostLogger.h"
#include <memory>

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
class EXPORT_API Logging {
private:
  static std::shared_ptr<logger::ILogger> m_logger;

public:
  static std::shared_ptr<logger::ILogger> GetLogger() noexcept;
};

inline std::shared_ptr<logging::logger::ILogger> GetLogger() {
  return Logging::GetLogger();
}

#define LOG_INFO(x) logging::Logging::GetLogger()->Info(x)
#define LOG_WARN(x) logging::Logging::GetLogger()->Warn(x)
#define LOG_ERR(x) logging::Logging::GetLogger()->Error(x)

#ifdef NDEBUG
#define LOG_DEBUG(x)
#else
#define LOG_DEBUG(x) logging::GetLogger()->Debug(x)
#endif

} // namespace logging

#endif /* LOGGINGAPI_H */
