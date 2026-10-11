# Create test executable with simple assert-based tests
add_executable(run_tests all_tests.cpp)
target_include_directories(run_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(run_tests PRIVATE
    $<LINK_GROUP:RESCAN,freerouting_designforms,freerouting_settings,freerouting_board,freerouting_autoroute,freerouting_core,freerouting_datastructures,freerouting_geometry,freerouting_rules,freerouting_logger,freerouting_constants>
    freerouting_drc
    Threads::Threads
)

# Register tests
add_test(NAME AllTests COMMAND run_tests)
set_tests_properties(AllTests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})