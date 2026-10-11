# DesignForms package CMakeLists
# Specctra DSN/SES file format parsing and writing

add_library(freerouting_designforms STATIC
    specctra/SessionWriter.cpp
    specctra/parser/AutorouteSettings.cpp
    specctra/parser/Circle.cpp
    specctra/parser/Circuit.cpp
    specctra/parser/Component.cpp
    specctra/parser/CoordinateTransform.cpp
    specctra/parser/DsnFile.cpp
    specctra/parser/Keyword.cpp
    specctra/parser/Layer.cpp
    specctra/parser/Library.cpp
    specctra/parser/MinimalBoardManager.cpp
    specctra/parser/NetClass.cpp
    specctra/parser/NetList.cpp
    specctra/parser/Network.cpp
    specctra/parser/Package.cpp
    specctra/parser/Parser.cpp
    specctra/parser/PartLibrary.cpp
    specctra/parser/Path.cpp
    specctra/parser/PlaceControl.cpp
    specctra/parser/Plane.cpp
    specctra/parser/Polygon.cpp
    specctra/parser/Rectangle.cpp
    specctra/parser/Rule.cpp
    specctra/parser/Shape.cpp
    specctra/parser/Resolution.cpp
    specctra/parser/ScopeKeyword.cpp
    specctra/parser/SpecctraDsnStreamReader.cpp
    specctra/parser/Structure.cpp
    specctra/parser/Wiring.cpp
    specctra/BoardCountsDump.cpp
    specctra/DsnReader.cpp
    specctra/parser/WriteScopeParameter.cpp
)

target_link_libraries(freerouting_designforms PUBLIC
    freerouting_logger
    freerouting_board
    freerouting_geometry
    freerouting_settings
)