cmake_minimum_required(VERSION 3.10)

project(jobsystem)

find_package(Boost REQUIRED COMPONENTS
        system
        context
        fiber
        filesystem
)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(jobsystem SHARED
        src/Job.cpp
        src/JobManager.cpp
        src/SingleThreadedExecutionImpl.cpp
        src/TimerJob.cpp
        src/FiberExecutionImpl.cpp)

target_include_directories(jobsystem PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(jobsystem
        ${Boost_LIBRARIES}
        common
        logging
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(jobsystem PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS jobsystem
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
        doxygen_add_docs(jobsystemdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)