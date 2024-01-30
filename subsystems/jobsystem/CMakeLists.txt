cmake_minimum_required(VERSION 3.10)

project(jobsystem)

find_package(Boost REQUIRED COMPONENTS system context fiber filesystem)

# build sources
add_library(jobsystem STATIC
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
        logging
        common
        ${Boost_Libraries}
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