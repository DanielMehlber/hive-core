cmake_minimum_required(VERSION 3.10)
project(events)

find_package(Boost REQUIRED COMPONENTS fiber context)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(events SHARED
        src/JobBasedEventBroker.cpp
)

target_include_directories(events PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(events PUBLIC
        ${Boost_LIBRARIES}
        logging
        jobsystem
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(events PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS events
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include
)

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(eventsdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)