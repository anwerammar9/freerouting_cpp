# File Index: Java and C++ Source Files

## Java Files (Source of Truth) — 289 files

### autoroute/ (35 files)
AutorouteAttemptResult, AutorouteAttemptState, AutorouteControl, AutorouteEngine,
BatchAutorouter, BatchAutorouterThread, BatchAutorouterV19, BatchOptimizer,
BatchOptimizerMultiThreaded, BoardHistory, BoardHistoryEntry, BoardUpdateStrategy,
CompleteExpansionRoom, CompleteFreeSpaceExpansionRoom, Connection, DestinationDistance,
DrillPage, DrillPageArray, ExpandableObject, ExpansionDoor, ExpansionDrill,
ExpansionRoom, FreeSpaceExpansionRoom, IncompleteFreeSpaceExpansionRoom,
InsertFoundConnectionAlgo, ItemAutorouteInfo, ItemRouteResult, ItemSelectionStrategy,
LocateFoundConnectionAlgo, LocateFoundConnectionAlgo45Degree,
LocateFoundConnectionAlgoAnyAngle, MazeListElement, MazeSearchAlgo, MazeSearchElement,
MazeShoveTraceAlgo, NamedAlgorithm, NamedAlgorithmType, ObstacleExpansionRoom,
OptimizeRouteTask, PerformanceProfiler, RoutingFailureLog,
Sorted45DegreeRoomNeighbours, SortedOrthogonalRoomNeighbours, SortedRoomNeighbours,
TargetItemExpansionDoor, TaskState
+ 6 event files in autoroute/events/

### board/ (51 files)
AngleRestriction, BasicBoard, BoardLoader, BoardObserverAdaptor, BoardObservers,
BoardOutline, CalcFromSide, CalcShapeAndFromSide, ChangedArea, Communication,
Component, ComponentObstacleArea, ComponentOutline, Components, ConductionArea,
Connectable, CoordinateTransform, DrillItem, FixedState, ForcedPadAlgo, ForcedViaAlgo,
Item, ItemIdentificationNumberGenerator, ItemSearchTreesInfo, ItemSelectionFilter,
Layer, LayerStructure, MoveComponent, MoveDrillItemAlgo, ObjectInfoPanel, ObstacleArea,
OptViaAlgo, Pin, PolylineTrace, PrintableShape, PullTightAlgo, PullTightAlgo45,
PullTightAlgo90, PullTightAlgoAnyAngle, RoutingBoard, SearchTreeManager,
SearchTreeObject, ShapeSearchTree, ShapeSearchTree45Degree, ShapeSearchTree90Degree,
ShapeTraceEntries, ShoveTraceAlgo, Trace, Unit, Via, ViaObstacleArea

### core/ (36 files)
BoardFileDetails, BoardLibrary, FileFormat, LogicalPart, LogicalParts, Package,
Packages, Padstack, Padstacks, RouterCounters, RouterJobResourceUsage,
RoutingJob, RoutingJobPriority, RoutingStage, Session, StopRequestState,
StoppableThread
+ 6 event files, 12 scoring files

### designforms/specctra/ (41 files)
AutorouteSettings, Circuit, Component, ComponentPlacement, CoordinateTransform,
DsnFile, Keyword, Layer, LayerStructure, Library, Net, NetClass, Network,
PartLibrary, Placement, PlaceControl, Plane, PolygonPath, PolylinePath, Parser,
ReadScopeParameter, Rectangle, Resolution, Rule, RulesFile, Scanner, ScopeKeyword,
SesFileReader, SessionToEagle, SessionWriter, Shape, SpecctraDsnStreamReader,
SpecctraSesFileWriter, Structure, Unit, Wiring, WriteScopeParameter

### geometry/planar/ (34 files)
All fully migrated — Point, Vector, Line, Shape, Area, TileShape, Simplex, etc.

### rules/ (10 files)
All fully migrated — BoardRules, ClearanceMatrix, Net, NetClass, etc.

### settings/ (21 files)
GlobalSettings, RouterSettings, RuntimeEnvironment, ProfileSettings, OptimizerSettings,
RouterOptimizerSettings, RouterScoringSettings, SettingsMerger, SettingsSource,
DesignRulesCheckerSettings, FeatureFlagsSettings, StatisticsSettings,
UsageAndDiagnosticDataSettings, UserProfileSettings
+ 9 settings source files

### Other modules
- logger/ (4 files) — done
- constants/ (1 file) — done
- datastructures/ (14 files) — 6+2 done
- interactive/ (9 files) — 3 done
- drc/ (7 files) — 1 done
- management/ (18 files) — 2 done
- boardgraphics/ (8 files) — 0 done (GUI only)

## C++ Files — ~385 files

See `docs/migration/INDEX.md` and per-module `MIGRATION_*.md` for file-level status.

Key directories with C++ implementations:
- src/autoroute/ — 23 classes compiled
- src/board/ — 41 classes compiled
- src/geometry/planar/ — 34 classes (complete)
- src/rules/ — 10 classes (complete)
- src/designforms/specctra/ — 9 files compiled
- src/core/ — 6 classes compiled
- src/settings/ — 2 classes compiled
- src/interactive/ — 3 classes compiled
- src/logger/ — 4 classes (complete)
- src/drc/ — 1 class compiled
- src/management/ — 2 classes compiled
