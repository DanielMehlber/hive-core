# Introduction

Hive is a framework for building distributed, modular, and highly scalable simulation and image generation applications.
It
employs a plugin-based approach which allows nodes in the hive to contribute their individual abilities and offer
various services. In practice this results in simulations running across multiple machines in a collaborative fashion.

## Applications

* **Monte-Carlo Simulations**: Vast amounts of procedurally generated simulations can be run in parallel across multiple
  machines, possibly reducing the time from hours to minutes.
* **Multi-View Image Generation**: Hive can be used to generate images from multiple viewpoints in a scene in parallel.

## Core Features and Capabilities

* **Distributed**: Hive is designed to run across multiple machines. Distributed nodes communicate using network
  protocols to distribute the workload.
* **High Performance**: Hive's core is purely multithreaded and designed to process its tasks in a parallel manner.
  Written in C++ and employing modern high-performance APIs like Vulkan to utilize GPU cores, it is designed to be fast.
* **Asynchronous**: Submitted tasks or services are processed without blocking the node's execution. This allows for
  heavy simulation services with completion times up to minutes without impairing the responsiveness of the system.
* **Dynamically Scalable**: Nodes can be added or removed from the hive at runtime. This allows for the creation of
  systems which
  distribute their workload in a smart and efficient manner.
* **Fault-Tolerant**: Hive is designed to be fault-tolerant. If a node fails, the system can continue to run
  without interruption.
* **Extendable**: Hive has a plugin system that allows developers to extend the functionality of the framework.
  This allows for the employment of custom nodes in the hive.
* **Image Generation**: Hive has a built-in graphics engine that allows for the creation of 3D scenes and rendering of
  images for simulation data using cutting-edge APIs like Vulkan.

## Dependencies

* [Boost 1.85](https://www.boost.org/)
* [Vulkan SDK](https://vulkan.lunarg.com/)
* [Vulkan Scene Graph (VSG)](https://github.com/vsg-dev/VulkanSceneGraph)