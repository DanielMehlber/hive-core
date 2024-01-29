cmake_minimum_required(VERSION 3.10)
project(common)

find_package(Boost REQUIRED)

# build sources
add_library(common STATIC
        src/common.cpp
        src/BoostUuidGenerator.cpp
        src/Subsystems.cpp
)

target_include_directories(common PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(common PUBLIC ${Boost_LIBRARIES})

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(commondocs " include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)