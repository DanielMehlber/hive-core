cmake_minimum_required(VERSION 3.10)
project(standalone)

find_package(Boost REQUIRED COMPONENTS program_options log)

add_subdirectory(src)

find_package(Doxygen QUIET)

if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(standalonedocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)