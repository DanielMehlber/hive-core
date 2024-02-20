#include "logging/logger/impl/BoostLogger.h"
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <thread>

void logging::logger::impl::BoostLogger::Init() {
  // setup console logger
  boost::log::add_console_log(
      std::clog,
      boost::log::keywords::format =
          "[%TimeStamp%] [%Severity%] in Thread [%ThreadID%]: %Message%");
  boost::log::add_common_attributes();
}

void logging::logger::impl::BoostLogger::ShutDown(){};

logging::logger::impl::BoostLogger::~BoostLogger() { ShutDown(); }
logging::logger::impl::BoostLogger::BoostLogger() { Init(); }

void logging::logger::impl::BoostLogger::Info(
    const std::string &message) const noexcept {
  BOOST_LOG_TRIVIAL(info) << boost::log::add_value("ThreadID",
                                                   std::this_thread::get_id())
                          << message;
}

void logging::logger::impl::BoostLogger::Error(
    const std::string &message) const noexcept {
  BOOST_LOG_TRIVIAL(error) << boost::log::add_value("ThreadID",
                                                    std::this_thread::get_id())
                           << message;
}

void logging::logger::impl::BoostLogger::Warn(
    const std::string &message) const noexcept {
  BOOST_LOG_TRIVIAL(warning)
      << boost::log::add_value("ThreadID", std::this_thread::get_id())
      << message;
}

void logging::logger::impl::BoostLogger::Debug(
    const std::string &message) const noexcept {
  BOOST_LOG_TRIVIAL(debug) << boost::log::add_value("ThreadID",
                                                    std::this_thread::get_id())
                           << message;
}