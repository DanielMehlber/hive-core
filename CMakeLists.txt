cmake_minimum_required(VERSION 3.10)
project(simulation-framework)

enable_testing()

option(GENERATE_DOCS "Run Doxygen to build the documentation" OFF)

# necessary for building shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif ()

# *** required for Windows ***
# MSVC would define a marco called 'max' in windows.h otherwise,
# clashing with some boost variable & function names.
add_compile_definitions(NOMINMAX)

# *** required for Windows ***
# else the error "WinSock has already been included" occurs when using boost asio.
add_compile_definitions(WIN32_LEAN_AND_MEAN)

# activate mathematical constants on older compilers
add_compile_definitions(_USE_MATH_DEFINES)

add_subdirectory(subsystems)
add_subdirectory(kernel)
add_subdirectory(plugins)
add_subdirectory(standalone)