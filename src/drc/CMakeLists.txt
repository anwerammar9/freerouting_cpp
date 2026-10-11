# DRC package CMakeLists

add_library(freerouting_drc STATIC
    ClearanceViolation.cpp
    UnconnectedItems.cpp
    ZoneIslandViolation.cpp
    DrcReport.cpp
    DrcSummaryResponse.cpp
    DesignRulesChecker.cpp
)

target_link_libraries(freerouting_drc PUBLIC freerouting_core freerouting_board freerouting_constants freerouting_interactive)