# Settings package CMakeLists

add_library(freerouting_settings STATIC
    RouterSettings.cpp
    RuntimeEnvironment.cpp
    GlobalSettings.cpp
)

# RouterSettings.h only includes the header-only autoroute/ExpansionCostFactor.h, so there is no
# link dependency on freerouting_autoroute. Linking it would make freerouting_designforms ->
# freerouting_settings -> freerouting_autoroute -> {designforms,...} a cycle in the RESCAN groups.
target_link_libraries(freerouting_settings PUBLIC freerouting_core freerouting_logger)
