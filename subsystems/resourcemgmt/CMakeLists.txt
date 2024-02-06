cmake_minimum_required(VERSION 3.10)
project(resourcemgmt)

find_package(Boost REQUIRED COMPONENTS fiber)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(resourcemgmt SHARED
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

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(resourcemgmt PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS resourcemgmt
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include
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