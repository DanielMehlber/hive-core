cmake_minimum_required(VERSION 3.10)
project(plugins)

find_package(Boost REQUIRED COMPONENTS log)

# build sources
add_library(plugins STATIC
        src/BoostPluginManager.cpp
)

target_include_directories(plugins PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(plugins PUBLIC
        ${Boost_LIBRARIES}
        logging
        jobsystem
        events
        common
        networking
        services
        resourcemgmt
        scene
        graphics
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