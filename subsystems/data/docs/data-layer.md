# Data Layer

[TOC]

## Introduction

The data layer is responsible for managing the data of the application. Instead of storing global data in individual
components or plugins in heterogeneous formats and finding tedious ways of exchanging them (e.g. using chained getters
and setters), the data layer can be used. It provides a way of centrally storing data in a structured manner and
listening to changes to certain data fields. This allows data synchronization between subsystems, plugins, and even
nodes.

The [DataLayer](\ref hive::data::DataLayer) class relies on a [IDataProvider](\ref hive::data::IDataProvider)
implementation. The data provider is responsible for storing and managing the data. The data layer itself manages and
notifies [IDataChangeListener](\ref hive::data::IDataChangeListener) instances about changes to the data.

## Data Providers

This subsystem is designed to interface with many data storage technologies. Implementations of
the [IDataProvider](\ref hive::data::IDataProvider) can be:

* A simple in-memory data structure, like the [LocalDataProvider](\ref hive::data::LocalDataProvider).
* Some central database or key-value store accessed by multiple nodes (not implemented yet). Network delays might be
  involved when fetching data from a remote server. This is why operations of
  the [IDataProvider](\ref hive::data::IDataProvider) are asynchronous and return a `std::future` by choice.
* Some integrated distributed database, like [etcd](https://etcd.io/) or [ZooKeeper](https://zookeeper.apache.org/) that
  synchronizes shared data with other nodes automatically (not implemented yet).
* Some protocol or algorithm that synchronizes data with other members in the cluster
  like [Raft](https://raft.github.io/) or [DDS](https://www.dds-foundation.org/what-is-dds-3/) (not implemented yet).

## Example Usage

```cpp
using namespace hive::data;

auto data_layer = subsystems->RequireSubsystem<DataLayer>();
auto job_system = subsystems->RequireSubsystem<JobManager>();

/* data is stored hierarchically in a tree structure */
data_layer->Set("my_plugin.settings.value", "10");
std::future<std::optional<std::string>> fetching_value = data_layer->Get<std::string>("my_plugin.settings.value");

/* if inside a job, you can wait for the completion of the data fetching operation.
 Otherwise just use future::wait() to block the thread. This step might seem redundant if there is just a local
 delay-free data structure running in the background, but we can't be sure and want to support databases */
job_system->WaitForCompletion(fetching_value);

/* data is not guaranteed to exist in the data layer */
std::optional<std::string> maybe_value = fetching_value.get();
if (maybe_value.has_value()) {
    std::cout << "Value: " << maybe_value.value() << std::endl;
} else {
    std::cout << "Value not found in data layer" << std::endl;
}

std::shared_ptr<IDataChangeListener> = std::make_shared<FunctionalDataChangeListener>(
  [](const std::string& path, const std::string& value) {
    std::cout << "Data changed at path " << path << " to value " << value << std::endl;
  }
);

/* listeners listen not only for exact path matches, but also for changes in the subsequent path. 
Example: when 'my_plugin.settings.some.nested.value' changed, listeners of 'my_plugin.settings' 
or 'my_plugin.settings.some' will also be notified. */
data_layer->RegisterListener("my_plugin.settings.value", listener); // listen for exact path
data_layer->RegisterListener("my_plugin.settings", listener); // listen for all changes in the subtree
```