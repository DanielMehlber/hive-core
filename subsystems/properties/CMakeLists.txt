cmake_minimum_required(VERSION 3.10)
project(properties)

find_package(Boost REQUIRED COMPONENTS)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(properties SHARED
        src/PropertyChangeListener.cpp
        src/PropertyProvider.cpp
)

add_library(Hive::properties ALIAS properties)

target_include_directories(properties PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}>"
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(properties PUBLIC
        ${Boost_LIBRARIES}
        logging
        jobsystem
        events
        common
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(properties PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS properties
        EXPORT propertiesTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include
)

install(EXPORT propertiesTargets
        FILE hive-properties-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(propertiesdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)