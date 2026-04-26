# Create test executable with simple assert-based tests
add_executable(run_tests EXCLUDE_FROM_ALL all_tests.cpp)
target_include_directories(run_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(run_tests PRIVATE
    freerouting_core
    freerouting_geometry
    freerouting_rules
    freerouting_board
    freerouting_core_lib
    freerouting_designforms
    freerouting_drc
    Threads::Threads
)

# Register tests
add_test(NAME AllTests COMMAND run_tests)