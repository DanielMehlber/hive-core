cmake_minimum_required(VERSION 3.23)
project(hive-scene)

find_package(Boost REQUIRED CONFIG COMPONENTS log)
find_package(vsg REQUIRED)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(scene SHARED
        src/SceneManager.cpp
)

add_library(Hive::scene ALIAS scene)
set_target_properties(scene PROPERTIES OUTPUT_NAME "hive-scene")

target_include_directories(scene PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(scene PUBLIC
        ${Boost_LIBRARIES}
        logging
        vsg::vsg
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(scene PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS scene
        EXPORT hiveSceneTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT hiveSceneTargets
        FILE hive-scene-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(scenedocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)