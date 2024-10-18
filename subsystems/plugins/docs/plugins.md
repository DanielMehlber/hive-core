# Plugin Management

[TOC]

Plugins allow third-party developers to equip the core with use-case specific functionality at runtime. Remember:

* The core only provides basic infrastructure and platform functionality that is **use-case independent**.
* Plugins package services, functionality, and jobs that solve specific problems by using the underlying core
  infrastructure. They are **use-case specific**.

That means, that the core does not have to be touched or modified to add functionality to it.

## Technical Overview

In software engineering, there are multiple ways to load plugins into a core system:

* **Script Bindings**: Popular software projects like [Blender 3D](https://www.blender.org/) use scripting languages to
  allow users to extend its behavior. This is very flexible and user-friendly, but requires script bindings to the C++
  core. Therefore, it was not (yet) implemented for this project.
* **Shared Libraries**: Other projects like the [Unreal Engine](https://www.unrealengine.com) load shared libraries (
  `.dll` on Windows, `.so` on Linux) that implement a certain interface. It is less user-friendly because developers
  must cope with the C++ programming language, but it does not require a script binding to be implemented. Furthermore,
  most libraries in this technical area in written in C++ and require such an interface to be integrated into Hive.

Although Hive foremost uses shared libraries that implement the [IPlugin](\ref hive::plugins::IPlugin) interface to load
functionality, other ways of loading plugins can be introduced in the future using
the [IPluginManager](\ref hive::plugins::IPluginManager) interface.

## Loading Plugins from Shared Libraries

Each [IPlugin](\ref hive::plugins::IPlugin) implementation must be contained in its own shared library. A plugin that
enables the core to provide distributed rendering, for instance, has a class `DistributedRenderingPlugin` which
implements [IPlugin](\ref hive::plugins::IPlugin) and is contained in `distributed-rendering-plugin.dll`.

### Plugin Routines and Overrides

A plugin attaches itself to the core infrastructure and equips it with useful functionality when loaded. When unloaded,
it has to undo this action. Therefore, there are two functions that need to be implemented:

> Have a look at the Demo-Plugin coming with this repository for a complete example. You can also use it as a template.

* The `IPlugin::Init` override is responsible for setup tasks: It registers services, jobs, properties, and other API
  objects in the core's infrastructure.
* The `IPlugin::ShutDown` override is responsible to undo every change the init routine has done. Registered services
  must be unregistered, and so on.

The implementations of these override functions are then exported into a shared library, so they can be loaded at
runtime.

### Loading Behaviors

Using the [IPluginManager](\ref hive::plugins::IPluginManager), plugins can be loaded and unloaded.

* Given a path to a directory without a **Plugin Descriptor**, it attempts to load all shared libraries contained in it
  as plugins.
* Given a path to a directory which contains a **Plugin Descriptor**, it loads all shared libraries referenced in it.
* Given a path to a shared library, it attempts to load it as plugin.

The Plugin Descriptor can look like the following:

```json
{
  "plugins": [
    "/path/to/absolute/plugin.so",
    "path/to/relative/plugin.so"
  ]
}
```