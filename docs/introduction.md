# Technical Overview

[TOC]

## Subsystems and Components

Hive consists of multiple modules, also called sub-systems. The following sub-systems are part of the Hive framework:

* **Job-System**: Nearly all tasks and workloads from both subsystems and plugins are scheduled for **multithreaded**
  processing (_Everything is a job_). It provides speed, flexibility, and observability.
* **Event-System**: Provides a way to communicate between different parts of the framework. It is used to signal events
  and data between subsystems, plugins, and nodes. The `IEventBroker` interface can be extended to also support
  third-party brokers, like Apache Kafka.
* **Resource Management**: Used to load files, images, and other resources from disk or memory in a unified and
  asynchronous
  manner. `IResourceLoader` can be extended to support custom file formats and sources.
* **Properties**: Manages shared data in the framework and allows plugins or subsystems to listen for changes.
* **Networking**: Provides a way to communicate between different nodes in the hive. It currently supports persistent
  web-socket tunnels for messaging, but the `IMessagingEndpoint` interface can be easily extended to support other
  protocols like
  ZeroMQ or REST.
* **Service Infrastructure**: Allows offering and requesting services in the hive. Services can be requested by
  other nodes in the hive. By default, it uses simple messaging (`IMessagingEndpoint`), but
  the `IServiceRegistry` interface can be extended to support third-party service registry
  technologies and infrastructures.
* **Graphics**: Used for image generation. Uses Vulkan by default.
* **Plugin Management**: Loads and unloads implementations of `IPlugin` which can contain subsystem implementations,
  services, properties, or
  other capabilities that can be offered to the hive.

The `Core` module simply wraps and initializes these subsystems.

## Single Node Architecture

A single node in the hive consists of the core and a set of plugins:

* The core contains all subsystems and is provided by the framework. The core itself does not possess any functionality
  but acts as container, infrastructure, and platform for the plugins.
* Plugins are dynamic libraries that are loaded at runtime. They contain implementations of subsystems, services, and
  properties. They contribute their capabilities to the hive.

![Single Node Architecture](./docs/images/single-node.png)

An example for this is the Coordinator-Worker-Pattern: A node loads the `Coordinator` plugin, which enables it to
distribute workloads to nodes equipped with the `Worker` plugin in the same hive. This pattern can be used in various
scenarios.

## Distributed Nodes Architecture (Hive)

A hive consists of multiple nodes that are connected via a network. Each node in the hive can offer and request
services, access properties, publish events, and communicate with other nodes in the hive. Nodes can be added or removed
from the hive at runtime.

![Distributed Hive Architecture](./docs/images/nodes-in-hive.png)

When a new node joins the hive, it gains access to all services offered by other nodes in the hive and vice versa. The
workload can then be distributed across all nodes in the hive.