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

using namespace hive::logging::logger::impl;

void BoostLogger::Init() {
  // setup console logger
  boost::log::add_console_log(
      std::clog,
      boost::log::keywords::format =
          "[%TimeStamp%] [%Severity%] in Thread [%ThreadID%]: %Message%");
  boost::log::add_common_attributes();
}

void BoostLogger::ShutDown(){};

BoostLogger::~BoostLogger() { ShutDown(); }
BoostLogger::BoostLogger() { Init(); }

void BoostLogger::Info(const std::string &message) const noexcept {
  BOOST_LOG_TRIVIAL(info) << boost::log::add_value("ThreadID",
                                                   std::this_thread::get_id())
                          << message;
}

void BoostLogger::Error(const std::string &message) const noexcept {
  BOOST_LOG_TRIVIAL(error) << boost::log::add_value("ThreadID",
                                                    std::this_thread::get_id())
                           << message;
}

void BoostLogger::Warn(const std::string &message) const noexcept {
  BOOST_LOG_TRIVIAL(warning)
      << boost::log::add_value("ThreadID", std::this_thread::get_id())
      << message;
}

void BoostLogger::Debug(const std::string &message) const noexcept {
  BOOST_LOG_TRIVIAL(debug) << boost::log::add_value("ThreadID",
                                                    std::this_thread::get_id())
                           << message;
}