cmake_minimum_required(VERSION 3.20)
project(freerouting VERSION 2.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Testing - enabled for DSN parsing tests
option(BUILD_TESTS "Build unit tests" ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

find_package(Threads REQUIRED)

# ============== Core Library ==============
add_library(freerouting_core STATIC
    src/datastructures/Signum.cpp
    src/datastructures/UndoableObjects.cpp
    src/logger/FRLogger.cpp
    src/core/RoutingJob.cpp
)

# ============== Geometry Library ==============
add_library(freerouting_geometry STATIC
    src/geometry/planar/Geometry.cpp
    src/geometry/planar/Shape.cpp
    src/geometry/planar/PolygonShape.cpp
    src/geometry/planar/IntBox.cpp
    src/geometry/planar/Vector.cpp
    src/geometry/planar/LineSegment.cpp
    src/geometry/planar/Polyline.cpp
    src/geometry/planar/Direction.cpp
    src/geometry/planar/FloatLine.cpp
    src/geometry/planar/Side.cpp
    src/geometry/planar/Point.cpp
)

target_link_libraries(freerouting_geometry PUBLIC freerouting_core)

# ============== Rules Library ==============
add_library(freerouting_rules STATIC
    src/rules/DefaultItemClearanceClasses.cpp
    src/rules/ClearanceMatrix.cpp
    src/rules/ViaRule.cpp
    src/rules/NetClass.cpp
    src/rules/Net.cpp
    src/rules/BoardRules.cpp
)

target_link_libraries(freerouting_rules PUBLIC freerouting_core freerouting_geometry)

# ============== Board Library ==============
add_library(freerouting_board STATIC
    src/board/Layer.cpp
    src/board/Item.cpp
    src/board/Via.cpp
    src/board/Padstack.cpp
    src/board/Pin.cpp
    src/board/Component.cpp
    src/board/Components.cpp
    src/board/DrillItem.cpp
    src/board/CoordinateTransform.cpp
    src/board/BasicBoard.cpp
    src/board/BoardBuilder.cpp
    src/board/RoutingBoard.cpp
    src/board/Communication.cpp
    src/board/ChangedArea.cpp
    src/board/ShapeSearchTree.cpp
    src/board/SearchTreeManager.cpp
)

target_link_libraries(freerouting_board PUBLIC freerouting_core freerouting_core_lib freerouting_geometry freerouting_rules)

# ============== Core Library ==============
add_library(freerouting_core_lib STATIC
    src/core/Package.cpp
)

target_link_libraries(freerouting_core_lib PUBLIC freerouting_core freerouting_board)

# ============== DRC Library ==============
add_library(freerouting_drc STATIC
    src/drc/ClearanceViolation.cpp
)

target_link_libraries(freerouting_drc PUBLIC freerouting_core freerouting_board)

# ============== Autoroute Library ==============
add_library(freerouting_autoroute STATIC
    src/autoroute/AutorouteEngine.cpp
    src/autoroute/AutorouteControl.cpp
    src/autoroute/BatchAutorouter.cpp
    src/autoroute/MazeSearchElement.cpp
    src/autoroute/MazeSearchAlgo.cpp
    src/autoroute/MazeSearchRoute.cpp
    src/autoroute/ExpansionRoom.cpp
    src/autoroute/DestinationDistance.cpp
    src/autoroute/AutorouteAttemptResult.cpp
    src/autoroute/ItemAutorouteInfo.cpp
    src/autoroute/Connection.cpp
    src/autoroute/InsertFoundConnectionAlgo.cpp
    src/autoroute/LocateFoundConnectionAlgo.cpp
)

target_link_libraries(freerouting_autoroute PUBLIC freerouting_core freerouting_board)

# ============== Settings Library ==============
add_library(freerouting_settings STATIC
    src/settings/RouterSettings.cpp
)

target_link_libraries(freerouting_settings PUBLIC freerouting_core freerouting_autoroute)

# ============== DesignForms Library ==============
add_library(freerouting_designforms STATIC
    src/designforms/specctra/Scanner.cpp
    src/designforms/specctra/Keyword.cpp
    src/designforms/specctra/Structure.cpp
    src/designforms/specctra/Library.cpp
    src/designforms/specctra/Network.cpp
    src/designforms/specctra/Placement.cpp
    src/designforms/specctra/SessionWriter.cpp
)

target_link_libraries(freerouting_designforms PUBLIC freerouting_core freerouting_core_lib freerouting_board)

# ============== Main Executable ==============
add_executable(freerouting
    src/main.cpp
)

target_link_libraries(freerouting PRIVATE
    freerouting_core
    freerouting_geometry
    freerouting_rules
    freerouting_board
    freerouting_core_lib
    freerouting_designforms
    freerouting_drc
    freerouting_autoroute
    freerouting_settings
    Threads::Threads
)

# Testing - enabled
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()