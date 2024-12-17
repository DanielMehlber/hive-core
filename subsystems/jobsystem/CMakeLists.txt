cmake_minimum_required(VERSION 3.23)
project(hive-jobsystem)

# Set a configuration option for selecting the implementation
set(JOB_EXECUTION_IMPL "Boost.Fiber" CACHE STRING "Select the Job Execution implementation")

# Provide the user with possible values for the variable
set_property(CACHE JOB_EXECUTION_IMPL PROPERTY STRINGS "Boost.Fiber")

set(SOURCES
        src/Job.cpp
        src/JobManager.cpp
        src/TimerJob.cpp
)

IF (JOB_EXECUTION_IMPL STREQUAL "Boost.Fiber")
    message(STATUS "Using Boost.Fiber implementation for Job System")

    find_package(Boost REQUIRED CONFIG COMPONENTS
            system
            context
            fiber
            filesystem
    )

    set(SOURCES ${SOURCES}
            src/Boost/BoostFiberExecution.cpp
            src/Boost/BoostFiberRecursiveJobMutex.cpp
            src/Boost/BoostFiberJobMutex.cpp
    )
ENDIF ()

# disable some optimizations because they cause known bugs on MSVC (Windows)
# see https://www.boost.org/doc/libs/1_77_0/libs/context/doc/html/context/overview.html
IF (MSVC)
    # Add the /GL- flag to disable global program optimization
    string(REPLACE "/GL" "/GL-" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

    # Replace /EHsc with /EHs
    string(REPLACE "/EHsc" "/EHs" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
ENDIF ()

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(jobsystem SHARED ${SOURCES})

add_library(Hive::jobsystem ALIAS jobsystem)
set_target_properties(jobsystem PROPERTIES OUTPUT_NAME "hive-jobsystem")

target_include_directories(jobsystem PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
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
        EXPORT hiveJobsystemTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT hiveJobsystemTargets
        FILE hive-jobsystem-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
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