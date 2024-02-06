cmake_minimum_required(VERSION 3.10)
project(scene)

find_package(Boost REQUIRED COMPONENTS log)
find_package(vsg REQUIRED)

# build sources
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(scene SHARED
        src/SceneManager.cpp
)

target_include_directories(scene PUBLIC
        include
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(scene PUBLIC
        ${Boost_LIBRARIES}
        logging
        vsg::vsg
)

# install
file(GLOB_RECURSE HEADER_FILES "include/**/*.h")

target_sources(scene PUBLIC FILE_SET HEADERS
        BASE_DIRS include
        FILES ${HEADER_FILES})

install(TARGETS scene
        RUNTIME DESTINATION lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        FILE_SET HEADERS DESTINATION include
)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(scenedocs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)