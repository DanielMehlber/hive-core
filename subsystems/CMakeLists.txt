cmake_minimum_required(VERSION 3.10)
project(subsystems)

enable_testing()

add_subdirectory(common)
add_subdirectory(logging)
add_subdirectory(jobsystem)
add_subdirectory(events)
add_subdirectory(networking)
add_subdirectory(properties)
add_subdirectory(resources)
add_subdirectory(services)
add_subdirectory(scene)
add_subdirectory(graphics)
add_subdirectory(plugins)

find_package(Doxygen QUIET OPTIONAL_COMPONENTS mscgen dia dot)

if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        SET(DOXYGEN_EXCLUDE_PATTERNS "*/test/*")
        SET(DOXYGEN_PROJECT_NAME "Core Subsystems")
        doxygen_add_docs(subsystemsdocs "." ALL)
    else (DOXYGEN_FOUND)
        message("Doxygen need to be installed to generate the doxygen documentation")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)