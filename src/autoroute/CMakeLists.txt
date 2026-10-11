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
    DrillPage.cpp
    DrillPageArray.cpp
    ExpansionDrill.cpp
    ExpansionRoom.cpp
    FoundConnectionInserter.cpp
    FoundConnectionLocator.cpp
    FoundConnectionLocator45Degree.cpp
    FoundConnectionLocatorAnyAngle.cpp
    ItemAutorouteInfo.cpp
    ItemRouteResult.cpp
    ItemSelectionStrategy.cpp
    LocateFoundConnectionAlgo.cpp
    MazeRandom.h
    MazeRipupResolver.cpp
    MazeSearchAlgo.cpp
    MazeExpansionEngine.cpp
    MazeFanoutDiagnostics.cpp
    MazeSearchElement.cpp
    MazeSearchRoute.cpp
    MazeTraceShover.cpp
    NamedAlgorithm.cpp
    NamedAlgorithmType.cpp
    PerformanceProfiler.cpp
    RoutingBoardAutoroute.cpp
    RoutingBoardFanout.cpp
    RoutingFailureLog.cpp
    Sorted45DegreeRoomNeighbours.cpp
    SortedOrthogonalRoomNeighbours.cpp
    SortedRoomNeighbours.cpp
    TaskState.cpp
)

# Not yet compiled (blocked on unported Java dependencies or stale drafts):
#   BoardHistoryEntry.cpp            -> settings/RouterScoringSettings, core/scoring (not ported)
#   IncompleteFreeSpaceExpansionRoom.cpp  -> FreeSpaceExpansionRoom draft (canonical lives in ExpansionRoom.h)
#   OptimizeRouteTask.cpp            -> BatchOptimizer/BatchOptimizerMultiThreaded (Java only)
#   TargetItemExpansionDoor.cpp      -> shared_ptr-based draft conflicting with ExpansionRoom.h

target_link_libraries(freerouting_autoroute PUBLIC freerouting_core freerouting_board)