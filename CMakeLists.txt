cmake_minimum_required(VERSION 3.23)
project(hive-framework)
set(version 1.0.1)

enable_testing()

option(GENERATE_DOCS "Run Doxygen to build the documentation" OFF)
option(ENABLE_PROFILING "Measure elapsed time of various processes" OFF)

# if profiling is wished, enable it
IF (ENABLE_PROFILING)
    message("Profiling has been enabled")
    add_compile_definitions(ENABLE_PROFILING)
ENDIF ()

# necessary for building shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif ()

# *** required for Windows ***
# MSVC would define a marco called 'max' in windows.h otherwise,
# clashing with some boost variable & function names.
add_compile_definitions(NOMINMAX)

# *** required for Windows ***
# The object size in MSVC is limited by default. The network subsystem
# uses big headers from boost, so we need to increase the object size.
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/bigobj>)


# *** required for Windows ***
# else the error "WinSock has already been included" occurs when using boost asio.
add_compile_definitions(WIN32_LEAN_AND_MEAN)

# activate mathematical constants on older compilers
add_compile_definitions(_USE_MATH_DEFINES)

add_subdirectory(subsystems)
add_subdirectory(core)
add_subdirectory(standalone)
add_subdirectory(plugins)

# export steps
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/hive-config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/hive-config-version.cmake"
        VERSION "${version}"
        COMPATIBILITY AnyNewerVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/hive-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/hive-config-version.cmake"
        # "${CMAKE_CURRENT_BINARY_DIR}/hive-targets.cmake"
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

#export(EXPORT HiveTargets
#        FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/hive-targets.cmake"
#        NAMESPACE Hive::
#)