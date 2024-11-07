cmake_minimum_required(VERSION 3.10)
project(core)

find_package(Boost COMPONENTS log system fiber context REQUIRED)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(core SHARED
        src/Core.cpp
)

add_library(Hive::core ALIAS core)

set_target_properties(core PROPERTIES
        PREFIX "hive-"
)

target_include_directories(core PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
        ${Boost_INCLUDE_DIRS}
)


target_link_libraries(core PUBLIC
        ${Boost_LIBRARIES}
        common
        logging
        jobsystem
        events
        networking
        scene
        graphics
        services
        resources
        data
        plugins
)

file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(core PUBLIC FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${HEADER_FILES})

install(TARGETS core
        EXPORT coreTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT coreTargets
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