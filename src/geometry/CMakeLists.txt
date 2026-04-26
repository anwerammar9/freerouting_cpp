# Geometry package CMakeLists

add_library(freerouting_geometry STATIC
    planar/Direction.cpp
    planar/Geometry.cpp
    planar/IntBox.cpp
    planar/Limits.cpp
    planar/Point.cpp
    planar/Vector.cpp
    planar/Shape.cpp
    planar/Polyline.cpp
    planar/FloatLine.cpp
    planar/PolygonShape.cpp
)

target_link_libraries(freerouting_geometry PUBLIC freerouting_core)