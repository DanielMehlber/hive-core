cmake_minimum_required(VERSION 3.10)
project(kernel)

find_package(Boost COMPONENTS log system fiber context REQUIRED)

add_subdirectory(src)

find_package(Doxygen QUIET)

if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(kerneldocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)