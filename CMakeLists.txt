cmake_minimum_required(VERSION 3.20)
project(freerouting VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

option(BUILD_TESTS "Build unit tests" ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

find_package(Threads REQUIRED)

# ============== Dependencies (installed via Conan, see conanfile.txt) ==============
# Boost is used header-only (boost::multiprecision::cpp_int) by the geometry
# rational types (RationalPoint / RationalVector / BigIntDirection).
find_package(Boost REQUIRED)

# ============== Include subdirectories for each package ==============
add_subdirectory(src/core)
add_subdirectory(src/core/scoring)
add_subdirectory(src/datastructures)
add_subdirectory(src/logger)
add_subdirectory(src/rules)
add_subdirectory(src/geometry)
add_subdirectory(src/board)
add_subdirectory(src/drc)
add_subdirectory(src/interactive)
add_subdirectory(src/autoroute)
add_subdirectory(src/settings)
add_subdirectory(src/constants)
add_subdirectory(src/management)

# Empty packages (not yet migrated)
# add_subdirectory(src/boardgraphics)

add_subdirectory(src/designforms)

# ============== Main Executable ==============
add_executable(freerouting src/main.cpp src/Freerouting.cpp)

target_link_libraries(freerouting PRIVATE
    freerouting_core
    freerouting_datastructures
    freerouting_logger
    freerouting_rules
    freerouting_geometry
    freerouting_board
    freerouting_interactive
    freerouting_autoroute
    freerouting_settings
    freerouting_constants
    freerouting_management
    freerouting_designforms
    freerouting_scoring
    Threads::Threads
)

# ============== Test ==============
if(BUILD_TESTS)
    enable_testing()

    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_executable(test_main src/test_main.cpp)
    target_link_libraries(test_main PRIVATE freerouting_core freerouting_logger freerouting_constants)
    add_test(NAME test_main COMMAND test_main)

    add_subdirectory(tests)
endif()