# Settings package CMakeLists

add_library(freerouting_settings STATIC
    AutorouterSettings.cpp
    FanoutSettings.cpp
    LayerSettings.cpp
    OptimizerScoreSettings.cpp
    OptimizerSettings.cpp
    RouterScoreSettings.cpp
    RouterSettings.cpp
    RoutingCostSettings.cpp
    sources/DefaultSettings.cpp
    RuntimeEnvironment.cpp
    GlobalSettings.cpp
)

# RouterSettings.h only includes header-only autoroute headers (ExpansionCostFactor.h and the
# BoardUpdateStrategy / ItemSelectionStrategy enums), so there is no
# link dependency on freerouting_autoroute. Linking it would make freerouting_designforms ->
# freerouting_settings -> freerouting_autoroute -> {designforms,...} a cycle in the RESCAN groups.
target_link_libraries(freerouting_settings PUBLIC freerouting_core freerouting_logger)
