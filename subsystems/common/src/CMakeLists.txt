add_library(common
        common.cpp
        BoostUuidGenerator.cpp
        Subsystems.cpp
        ../include/common/assert/Assert.h
        ../include/common/config/Configuration.h
)
target_include_directories(common PUBLIC ../include ${Boost_INCLUDE_DIRS})
target_link_libraries(common PUBLIC ${Boost_LIBRARIES})