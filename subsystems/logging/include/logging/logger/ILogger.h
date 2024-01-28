#ifndef ILOGGER_H
#define ILOGGER_H

#include <string>

namespace logging::logger {

/**
 * Generic interface for logging
 */
class ILogger {
public:
  virtual void Init() = 0;
  virtual void ShutDown() = 0;
  virtual void Info(const std::string &message) const noexcept = 0;
  virtual void Warn(const std::string &message) const noexcept = 0;
  virtual void Error(const std::string &message) const noexcept = 0;
  virtual void Debug(const std::string &message) const noexcept = 0;
};
} // namespace logging::logger

#endif /* ILOGGER_H */
