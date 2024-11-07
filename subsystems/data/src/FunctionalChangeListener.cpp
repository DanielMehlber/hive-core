#include <utility>

#include "data/listeners/impl/FunctionalChangeListener.h"

using namespace hive::data;

FunctionalChangeListener::FunctionalChangeListener(change_callback_t callback)
    : m_callback(std::move(callback)) {}

void FunctionalChangeListener::Notify(const std::string &path,
                                      const std::string &new_value) {
  m_callback(path, new_value);
}