# Interactive package CMakeLists

add_library(freerouting_interactive STATIC
    RatsNestItemInfo.cpp
)

target_link_libraries(freerouting_interactive PUBLIC freerouting_core freerouting_board)