# Interactive package CMakeLists

add_library(freerouting_interactive STATIC
    RatsNestItemInfo.cpp
    NetIncompletes.cpp
    RatsNest.cpp
)

target_link_libraries(freerouting_interactive PUBLIC
    freerouting_core
    freerouting_board
    freerouting_datastructures
)