cmake_minimum_required(VERSION 3.23)
project(hive-data)

find_package(Boost REQUIRED CONFIG COMPONENTS)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)


add_library(data SHARED
        src/FunctionalChangeListener.cpp
        src/DataLayer.cpp

        # Boost implementation
        src/Boost/BoostLocalDataProvider.cpp
)

add_library(Hive::data ALIAS data)
set_target_properties(data PROPERTIES OUTPUT_NAME "hive-data")

target_include_directories(data PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(data PUBLIC
        ${Boost_LIBRARIES}
        logging
        jobsystem
        common
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(data PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS data
        EXPORT hiveDataTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT hiveDataTargets
        FILE hive-data-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(datadocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)