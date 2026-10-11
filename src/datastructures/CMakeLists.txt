# Data Structures package CMakeLists

add_library(freerouting_datastructures STATIC
    Signum.cpp
    UndoableObjects.cpp
    Stoppable.cpp
    TimeLimit.cpp
    IdentifierType.cpp
    IndentFileWriter.cpp
    JavaFormat.cpp
    FileFilter.cpp
    IdentificationNumberGenerator.cpp
    PlanarDelaunayTriangulation.cpp
    ShapeTree.cpp
    MinAreaTree.cpp
)

# The Java packages datastructures and geometry reference each other
# (Signum in datastructures is used by geometry; the shape trees in
# datastructures use the geometry tiles). Static library symbols are resolved
# at the final executable link, where both libraries (and their dependencies)
# are linked explicitly, so no declared dependency edge is required here.
target_link_libraries(freerouting_datastructures PUBLIC freerouting_core Boost::headers)