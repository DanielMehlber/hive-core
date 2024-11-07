#pragma once

#include "data/providers/IDataProvider.h"
#include <boost/property_tree/ptree.hpp>
#include <future>
#include <optional>

namespace hive::data {

class LocalDataProvider : public IDataProvider {
  /** property tree keeps data hierarchy */
  boost::property_tree::ptree m_data;

public:
  std::future<std::optional<std::string>> Get(const std::string &path) override;
  void Set(const std::string &path, const std::string &data) override;
};

} // namespace hive::data
