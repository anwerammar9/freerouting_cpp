# Management package CMakeLists

add_library(freerouting_management STATIC
    HeadlessBoardManager.cpp
    SessionManager.cpp
    TextManager.cpp
)

target_link_libraries(freerouting_management PUBLIC freerouting_core freerouting_logger freerouting_designforms freerouting_board)