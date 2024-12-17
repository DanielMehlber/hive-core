cmake_minimum_required(VERSION 3.23)
project(hive-services)

find_package(Boost REQUIRED CONFIG COMPONENTS log)

# *** required for Windows ***
# MSVC would define a marco called 'max' in windows.h otherwise,
# clashing with some boost variable & function names.
add_compile_definitions(NOMINMAX)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(services SHARED
        src/ServiceRequest.cpp
        src/ServiceResponse.cpp
        src/LocalServiceExecutor.cpp
        src/RoundRobinServiceCaller.cpp
        src/local-only/LocalOnlyServiceRegistry.cpp
        src/remote/RemoteServiceRegistrationConsumer.cpp
        src/remote/RemoteServiceExecutor.cpp
        src/remote/RemoteServiceResponseConsumer.cpp
        src/remote/RemoteServiceMessagesConverter.cpp
        src/p2p/PeerToPeerServiceRegistry.cpp
        src/remote/RemoteServiceRequestConsumer.cpp
)

add_library(Hive::services ALIAS services)
set_target_properties(services PROPERTIES OUTPUT_NAME "hive-services")

target_include_directories(services PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(services PUBLIC
        ${Boost_LIBRARIES}
        logging
        jobsystem
        events
        common
        networking
        resources
        data
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(services PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS services
        EXPORT hiveServicesTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include/hive
)

install(EXPORT hiveServicesTargets
        FILE hive-services-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(servicesdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)