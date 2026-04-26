# Autoroute package CMakeLists

add_library(freerouting_autoroute STATIC
    AutorouteAttemptResult.cpp
    AutorouteAttemptState.cpp
    Connection.cpp
    ItemAutorouteInfo.cpp
    MazeSearchElement.cpp
    NamedAlgorithmType.cpp
    TaskState.cpp
)

target_link_libraries(freerouting_autoroute PUBLIC freerouting_core freerouting_board)