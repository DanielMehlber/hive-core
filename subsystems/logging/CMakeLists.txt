cmake_minimum_required(VERSION 3.10)
project(logging)

find_package(Boost REQUIRED)

add_subdirectory(src)

find_package(Doxygen REQUIRED dot)

if(GENERATE_DOCS)
    if(DOXYGEN_FOUND)
        doxygen_add_docs(loggingdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif(GENERATE_DOCS)