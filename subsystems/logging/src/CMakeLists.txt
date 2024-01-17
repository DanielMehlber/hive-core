add_library(logging STATIC logging/logger/impl/BoostLogger.cpp logging/LogManager.cpp
        
)

target_include_directories(logging PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(logging PUBLIC ${Boost_LIBRARIES})