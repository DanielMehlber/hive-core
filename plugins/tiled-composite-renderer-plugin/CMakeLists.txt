cmake_minimum_required(VERSION 3.10)
project(tiled-composite-renderer-plugin)

find_package(Boost REQUIRED)

# build sources
add_library(tiled-composite-renderer-plugin SHARED
        src/TiledCompositeRendererPlugin.cpp
        src/TiledCompositeRenderer.cpp
)

install(TARGETS tiled-composite-renderer-plugin
        RUNTIME DESTINATION bin/plugins
        LIBRARY DESTINATION bin/plugins
        ARCHIVE DESTINATION lib
)

target_include_directories(tiled-composite-renderer-plugin PUBLIC
        include
)

target_link_libraries(tiled-composite-renderer-plugin
        PUBLIC
        ${Boost_LIBRARIES}
        plugins
)

# create documentation using doxygen (if doxygen is installed and documentation is requested)
find_package(Doxygen QUIET)
if (GENERATE_DOCS)
    if (DOXYGEN_FOUND)
        doxygen_add_docs(tiled-composite-renderer-plugin-docs "include" "src" ALL)
    else (DOXYGEN_FOUND)
        message(WARNING "Doxygen need to be installed to generate the doxygen documentation of ${PROJECT_NAME}.")
    endif (DOXYGEN_FOUND)
endif (GENERATE_DOCS)