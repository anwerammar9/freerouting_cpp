# Scoring package CMakeLists
# Board statistics and scoring calculation

set(SCORING_SOURCES
    BoardStatistics.cpp
)

add_library(freerouting_scoring STATIC ${SCORING_SOURCES})

target_include_directories(freerouting_scoring PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(freerouting_scoring PRIVATE
    freerouting_core
    freerouting_board
    freerouting_rules
    freerouting_geometry
    freerouting_datastructures
    freerouting_logger
    freerouting_constants
    Threads::Threads
)