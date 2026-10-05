# Core package CMakeLists
# Core enums, data structures, and fundamental classes

set(CORE_SOURCES
    FileFormat.cpp
    Package.cpp
    RoutingJob.cpp
    RoutingStage.cpp
    StopRequestState.cpp
    Session.cpp
)

add_library(freerouting_core STATIC ${CORE_SOURCES})