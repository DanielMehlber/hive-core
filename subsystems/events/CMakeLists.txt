cmake_minimum_required(VERSION 3.23)
project(hive-events)

find_package(Boost REQUIRED CONFIG COMPONENTS fiber context)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(hive-events SHARED
        src/JobBasedEventBroker.cpp
)

add_library(Hive::events ALIAS hive-events)

target_include_directories(hive-events PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(hive-events PUBLIC
        ${Boost_LIBRARIES}
        hive-logging
        hive-jobsystem
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(hive-events PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS hive-events
        EXPORT hiveEventsTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT hiveEventsTargets
        FILE hive-events-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(eventsdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)