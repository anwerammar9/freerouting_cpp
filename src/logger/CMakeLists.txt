# Logger package CMakeLists
# Logging system

set(LOGGER_SOURCES
    FRLogger.cpp
    LogEntries.cpp
    LogEntry.cpp
    LogEntryType.cpp
)

add_library(freerouting_logger STATIC ${LOGGER_SOURCES})
target_link_libraries(freerouting_logger PUBLIC freerouting_core)