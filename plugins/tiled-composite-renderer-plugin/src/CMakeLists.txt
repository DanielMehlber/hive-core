add_library(tiled-composite-renderer-plugin SHARED
        TiledCompositeRendererPlugin.cpp
        TiledCompositeRenderer.cpp
)

install(TARGETS tiled-composite-renderer-plugin 
    RUNTIME DESTINATION bin/plugins
    LIBRARY DESTINATION bin/plugins
    ARCHIVE DESTINATION lib
)

target_include_directories(tiled-composite-renderer-plugin PUBLIC ../include)
target_link_libraries(tiled-composite-renderer-plugin PUBLIC ${Boost_LIBRARIES} plugins)