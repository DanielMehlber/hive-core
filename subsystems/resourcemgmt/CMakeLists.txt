cmake_minimum_required(VERSION 3.10)
project(resourcemgmt)

find_package(Boost REQUIRED COMPONENTS fiber)

# build sources
add_library(resourcemgmt STATIC
        src/ThreadPoolResourceManager.cpp
        src/FileLoader.cpp
)

target_include_directories(resourcemgmt PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(resourcemgmt PUBLIC
        ${Boost_LIBRARIES}
        logging
        jobsystem
        common
)

# build test
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(resourcemgmtdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)