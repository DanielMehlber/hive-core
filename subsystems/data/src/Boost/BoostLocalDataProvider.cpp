#include "data/providers/impl/LocalDataProvider.h"

#include <boost/property_tree/ptree.hpp>

using namespace hive::data;

struct LocalDataProvider::Impl {
  boost::property_tree::ptree data_tree;
};

LocalDataProvider::LocalDataProvider() : m_impl(std::make_unique<Impl>()) {}
LocalDataProvider::~LocalDataProvider() = default;

LocalDataProvider::LocalDataProvider(LocalDataProvider &&other) noexcept =
    default;
LocalDataProvider &
LocalDataProvider::operator=(LocalDataProvider &&other) noexcept = default;

std::future<std::optional<std::string>>
LocalDataProvider::Get(const std::string &path) {
  std::promise<std::optional<std::string>> promise;
  std::future<std::optional<std::string>> future = promise.get_future();

  try {
    auto value = m_impl->data_tree.get<std::string>(path);
    promise.set_value(value);
  } catch (const boost::property_tree::ptree_bad_path &e) {
    promise.set_value({});
  }

  return future;
}

void LocalDataProvider::Set(const std::string &path, const std::string &data) {
  m_impl->data_tree.put(path, data);
}