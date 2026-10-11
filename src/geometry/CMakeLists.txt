# Geometry package CMakeLists

add_library(freerouting_geometry STATIC
    planar/CircleArea.cpp
    planar/Direction.cpp
    planar/Ellipse.cpp
    planar/FortyfiveDegreeBoundingDirections.cpp
    planar/FortyfiveDegreeDirection.cpp
    planar/Geometry.cpp
    planar/IntBox.cpp
    planar/IntOctagon.cpp
    planar/Limits.cpp
    planar/LineSegment.cpp
    planar/OrthogonalBoundingDirections.cpp
    planar/Point.cpp
    planar/Polygon.cpp
    planar/Rational.cpp
    planar/Vector.cpp
    planar/Shape.cpp
    planar/ShapeBoundingDirections.cpp
    planar/Side.cpp
    planar/Simplex.cpp
    planar/TileShape.cpp
    planar/ConvexTileShape.cpp
    planar/Polyline.cpp
    planar/PolylineShape.cpp
    planar/FloatLine.cpp
    planar/PolygonShape.cpp
    planar/PolylineArea.cpp
)

target_link_libraries(freerouting_geometry PUBLIC freerouting_datastructures freerouting_core Boost::headers)