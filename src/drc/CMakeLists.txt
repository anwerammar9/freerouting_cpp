# DRC package CMakeLists

add_library(freerouting_drc STATIC
    ClearanceViolation.cpp
)

target_link_libraries(freerouting_drc PUBLIC freerouting_core freerouting_board)