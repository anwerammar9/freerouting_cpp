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
    ShapeSearchTree.cpp
    SearchTreeManager.cpp
    Pin.cpp
    Padstack.cpp
    Via.cpp
    BoardLoader.cpp
)

target_link_libraries(freerouting_board PUBLIC freerouting_core freerouting_rules freerouting_geometry)