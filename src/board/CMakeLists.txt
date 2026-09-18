# Board package CMakeLists

add_library(freerouting_board STATIC
    Layer.cpp
    LayerStructure.cpp
    BasicBoard.cpp
    Component.cpp
    CoordinateTransform.cpp
    Communication.cpp
    AngleRestriction.cpp
    Item.cpp
    ItemSearchTreesInfo.cpp
    ShapeSearchTree.cpp
    SearchTreeManager.cpp
    Pin.cpp
    Padstack.cpp
    Via.cpp
    ChangedArea.cpp
    BoardOutline.cpp
    RoutingBoard.cpp
    BoardLoader.cpp
    Trace.cpp
    PolylineTrace.cpp
    ObstacleArea.cpp
    ConductionArea.cpp
    ComponentObstacleArea.cpp
    ViaObstacleArea.cpp
    ItemIdentificationNumberGenerator.cpp
    ItemSelectionFilter.cpp
    CalcFromSide.cpp
    CalcShapeAndFromSide.cpp
    ComponentOutline.cpp
    BoardObserverAdaptor.cpp
    ShapeTraceEntries.cpp
    ShoveTraceAlgo.cpp
    ForcedPadAlgo.cpp
    ForcedViaAlgo.cpp
    MoveDrillItemAlgo.cpp
    MoveComponent.cpp
    OptViaAlgo.cpp
    PullTightAlgo.cpp
    PullTightAlgo90.cpp
    PullTightAlgo45.cpp
    PullTightAlgoAnyAngle.cpp
)

target_link_libraries(freerouting_board PUBLIC freerouting_core freerouting_rules freerouting_geometry)