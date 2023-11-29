add_library(graphics
        OffscreenRenderer.cpp
        OnscreenRenderer.cpp
        RenderService.cpp
        RenderServiceRequest.cpp
        CharEscapeRenderResultEncoder.cpp
        ../include/graphics/service/encoders/impl/GzipRenderResultEncoder.h
        GzipRenderResultEncoder.cpp
)

target_include_directories(graphics PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(graphics PUBLIC ${Boost_LIBRARIES} ZLIB::ZLIB logging jobsystem common scene vsg::vsg services)