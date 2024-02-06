cmake_minimum_required(VERSION 3.10)
project(logging)

find_package(Boost REQUIRED COMPONENTS log log_setup system)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(logging SHARED
        src/logging/logger/impl/BoostLogger.cpp
        src/logging/LogManager.cpp
)

target_include_directories(logging PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(logging PUBLIC
        ${Boost_LIBRARIES}
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(logging PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS logging
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include
)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(loggingdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)
