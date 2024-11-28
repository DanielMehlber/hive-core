cmake_minimum_required(VERSION 3.23)
project(hive-plugins)

find_package(Boost REQUIRED CONFIG COMPONENTS log)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(hive-plugins SHARED
        src/BoostPluginManager.cpp
)

add_library(Hive::plugins ALIAS hive-plugins)

target_include_directories(hive-plugins PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(hive-plugins PUBLIC
        ${Boost_LIBRARIES}
        hive-logging
        hive-jobsystem
        hive-events
        hive-common
        hive-networking
        hive-services
        hive-resources
        hive-scene
        hive-graphics
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(hive-plugins PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS hive-plugins
        EXPORT hivePluginsTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT hivePluginsTargets
        FILE hive-plugins-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(pluginsdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)