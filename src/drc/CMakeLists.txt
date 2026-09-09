# DRC package CMakeLists

add_library(freerouting_drc STATIC
    ClearanceViolation.cpp
    UnconnectedItems.cpp
    DrcReport.cpp
    DesignRulesChecker.cpp
)

target_link_libraries(freerouting_drc PUBLIC freerouting_core freerouting_board freerouting_constants)