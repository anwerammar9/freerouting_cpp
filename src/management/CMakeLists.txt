# Management package CMakeLists

add_library(freerouting_management STATIC
    SessionManager.cpp
    TextManager.cpp
)

target_link_libraries(freerouting_management PUBLIC freerouting_core freerouting_logger)