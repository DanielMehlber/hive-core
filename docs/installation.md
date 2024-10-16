# Installation

[TOC]

## Building Hive from Source

---

### Short CMake introduction

CMake is a cross-platform build system generator. It generates project files for various build systems like Make, Ninja,
or
Visual Studio. It is used to configure the build process of Hive.

> This is not a CMake tutorial. If you are not familiar with CMake, please refer to
> the [official documentation](https://cmake.org/cmake/help/latest/guide/user-interaction/index.html).

In CMake, there are two environment variables that can be set to control the build process:

* `CMAKE_INSTALL_PREFIX` defines where the compiled binaries, libraries, and headers of Hive will be installed.
* `CMAKE_PREFIX_PATH` defines where CMake will look for dependencies. Settings this is optional. Normally, CMake will
  look in
  the default system paths
  listed [here](https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_PREFIX_PATH.html#variable:CMAKE_SYSTEM_PREFIX_PATH).

You can set them using `cmake -DCMAKE_INSTALL_PREFIX=<your install location> -DCMAKE_PREFIX_PATH=<your prefix path> ..`.

If there are version conflicts with other libraries, it is advised to install Hive's dependencies into a non-default
location and set `CMAKE_PREFIX_PATH` accordingly.

---

Hive itself requires the following libraries:

* Boost 1.83 as **shared libraries** (this can be selected at installation time)
* Vulkan SDK
* Vulkan Scene Graph (VSG) as **shared libraries**
* GTest

Dependencies are **required as shared libraries** (not statically linked ones) because Hive consists of many modules
using the same dependencies. Using static libraries can lead to undefined behavior between modules. So don't do it.

Make sure to have them installed to `CMAKE_PREFIX_PATH` or the default location for your
platform [here](https://cmake.org/cmake/help/latest/variable/CMAKE_SYSTEM_PREFIX_PATH.html#variable:CMAKE_SYSTEM_PREFIX_PATH).

### Installing Boost using `boostrap.sh` and `b2`

This is a bit tricky because Boost uses its own build system, called `b2`. Yes, I'm not happy about it either, but here
we go:

1. Download the required Boost sources [from the official site](https://www.boost.org/users/download/) as archive.
2. Extract the archive to a directory of your choice.
3. Execute the `bootstrap.sh` or `bootstrap.bat` script in the extracted directory (depending on your platform). This
   will setup the `b2` build system.
4. Run `./b2 install --prefix=<your install location> link=shared`. This may take a while.
5. Congrats! You have successfully built the boost libraries.

The `--prefix` flag is optional, but it is recommended to install the libraries into a non-default location. This way,
we can avoid version clashes. When set, it should be the same as `CMAKE_PREFIX_PATH` because this is where CMake will
look for Hive's dependencies.

### Installing Vulkan SDK

The Vulkan SDK can be downloaded from the [official LunarG website](https://vulkan.lunarg.com/sdk/home). Just install it
on your system. It brings required Vulkan drivers (if not already installed).

### Installing Vulkan Scene Graph (VSG)

Vulkan Scene Graph is a library that simplifies the usage of Vulkan.

1. Its sources can be downloaded from the [official GitHub page](https://github.com/vsg-dev/VulkanSceneGraph). Stick to
   the releases.
2. Build it using CMake and install it (optionally to `CMAKE_PREFIX_PATH`). Make sure to build it as shared library.

### Configure, Generate, and Install Hive using CMake

1. Configure and generate the project files _the standard way_, If you don't know how to use CMake, please
   refer to the [official documentation](https://cmake.org/cmake/help/latest/guide/user-interaction/index.html).
2. Build the project using the generated build system.
3. Install the project using the generated build system. It will be installed to default locations on your platform or
   to `CMAKE_INSTALL_PREFIX` if set.

## Use Hive as library or for plugin development

After you Built and Installed Hive successfully, it can be found using `find_package` by other CMake projects.

> **Pro-Tip**: Take a look at the demo plugin which is included in the Hive repository. You can use it as template.

```cmake
# pull the entire Hive core framework
find_package(Hive REQUIRED COMPONENTS core)

target_link_libraries(my_project
        Hive::core
)
```

Remember to set `CMAKE_PREFIX_PATH` to the location where Hive is installed, if it hasn't been installed to the default
location of your platform.

