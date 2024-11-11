cmake_minimum_required(VERSION 3.23)
project(graphics)

find_package(Boost REQUIRED COMPONENTS log system iostreams)
find_package(vsg REQUIRED)

# build sources
set(GRAPHICS_SOURCES
        src/OffscreenRenderer.cpp
        src/OnscreenRenderer.cpp
        src/RenderService.cpp
        src/RenderServiceRequest.cpp
)

set(GRAPHICS_DEPENDENCIES logging jobsystem common scene vsg::vsg services)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(graphics SHARED
        ${GRAPHICS_SOURCES}
)

add_library(Hive::graphics ALIAS graphics)

set_target_properties(graphics PROPERTIES
        PREFIX "hive-"
)

target_include_directories(graphics PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(graphics PUBLIC
        ${Boost_LIBRARIES}
        ${GRAPHICS_DEPENDENCIES}
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(graphics PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS graphics
        EXPORT graphicsTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT graphicsTargets
        FILE hive-graphics-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(graphicsdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)