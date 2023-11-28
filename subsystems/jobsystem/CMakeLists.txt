cmake_minimum_required(VERSION 3.10)

project(jobsystem)

find_package(Boost REQUIRED COMPONENTS system context fiber filesystem)

find_package(Doxygen QUIET)

if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(jobsystemdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(test)