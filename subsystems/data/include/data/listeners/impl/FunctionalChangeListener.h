#pragma once

#include "data/listeners/IDataChangeListener.h"
#include <functional>
#include <string>

namespace hive::data {

typedef std::function<void(const std::string &, const std::string &)>
    change_callback_t;

/**
 * A functional change listener is a simple wrapper around a lambda function
 * that is called when the observed data point changes.
 */
class FunctionalChangeListener : public IDataChangeListener {
  change_callback_t m_callback;

public:
  explicit FunctionalChangeListener(change_callback_t callback);
  void Notify(const std::string &path, const std::string &new_value) override;
};

} // namespace hive::data
