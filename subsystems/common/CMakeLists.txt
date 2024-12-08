cmake_minimum_required(VERSION 3.23)
project(hive-common)

find_package(Boost REQUIRED CONFIG)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(hive-common SHARED
        src/BoostUuidGenerator.cpp
        src/Timer.cpp
        src/TimerManager.cpp
        src/SpinLock.cpp
)

add_library(Hive::common ALIAS hive-common)

target_include_directories(hive-common PUBLIC
        ${Boost_INCLUDE_DIRS}
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
)

target_link_libraries(hive-common PUBLIC ${Boost_LIBRARIES})

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(hive-common PUBLIC FILE_SET HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${HEADER_FILES})

install(TARGETS hive-common
        EXPORT hiveCommonTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT hiveCommonTargets
        FILE hive-common-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
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