# Settings package CMakeLists

add_library(freerouting_settings STATIC
    RouterSettings.cpp
    RuntimeEnvironment.cpp
    GlobalSettings.cpp
)

target_link_libraries(freerouting_settings PUBLIC freerouting_core freerouting_autoroute)