# Rules package CMakeLists
# Design rules, clearance matrices, and network classes

set(RULES_SOURCES
    BoardRules.cpp
    ClearanceMatrix.cpp
    DefaultItemClearanceClasses.cpp
    Net.cpp
    NetClass.cpp
    ViaInfo.cpp
    ViaRule.cpp
)

add_library(freerouting_rules STATIC ${RULES_SOURCES})
target_link_libraries(freerouting_rules PUBLIC freerouting_core)