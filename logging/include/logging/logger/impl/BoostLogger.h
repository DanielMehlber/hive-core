#include "../ILogger.h"

namespace logging::logger::impl {

/**
 * @brief Logger implementation that uses Boost.Log
 */
class BoostLogger : public logger::ILogger {
public:
  BoostLogger();
  virtual ~BoostLogger();

  virtual void Init() final override;
  virtual void ShutDown() final override;
  virtual void Info(const std::string &message) const noexcept final override;
  virtual void Warn(const std::string &message) const noexcept final override;
  virtual void Error(const std::string &message) const noexcept final override;
  virtual void Debug(const std::string &message) const noexcept final override;
};

} // namespace logging::logger::impl