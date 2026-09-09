# Autoroute package CMakeLists

add_library(freerouting_autoroute STATIC
    AutorouteAttemptResult.cpp
    AutorouteAttemptState.cpp
    AutorouteControl.cpp
    AutorouteEngine.cpp
    BatchAutorouter.cpp
    BoardUpdateStrategy.cpp
    Connection.cpp
    DestinationDistance.cpp
    ExpansionDrill.cpp
    ExpansionRoom.cpp
    InsertFoundConnectionAlgo.cpp
    ItemAutorouteInfo.cpp
    ItemRouteResult.cpp
    ItemSelectionStrategy.cpp
    LocateFoundConnectionAlgo.cpp
    MazeSearchAlgo.cpp
    MazeSearchElement.cpp
    MazeSearchRoute.cpp
    NamedAlgorithmType.cpp
    PerformanceProfiler.cpp
    RoutingFailureLog.cpp
    TaskState.cpp
)

# Not yet compiled (blocked on unported Java dependencies or stale drafts):
#   BoardHistoryEntry.cpp            -> settings/RouterScoringSettings, core/scoring (not ported)
#   DrillPageArray.cpp               -> DrillPage (Java only)
#   IncompleteFreeSpaceExpansionRoom.cpp  -> FreeSpaceExpansionRoom draft (canonical lives in ExpansionRoom.h)
#   OptimizeRouteTask.cpp            -> BatchOptimizer/BatchOptimizerMultiThreaded (Java only)
#   TargetItemExpansionDoor.cpp      -> shared_ptr-based draft conflicting with ExpansionRoom.h

target_link_libraries(freerouting_autoroute PUBLIC freerouting_core freerouting_board)