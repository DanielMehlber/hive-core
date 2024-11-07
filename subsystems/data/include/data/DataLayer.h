#pragma once

#include "common/memory/ExclusiveOwnership.h"
#include "common/subsystems/SubsystemManager.h"
#include "data/listeners/IDataChangeListener.h"
#include "data/providers/IDataProvider.h"
#include "data/providers/impl/LocalDataProvider.h"
#include "jobsystem/synchronization/JobMutex.h"
#include <map>
#include <vector>

namespace hive::data {

typedef LocalDataProvider DefaultDataProvider;

/**
 * Represents the data layer of the system. It is responsible for managing
 * global application data in a hierarchical fashion and notifying listeners
 * about changes in this hierarchy.
 *
 * It organizes data in a tree-like structure where leaves contain values
 * branches are paths to them. Each value can be accessed via its path in
 * dot-notation (example: 'rendering.distribution.count' or
 * 'plugins.loaded.').
 */
class DataLayer : public common::memory::EnableBorrowFromThis<DataLayer> {
  /** exchangeable implementation of the data provider */
  std::shared_ptr<IDataProvider> m_provider;
  mutable jobsystem::mutex m_provider_mutex;

  /** subsystems that the data layer requires */
  common::memory::Reference<common::subsystems::SubsystemManager> m_subsystems;

  /** maps pattern of paths to listeners */
  std::map<std::string, std::vector<std::weak_ptr<IDataChangeListener>>>
      m_listeners;
  mutable jobsystem::mutex m_listeners_mutex;

  /** used to track the data layer's liveliness in clean-up jobs */
  std::shared_ptr<bool> m_alive;

public:
  explicit DataLayer(
      common::memory::Borrower<common::subsystems::SubsystemManager> subsystems,
      const std::shared_ptr<IDataProvider> &provider =
          std::make_shared<DefaultDataProvider>());
  ~DataLayer();

  /**
   * Adds a new listener that is interested in changes in paths matching the
   * given pattern or sub-path in the data hierarchy.
   * @data_pattern regex pattern of paths that the listener is interested in
   * @listener listener that will be notified about changes in paths matching
   * the pattern. These listeners are weakly referenced and are not managed by
   * the data layer. When they expire, they will be removed from the list of
   * listeners.
   */
  void RegisterListener(const std::string &data_pattern,
                        const std::weak_ptr<IDataChangeListener> &listener);

  /**
   * Sets or replaces the underlying provider implementation.
   * @param provider new provider implementation
   */
  void SetProvider(const std::shared_ptr<IDataProvider> &&provider);

  /**
   * Sets the data at the given path. If the path does not exist yet, it will
   * be created. If the data already exists, the old value will be overwritten.
   * @param path unique identifier of the data in a path-like format
   * @param data new value of the data
   */
  void Set(const std::string &path, const std::string &data) const;

  /**
   * Retrieves the data at the given path. If the data does not exist, the
   * future will contain an empty optional.
   * @param path unique identifier of the data in a path-like format
   * @return future that will contain the data if it exists
   */
  std::future<std::optional<std::string>> Get(const std::string &path) const;

  void NotifyChange(const std::string &path, const std::string &data) const;
};

} // namespace hive::data