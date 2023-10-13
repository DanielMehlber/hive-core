#include "../ILogger.h"
#include "logging/Logging.h"

namespace logging::logger::impl {

/**
 * @brief Logger implementation that uses Boost.Log
 */
class NETWORKING_API BoostLogger : public logger::ILogger {
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

} // namespace logging::logger::impl