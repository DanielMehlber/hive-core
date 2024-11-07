#include "data/providers/impl/LocalDataProvider.h"

using namespace hive::data;

std::future<std::optional<std::string>>
LocalDataProvider::Get(const std::string &path) {
  std::promise<std::optional<std::string>> promise;
  std::future<std::optional<std::string>> future = promise.get_future();

  try {
    auto value = m_data.get<std::string>(path);
    promise.set_value(value);
  } catch (const boost::property_tree::ptree_bad_path &e) {
    promise.set_value({});
  }

  return future;
}

void LocalDataProvider::Set(const std::string &path, const std::string &data) {
  m_data.put(path, data);
}