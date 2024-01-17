
set(GRAPHICS_SOURCES OffscreenRenderer.cpp
        OnscreenRenderer.cpp
        RenderService.cpp
        RenderServiceRequest.cpp
        CharEscapeRenderResultEncoder.cpp
)

set(GRAPHICS_DEPENDENCIES logging jobsystem common scene vsg::vsg services)

if (ZLIB_FOUND)
    list(APPEND GRAPHICS_SOURCES GzipRenderResultEncoder.cpp)
    list(APPEND GRAPHICS_DEPENDENCIES ZLIB::ZLIB)
else ()
    message("Graphics module does not support Gzip encoder because ZLIB is missing")
endif ()

add_library(graphics STATIC
        ${GRAPHICS_SOURCES}
)

target_include_directories(graphics PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(graphics PUBLIC ${Boost_LIBRARIES} ${GRAPHICS_DEPENDENCIES})