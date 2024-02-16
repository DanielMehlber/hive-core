cmake_minimum_required(VERSION 3.10)
project(properties)

find_package(Boost REQUIRED COMPONENTS)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(properties SHARED
        src/PropertyChangeListener.cpp
        src/PropertyProvider.cpp
)

target_include_directories(properties PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(properties PUBLIC
        ${Boost_LIBRARIES}
        logging
        jobsystem
        events
        common
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(properties PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS properties
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include
)

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(propertiesdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)