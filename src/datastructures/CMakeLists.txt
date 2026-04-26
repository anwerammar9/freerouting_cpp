# Data Structures package CMakeLists

add_library(freerouting_datastructures STATIC
    Signum.cpp
    UndoableObjects.cpp
    Stoppable.cpp
    TimeLimit.cpp
)

target_link_libraries(freerouting_datastructures PUBLIC freerouting_core)