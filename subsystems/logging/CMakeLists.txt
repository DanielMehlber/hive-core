cmake_minimum_required(VERSION 3.10)
project(logging)

find_package(Boost REQUIRED COMPONENTS log log_setup system)

# build sources
add_library(logging STATIC
        src/logging/logger/impl/BoostLogger.cpp
        src/logging/LogManager.cpp
)

target_include_directories(logging PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(logging PUBLIC ${Boost_LIBRARIES})

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(loggingdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)
