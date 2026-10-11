if(BUILD_TESTS)
    add_subdirectory(data)

    if(EXISTS data/geometry_tile_test.cpp)
        add_executable(geometry_tile_test data/geometry_tile_test.cpp)
        target_include_directories(geometry_tile_test PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(geometry_tile_test PRIVATE
            freerouting_geometry
            freerouting_datastructures
            freerouting_logger
            freerouting_constants
        )
        add_test(NAME geometry_tile_test COMMAND geometry_tile_test)
    endif()

    if(EXISTS data/geometry_polyline_area_test.cpp)
        add_executable(geometry_polyline_area_test data/geometry_polyline_area_test.cpp)
        target_include_directories(geometry_polyline_area_test PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(geometry_polyline_area_test PRIVATE
            freerouting_geometry
            freerouting_datastructures
            freerouting_logger
            freerouting_constants
        )
        add_test(NAME geometry_polyline_area_test COMMAND geometry_polyline_area_test)
    endif()

    if(EXISTS data/geometry_regular_tile_shape_test.cpp)
        add_executable(geometry_regular_tile_shape_test data/geometry_regular_tile_shape_test.cpp)
        target_include_directories(geometry_regular_tile_shape_test PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(geometry_regular_tile_shape_test PRIVATE
            freerouting_geometry
            freerouting_datastructures
            freerouting_logger
            freerouting_constants
        )
        add_test(NAME geometry_regular_tile_shape_test COMMAND geometry_regular_tile_shape_test)
    endif()

    if(EXISTS data/datastructures_shape_tree_test.cpp)
        add_executable(datastructures_shape_tree_test data/datastructures_shape_tree_test.cpp)
        target_include_directories(datastructures_shape_tree_test PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(datastructures_shape_tree_test PRIVATE
            freerouting_datastructures
            freerouting_geometry
            freerouting_logger
            freerouting_constants
        )
        add_test(NAME datastructures_shape_tree_test COMMAND datastructures_shape_tree_test)
    endif()

    if(EXISTS data/board_shape_search_tree_test.cpp)
        add_executable(board_shape_search_tree_test data/board_shape_search_tree_test.cpp)
        target_include_directories(board_shape_search_tree_test PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(board_shape_search_tree_test PRIVATE
            $<LINK_GROUP:RESCAN,freerouting_board,freerouting_autoroute,freerouting_datastructures,freerouting_geometry,freerouting_rules,freerouting_logger,freerouting_constants>
        )
        add_test(NAME board_shape_search_tree_test COMMAND board_shape_search_tree_test)
    endif()

    if(EXISTS data/geometry_line_segment_test.cpp)
        add_executable(geometry_line_segment_test data/geometry_line_segment_test.cpp)
        target_include_directories(geometry_line_segment_test PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(geometry_line_segment_test PRIVATE
            freerouting_geometry
            freerouting_datastructures
            freerouting_logger
            freerouting_constants
        )
        add_test(NAME geometry_line_segment_test COMMAND geometry_line_segment_test)
    endif()

    if(EXISTS data/geometry_direction_test.cpp)
        add_executable(geometry_direction_test data/geometry_direction_test.cpp)
        target_include_directories(geometry_direction_test PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(geometry_direction_test PRIVATE
            freerouting_geometry
            freerouting_datastructures
            freerouting_logger
            freerouting_constants
        )
        add_test(NAME geometry_direction_test COMMAND geometry_direction_test)
    endif()

    # ---- gtest-based companion tests for compiled autoroute classes ----
    set(AUTOROUTE_TEST_SOURCES
        ${CMAKE_SOURCE_DIR}/src/autoroute/AutorouteAttemptState_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/AutorouteEngine_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/BoardUpdateStrategy_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/CompleteExpansionRoom_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/CompleteFreeSpaceExpansionRoom_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/DrillPage_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/DrillPageArray_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/ExpansionDrill_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/ExpandableObject_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/FoundConnectionInserter_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/FoundConnectionLocator_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/IncompleteFreeSpaceExpansionRoom_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/ItemAutorouteInfo_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/ItemRouteResult_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/ItemSelectionStrategy_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/MazeRipupResolver_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/MazeExpansionEngine_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/MazeSearchAlgo_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/MazeSearchElement_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/MazeTraceShover_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/NamedAlgorithm_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/NamedAlgorithmType_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/PerformanceProfiler_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/RoutingBoardAutoroute_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/RoutingBoardFanout_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/RoutingFailureLog_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/Sorted45DegreeRoomNeighbours_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/SortedOrthogonalRoomNeighbours_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/SortedRoomNeighbours_test.cpp
        ${CMAKE_SOURCE_DIR}/src/autoroute/TaskState_test.cpp
    )

    foreach(test_src IN LISTS AUTOROUTE_TEST_SOURCES)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(${test_name} PRIVATE
            $<LINK_GROUP:RESCAN,freerouting_autoroute,freerouting_board,freerouting_core,freerouting_datastructures,freerouting_geometry,freerouting_rules,freerouting_logger,freerouting_constants,freerouting_settings>
            GTest::gtest_main
        )
        gtest_discover_tests(${test_name})
    endforeach()

    # ---- gtest-based companion tests for the settings classes ----
    set(SETTINGS_TEST_SOURCES
        ${CMAKE_SOURCE_DIR}/src/settings/AutorouterSettings_test.cpp
        ${CMAKE_SOURCE_DIR}/src/settings/FanoutSettings_test.cpp
        ${CMAKE_SOURCE_DIR}/src/settings/LayerSettings_test.cpp
        ${CMAKE_SOURCE_DIR}/src/settings/OptimizerScoreSettings_test.cpp
        ${CMAKE_SOURCE_DIR}/src/settings/OptimizerSettings_test.cpp
        ${CMAKE_SOURCE_DIR}/src/settings/RouterScoreSettings_test.cpp
        ${CMAKE_SOURCE_DIR}/src/settings/RouterSettings_test.cpp
        ${CMAKE_SOURCE_DIR}/src/settings/RoutingCostSettings_test.cpp
        ${CMAKE_SOURCE_DIR}/src/settings/SettingsFieldTable_test.cpp
        ${CMAKE_SOURCE_DIR}/src/settings/sources/DefaultSettings_test.cpp
    )

    foreach(test_src IN LISTS SETTINGS_TEST_SOURCES)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(${test_name} PRIVATE
            freerouting_settings
            GTest::gtest_main
        )
        gtest_discover_tests(${test_name})
    endforeach()

    # ---- gtest-based companion tests for compiled board classes ----
    set(BOARD_TEST_SOURCES
        ${CMAKE_SOURCE_DIR}/src/board/ItemIdentificationNumberGenerator_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ItemSelectionFilter_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ObstacleArea_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ObstacleAreaSubclass_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ShapeSearchTree_complete_shape_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ShapeSearchTree_tie_pin_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/CalcFromSide_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/CalcShapeAndFromSide_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ComponentOutline_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/OptViaAlgo_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ForcedViaAlgo_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/Pin_center_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/Pin_shape_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/Package_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/Padstacks_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/BoardLibrary_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/LogicalPart_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/Component_test.cpp
        ${CMAKE_SOURCE_DIR}/src/rules/ClearanceMatrix_test.cpp
        ${CMAKE_SOURCE_DIR}/src/rules/BoardRules_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/PullTightAlgo_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/RoutingBoard_stubs_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/Trace_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ItemContacts_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ShoveTraceAlgo_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/RoutingBoardForcedTrace_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/Item_order_test.cpp
        ${CMAKE_SOURCE_DIR}/src/core/scoring/BoardStatistics_test.cpp
        ${CMAKE_SOURCE_DIR}/src/core/BoardFileDetails_test.cpp
    )

    foreach(test_src IN LISTS BOARD_TEST_SOURCES)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(${test_name} PRIVATE
            $<LINK_GROUP:RESCAN,freerouting_board,freerouting_autoroute,freerouting_core,freerouting_datastructures,freerouting_geometry,freerouting_rules,freerouting_logger,freerouting_constants,freerouting_scoring,freerouting_interactive,freerouting_settings>
            GTest::gtest_main
        )
        gtest_discover_tests(${test_name})
    endforeach()

    # ---- gtest-based companion tests for compiled interactive classes ----
    set(INTERACTIVE_TEST_SOURCES
        ${CMAKE_SOURCE_DIR}/src/interactive/NetIncompletes_test.cpp
        ${CMAKE_SOURCE_DIR}/src/interactive/RatsNest_test.cpp
    )

    foreach(test_src IN LISTS INTERACTIVE_TEST_SOURCES)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(${test_name} PRIVATE
            $<LINK_GROUP:RESCAN,freerouting_interactive,freerouting_board,freerouting_autoroute,freerouting_core,freerouting_datastructures,freerouting_geometry,freerouting_rules,freerouting_logger,freerouting_constants>
            GTest::gtest_main
        )
        gtest_discover_tests(${test_name})
    endforeach()

    # ---- gtest-based companion tests for compiled drc classes ----
    set(DRC_TEST_SOURCES
        ${CMAKE_SOURCE_DIR}/src/drc/DesignRulesChecker_test.cpp
    )

    foreach(test_src IN LISTS DRC_TEST_SOURCES)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(${test_name} PRIVATE
            $<LINK_GROUP:RESCAN,freerouting_drc,freerouting_interactive,freerouting_board,freerouting_autoroute,freerouting_core,freerouting_datastructures,freerouting_geometry,freerouting_rules,freerouting_logger,freerouting_constants,freerouting_settings>
            GTest::gtest_main
        )
        gtest_discover_tests(${test_name})
    endforeach()

    # ---- gtest-based companion tests for compiled designforms classes ----
    set(DESIGNFORMS_TEST_SOURCES
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/DsnReader_test.cpp
        ${CMAKE_SOURCE_DIR}/src/board/ShapeSearchTree45Degree_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/SessionWriter_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/SpecctraDsnStreamReader_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/ParserShape_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/ShapeWrite_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/WriteScopeParameter_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/CoordinateTransform_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/Parser_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/Circuit_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/Structure_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/AutorouteSettings_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/NetList_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/NetClass_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/Network_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/ParserWiring_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/Library_test.cpp
        ${CMAKE_SOURCE_DIR}/src/designforms/specctra/parser/ParserComponent_test.cpp
    )

    foreach(test_src IN LISTS DESIGNFORMS_TEST_SOURCES)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(${test_name} PRIVATE
            $<LINK_GROUP:RESCAN,freerouting_designforms,freerouting_settings,freerouting_board,freerouting_autoroute,freerouting_core,freerouting_datastructures,freerouting_geometry,freerouting_rules,freerouting_logger,freerouting_constants>
            GTest::gtest_main
        )
        target_compile_definitions(${test_name} PRIVATE FREEROUTING_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data")
        gtest_discover_tests(${test_name})
    endforeach()

    # ---- gtest-based companion tests for compiled geometry classes ----
    set(GEOMETRY_TEST_SOURCES
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/IntBox_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/IntOctagon_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/IntVector_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/Circle_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/Point_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/TileShape_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/ConvexTileShape_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/Polyline_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/LineSegment_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/PolygonShape_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/CircleArea_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/Area_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/Shape_test.cpp
        ${CMAKE_SOURCE_DIR}/src/geometry/planar/Rational_test.cpp
    )

    foreach(test_src IN LISTS GEOMETRY_TEST_SOURCES)
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_include_directories(${test_name} PRIVATE
            ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(${test_name} PRIVATE
            $<LINK_GROUP:RESCAN,freerouting_geometry,freerouting_datastructures,freerouting_logger,freerouting_constants>
            GTest::gtest_main
        )
        gtest_discover_tests(${test_name})
    endforeach()

    add_executable(ArrayStack_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/ArrayStack_test.cpp
    )
    target_include_directories(ArrayStack_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(ArrayStack_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(ArrayStack_test)

    add_executable(BigIntAux_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/BigIntAux_test.cpp
    )
    target_include_directories(BigIntAux_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(BigIntAux_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(BigIntAux_test)

    add_executable(IdentifierType_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/IdentifierType_test.cpp
    )
    target_include_directories(IdentifierType_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(IdentifierType_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(IdentifierType_test)

    add_executable(JavaHashMapOrder_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/JavaHashMapOrder_test.cpp
    )
    target_include_directories(JavaHashMapOrder_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(JavaHashMapOrder_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(JavaHashMapOrder_test)

    add_executable(JavaFormat_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/JavaFormat_test.cpp
    )
    target_include_directories(JavaFormat_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(JavaFormat_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(JavaFormat_test)

    add_executable(IndentFileWriter_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/IndentFileWriter_test.cpp
    )
    target_include_directories(IndentFileWriter_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(IndentFileWriter_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(IndentFileWriter_test)

    add_executable(FileFilter_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/FileFilter_test.cpp
    )
    target_include_directories(FileFilter_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(FileFilter_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(FileFilter_test)

    add_executable(Observers_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/Observers_test.cpp
    )
    target_include_directories(Observers_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(Observers_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(Observers_test)

    add_executable(JavaRandom_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/JavaRandom_test.cpp
    )
    target_include_directories(JavaRandom_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(JavaRandom_test PRIVATE
        GTest::gtest_main
    )
    gtest_discover_tests(JavaRandom_test)

    add_executable(PlanarDelaunayTriangulation_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/PlanarDelaunayTriangulation_test.cpp
    )
    target_include_directories(PlanarDelaunayTriangulation_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(PlanarDelaunayTriangulation_test PRIVATE
        freerouting_datastructures
        freerouting_geometry
        freerouting_logger
        GTest::gtest_main
    )
    gtest_discover_tests(PlanarDelaunayTriangulation_test)

    add_executable(IdentificationNumberGenerator_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/IdentificationNumberGenerator_test.cpp
    )
    target_include_directories(IdentificationNumberGenerator_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(IdentificationNumberGenerator_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(IdentificationNumberGenerator_test)

    add_executable(UndoableObjects_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/UndoableObjects_test.cpp
    )
    target_include_directories(UndoableObjects_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(UndoableObjects_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(UndoableObjects_test)

    add_executable(Stoppable_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/Stoppable_test.cpp
    )
    target_include_directories(Stoppable_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(Stoppable_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(Stoppable_test)

    add_executable(TimeLimit_test
        ${CMAKE_SOURCE_DIR}/src/datastructures/TimeLimit_test.cpp
    )
    target_include_directories(TimeLimit_test PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(TimeLimit_test PRIVATE
        freerouting_datastructures
        GTest::gtest_main
    )
    gtest_discover_tests(TimeLimit_test)

    # ---- management (HeadlessBoardManager) ----
    add_executable(HeadlessBoardManager_test ${CMAKE_SOURCE_DIR}/src/management/HeadlessBoardManager_test.cpp)
    target_include_directories(HeadlessBoardManager_test PRIVATE ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(HeadlessBoardManager_test PRIVATE
        freerouting_management
        $<LINK_GROUP:RESCAN,freerouting_designforms,freerouting_settings,freerouting_board,freerouting_autoroute,freerouting_core,freerouting_datastructures,freerouting_geometry,freerouting_rules,freerouting_logger,freerouting_constants>
        GTest::gtest_main
    )
    target_compile_definitions(HeadlessBoardManager_test PRIVATE FREEROUTING_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/tests/data")
    gtest_discover_tests(HeadlessBoardManager_test)

    # ---- DSN reader parity: every tests/data board against the Java oracle JSON (T117/T118) ----
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME DsnReaderJavaOracle
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/dsn_counts_compare.py
                    $<TARGET_FILE:freerouting> ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/dsn_counts_compare)
        set_tests_properties(DsnReaderJavaOracle PROPERTIES TIMEOUT 600)
    endif()
endif()
