#pragma once

#include "PropertyChangeListener.h"
#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "events/broker/IEventBroker.h"
#include <boost/property_tree/ptree.hpp>
#include <memory>
#include <optional>
#include <string>

namespace props {

/**
 * Provides read and write access to properties, as well as notification
 * to their changes. Properties are onganized in a property tree, meaning that
 * every property has a path in the hierarchy. This enables grouping and
 * listening to subsets of properties.
 */
class PropertyProvider
    : public common::memory::EnableBorrowFromThis<PropertyProvider> {
protected:
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

  boost::property_tree::ptree m_property_tree;

  void NotifyListenersAboutChange(const std::string &path);

public:
  explicit PropertyProvider(
      const common::memory::Reference<common::subsystems::SubsystemManager>
          &subsystems);
  virtual ~PropertyProvider();

  /**
   * Sets an existing property's value or creates a new one.
   * @tparam PropType type of the property's value
   * @param key uniquely identifies the property using its path in the property
   * tree
   * @param value new value of the property. If the property already exists, the
   * old value will be overwritten.
   */
  template <typename PropType> void Set(const std::string &key, PropType value);

  /**
   * GetAsInt value of some property (if it exists)
   * @tparam PropType type of the property's value
   * @param key path to some leaf in the property tree
   * @return value of property if it exists
   * @note If the given key does not belong to a concrete property (leaf) but
   * instead to a collection of properties (node) in the property tree, no
   * result will be returned.
   */
  template <typename PropType>
  std::optional<PropType> Get(const std::string &key);

  /**
   * GetAsInt value of some property or a alternative value if this
   * property does not exist
   * @tparam PropType type of the property's value
   * @param key path to some leaf in the property tree
   * @param value alternative value that will be returned if the requested
   * property does not exist.
   * @return The requested property's value or the alternative value if it does
   * not exist
   */
  template <typename PropType>
  PropType GetOrElse(const std::string &key, PropType value);

  /**
   * Registeres a property listener for a concrete property (if the path
   * leads to a leaf in the property tree) or to a subset of properties (if the
   * path leads to a node containing more leafs). Example: Listening to
   * 'example.property' also notifies on changes in property
   * 'example.property.value', but not in 'example'.
   * @param path path in the property tree
   * @param listener listener that listens to changes of the property of subset
   * of properties in the tree.
   */
  void RegisterListener(const std::string &path,
                        const SharedPropertyListener &listener);

  /**
   * Unregisters a property listener from all paths in the property tree.
   * @param listener listener that will be unregistered.
   */
  void UnregisterListener(const SharedPropertyListener &listener);
};

template <typename PropType>
inline void PropertyProvider::Set(const std::string &key,
                                  const PropType value) {
  m_property_tree.put(key, value);
  NotifyListenersAboutChange(key);
}

template <typename PropType>
std::optional<PropType> PropertyProvider::Get(const std::string &key) {
  auto optional_value =
      m_property_tree.get_optional<PropType>(boost::property_tree::path(key));

  if (optional_value) {
    return std::optional<PropType>(*optional_value);
  } else {
    return {};
  }
}

template <typename PropType>
inline PropType PropertyProvider::GetOrElse(const std::string &key,
                                            const PropType value) {
  auto optional_value = Get<PropType>(key);
  if (optional_value.has_value()) {
    return optional_value.value();
  } else {
    return value;
  }
}

} // namespace props
