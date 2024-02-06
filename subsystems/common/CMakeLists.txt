cmake_minimum_required(VERSION 3.10)
project(common)

find_package(Boost REQUIRED)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(common SHARED
        src/common.cpp
        src/BoostUuidGenerator.cpp
        src/Subsystems.cpp
        src/Timer.cpp
        src/TimerManager.cpp
)

target_include_directories(common PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(common PUBLIC ${Boost_LIBRARIES})

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(common PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS common
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
        doxygen_add_docs(commondocs " include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)