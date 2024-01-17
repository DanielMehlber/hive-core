cmake_minimum_required(VERSION 3.10)
project(tiled-composite-renderer-plugin)

find_package(Boost REQUIRED)

add_subdirectory(src)

find_package(Doxygen QUIET)

if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(tiled-composite-renderer-plugin-docs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)