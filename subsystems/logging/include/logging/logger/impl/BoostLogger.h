#include "logging/logger/ILogger.h"

namespace hive::logging::logger::impl {

/**
 * Logger implementation that uses Boost.Log
 */
class BoostLogger : public ILogger {
public:
  BoostLogger();
  virtual ~BoostLogger();

  void Init() final;
  void ShutDown() final;
  void Info(const std::string &message) const noexcept final;
  void Warn(const std::string &message) const noexcept final;
  void Error(const std::string &message) const noexcept final;
  void Debug(const std::string &message) const noexcept final;
};

} // namespace hive::logging::logger::impl