cmake_minimum_required(VERSION 3.10)
project(standalone)

find_package(Boost REQUIRED COMPONENTS program_options log)

# build sources
add_executable(standalone
        src/main.cpp
)

set_target_properties(standalone PROPERTIES
        PREFIX "hive-"
)

install(TARGETS standalone
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

target_include_directories(standalone PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(standalone PUBLIC
        ${Boost_LIBRARIES}
        core
)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(standalonedocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)