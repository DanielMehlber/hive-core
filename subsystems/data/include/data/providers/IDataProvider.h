#pragma once

#include <future>
#include <optional>
#include <string>

namespace hive::data {

/**
 * Represents a data source from which data can be read and updated. The data is
 * organized in a hierarchical fashion and can be accessed using a path-like id
 * that is seperated by points (example: 'rendering.distribution.count' or
 * 'plugin.settings.paths.base'). If some data has been updated in a provider,
 * the data layer must be notified about the change so that listeners can adapt
 * to the change.
 */
class IDataProvider {
public:
  virtual ~IDataProvider() = default;
  /**
   * Retrieves the data with the given path. Depending on the underlying
   * implementation this operation can take a while, like for remote data
   * stores, and is thus asynchronous.
   * @param path unique identifier of the data in a path-like format.
   * @return future that will contain the data if it exists
   */
  virtual std::future<std::optional<std::string>>
  Get(const std::string &path) = 0;

  /**
   * Sets the data with the given path. Depending on the underlying
   * implementation this operation can take a while, like for remote data
   * stores, and is thus asynchronous.
   * @param path unique identifier of the data in a path-like format. If the
   * path does not exist yet, it will be created.
   * @param data new value of the data. If the data already exists, the old
   * value will be overwritten.
   */
  virtual void Set(const std::string &path, const std::string &data) = 0;
};
} // namespace hive::data
