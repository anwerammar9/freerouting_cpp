# Geometry/Planar package CMakeLists
# Planar geometry primitives

set(GEOMETRY_PLANAR_SOURCES
    Direction.cpp
    FloatLine.cpp
    Geometry.cpp
    IntBox.cpp
    Limits.cpp
    Line.cpp
    LineSegment.cpp
    Point.cpp
    PolygonShape.cpp
    Polyline.cpp
    Shape.cpp
    Side.cpp
    Vector.cpp
)

# Tests
set(GEOMETRY_PLANAR_TESTS
    Limits_test.cpp
    Line_test.cpp
)

add_library(freerouting_geometry_planar OBJECT ${GEOMETRY_PLANAR_SOURCES})
target_link_libraries(freerouting_geometry_planar PUBLIC freerouting_core)