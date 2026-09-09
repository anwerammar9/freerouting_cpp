# Geometry/Planar package CMakeLists
# Planar geometry primitives

set(GEOMETRY_PLANAR_SOURCES
    Direction.cpp
    Ellipse.cpp
    FloatLine.cpp
    FortyfiveDegreeBoundingDirections.cpp
    FortyfiveDegreeDirection.cpp
    Geometry.cpp
    IntBox.cpp
    IntOctagon.cpp
    Limits.cpp
    LineSegment.cpp
    OrthogonalBoundingDirections.cpp
    Point.cpp
    Polygon.cpp
    PolygonShape.cpp
    Polyline.cpp
    Shape.cpp
    ShapeBoundingDirections.cpp
    Simplex.cpp
    Side.cpp
    TileShape.cpp
    Vector.cpp
)

# Tests
set(GEOMETRY_PLANAR_TESTS
    Limits_test.cpp
)

add_library(freerouting_geometry_planar OBJECT ${GEOMETRY_PLANAR_SOURCES})
target_link_libraries(freerouting_geometry_planar PUBLIC freerouting_core)