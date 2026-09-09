# DesignForms package CMakeLists
# Specctra DSN/SES file format parsing and writing

add_library(freerouting_designforms STATIC
    specctra/Circle.cpp
    specctra/Keyword.cpp
    specctra/Library.cpp
    specctra/Network.cpp
    specctra/Placement.cpp
    specctra/Polygon.cpp
    specctra/Scanner.cpp
    specctra/SessionWriter.cpp
    specctra/Structure.cpp
)

target_link_libraries(freerouting_designforms PUBLIC
    freerouting_logger
    freerouting_board
    freerouting_geometry
)