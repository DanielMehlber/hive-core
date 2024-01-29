cmake_minimum_required(VERSION 3.10)
project(graphics)

find_package(Boost REQUIRED COMPONENTS log system iostreams)
find_package(vsg REQUIRED)
find_package(ZLIB)

# build sources
set(GRAPHICS_SOURCES
        src/OffscreenRenderer.cpp
        src/OnscreenRenderer.cpp
        src/RenderService.cpp
        src/RenderServiceRequest.cpp
        src/CharEscapeRenderResultEncoder.cpp
)

set(GRAPHICS_DEPENDENCIES logging jobsystem common scene vsg::vsg services)

# compile Gzip encoder only if ZLIB library is installed.
if (ZLIB_FOUND)
    list(APPEND GRAPHICS_SOURCES src/GzipRenderResultEncoder.cpp)
    list(APPEND GRAPHICS_DEPENDENCIES ZLIB::ZLIB)
else ()
    message(WARNING "Graphics module does not support Gzip encoder because ZLIB is missing")
endif ()

add_library(graphics STATIC
        ${GRAPHICS_SOURCES}
)

target_include_directories(graphics PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(graphics PUBLIC
        ${Boost_LIBRARIES}
        ${GRAPHICS_DEPENDENCIES}
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