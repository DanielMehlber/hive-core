# Hive Core Framework

Hive is a framework for building modular, distributed, and scalable simulation and image generation applications. It
employs a plugin system which allows developers to equip individual nodes with abilities they can contribute to the
hive. In practice this results in simulation environments across multiple machines offering different kinds of services.

## Capabilities

* **Modular**: Hive is built around the concept of nodes. Each node is a self-contained instance that can
  connect with others to create complex systems.
* **Distributed**: Nodes can be distributed across multiple machines. This allows for the creation of large-scale
  simulations.
* **Scalable**: Hive is designed to be scalable. It can run on a single machine or across multiple machines.
* **Extendable**: Hive has a plugin system that allows developers to extend the functionality of the framework.
  This allows for the employment of custom nodes in the hive.
* **Image Generation**: Hive has a built-in graphics engine that allows for the creation of 3D scenes and rendering of
  images using cutting-edge APIs like Vulkan.

## Dependencies

* [Boost 1.85](https://www.boost.org/)
* [Vulkan SDK](https://vulkan.lunarg.com/)
* [Vulkan Scene Graph (VSG)](https://github.com/vsg-dev/VulkanSceneGraph)