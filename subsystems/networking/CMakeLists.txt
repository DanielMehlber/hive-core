cmake_minimum_required(VERSION 3.10)
project(networking)

find_package(Boost REQUIRED COMPONENTS json log)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(networking SHARED
        src/util/UrlParser.cpp
        src/messaging/Message.cpp
        src/messaging/MessageConsumerJob.cpp
        src/messaging/MessageConverter.cpp
        src/NetworkingManager.cpp
        src/messaging/impl/websockets/boost/BoostWebSocketEndpoint.cpp
        src/messaging/impl/websockets/boost/BoostWebSocketConnection.cpp
        src/messaging/impl/websockets/boost/BoostWebSocketConnectionListener.cpp
        src/messaging/impl/websockets/boost/BoostWebSocketConnectionEstablisher.cpp
        src/messaging/MultipartFormdata.cpp
)

add_library(Hive::networking ALIAS networking)

target_include_directories(networking PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}>"
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(networking PUBLIC
        ${Boost_LIBRARIES}
        logging
        jobsystem
        events
        common
        properties
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(networking PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS networking
        EXPORT networkingTargets
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include
)

install(EXPORT networkingTargets
        FILE hive-networking-targets.cmake
        NAMESPACE Hive::
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/Hive
)

# build tests
add_subdirectory(test)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(networkingdocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)