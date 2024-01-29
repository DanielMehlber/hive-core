cmake_minimum_required(VERSION 3.10)
project(networking)

find_package(Boost REQUIRED COMPONENTS json log)

# build sources
add_library(networking STATIC
        src/UrlParser.cpp
        src/Message.cpp
        src/MessageConsumerJob.cpp
        src/MessageConverter.cpp
        src/NetworkingManager.cpp
        src/BoostWebSocketEndpoint.cpp
        src/BoostWebSocketConnection.cpp
        src/BoostWebSocketConnectionListener.cpp
        src/BoostWebSocketConnectionEstablisher.cpp
        src/MultipartFormdata.cpp
)

target_include_directories(networking PUBLIC
        include
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