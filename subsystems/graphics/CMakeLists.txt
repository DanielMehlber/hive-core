cmake_minimum_required(VERSION 3.10)
project(graphics)

find_package(Boost REQUIRED COMPONENTS log system iostreams)
find_package(vsg REQUIRED)
find_package(ZLIB)

add_subdirectory(src)
add_subdirectory(test)

find_package(Doxygen QUIET)

if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(graphicsdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)