add_library(logging logging/logger/impl/BoostLogger.cpp logging/LogManager.cpp
        ../include/logging/Logging.h
)

target_include_directories(logging PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(logging PUBLIC ${Boost_LIBRARIES})