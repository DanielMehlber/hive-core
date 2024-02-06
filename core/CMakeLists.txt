cmake_minimum_required(VERSION 3.10)
project(core)

find_package(Boost COMPONENTS log system fiber context REQUIRED)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(core SHARED
        src/Core.cpp
)

install(TARGETS core
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

target_include_directories(core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
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
        resourcemgmt
        properties
        plugins
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