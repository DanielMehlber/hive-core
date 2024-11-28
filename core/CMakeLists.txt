cmake_minimum_required(VERSION 3.23)
project(hive-core)

find_package(Boost COMPONENTS CONFIG REQUIRED log system fiber context)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(hive-core SHARED
        src/Core.cpp
)

add_library(Hive::core ALIAS hive-core)

target_include_directories(hive-core PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
        ${Boost_INCLUDE_DIRS}
)


target_link_libraries(hive-core PUBLIC
        ${Boost_LIBRARIES}
        hive-common
        hive-logging
        hive-jobsystem
        hive-events
        hive-networking
        hive-scene
        hive-graphics
        hive-services
        hive-resources
        hive-data
        hive-plugins
)

file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(hive-core PUBLIC FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${HEADER_FILES})

install(TARGETS hive-core
        EXPORT hiveCoreTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT hiveCoreTargets
        FILE hive-core-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(coredocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)