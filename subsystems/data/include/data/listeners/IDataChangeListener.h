#pragma once

#include <string>

namespace hive::data {

class IDataChangeListener {
public:
  virtual ~IDataChangeListener() = default;

  /**
   * Called when data in the observed hierarchy has changed. Each listener can
   * implement its own handling operations.
   * @param path absolute path to the data point that has been changed.
   * @param new_value new value of the property
   */
  virtual void Notify(const std::string &path,
                      const std::string &new_value) = 0;
};

} // namespace hive::data