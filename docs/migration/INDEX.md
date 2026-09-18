# Migration Status Index

**Project**: Freerouting Java -> C++ Migration
**Last Updated**: 2026-09-18

---

## Module Status Overview

| Module | Java Files | Done | Partial | Remaining | Status | Report |
|--------|-----------|------|---------|-----------|--------|--------|
| Logger | 4 | 4 | 0 | 0 | 100% | [MIGRATION_logger.md](MIGRATION_logger.md) |
| Rules | 10 | 10 | 0 | 0 | 100% | [MIGRATION_rules.md](MIGRATION_rules.md) |
| Constants | 1 | 1 | 0 | 0 | 100% | [MIGRATION_constants.md](MIGRATION_constants.md) |
| Geometry | 34 | 34 | 0 | 0 | 100% | [MIGRATION_geometry.md](MIGRATION_geometry.md) |
| Interactive | 9 | 5 | 1 | 3 | 67% | [MIGRATION_interactive.md](MIGRATION_interactive.md) |
| DataStructures | 14 | 6 | 2 | 6 | 57% | [MIGRATION_datastructures.md](MIGRATION_datastructures.md) |
| Board | 51 | 50 | 0 | 1 | 98% headless* | [MIGRATION_board.md](MIGRATION_board.md) |
| Autoroute | 35 | 21 | 0 | 14 | 60% | [MIGRATION_autoroute.md](MIGRATION_autoroute.md) |
| DRC | 7 | 1 | 1 | 5 | 29% | [MIGRATION_drc.md](MIGRATION_drc.md) |
| DesignForms | 41 | 1 | 12 | 28 | 32% | [MIGRATION_designforms.md](MIGRATION_designforms.md) |
| Settings | 21 | 2 | 4 | 15 | 29% | [MIGRATION_settings.md](MIGRATION_settings.md) |
| Core | 36 | 6 | 1 | 29 | 19% | [MIGRATION_core.md](MIGRATION_core.md) |
| Management | 18 | 2 | 0 | 16 | 11% | [MIGRATION_management.md](MIGRATION_management.md) |
| BoardGraphics | 8 | 0 | 0 | 8 | 0% | [MIGRATION_boardgraphics.md](MIGRATION_boardgraphics.md) |

## Totals

- **Total Java Files**: 289
- **Fully Migrated**: 144 (50%)
- **Partially Migrated**: 21 (7%)
- **Not Migrated**: 124 (43%)

## Critical Path (DSN -> Route -> SES)

1. `geometry/planar` - Foundation primitives (100% by selection: 34/34, incl. rational `IntPoint`-substitutes via `cpp_int`, full `Shape` interface surface, `Area` interface + `PolygonArea`/`PolylineArea`/`TileShape`)
2. `datastructures` - Supporting data structures (ShapeTree/MinAreaTree complete)
3. `rules` - Design rules (100% done)
4. `board` - Board representation (98% headless; 50/51; PullTight/ForcedPad/ForcedVia/MoveDrillItemAlgo/MoveComponent/OptVia compiled (WIP), RoutingBoard facade stubs `combine_traces`/`get_outline`/`contains_trace_tails`/`get_trace_tail` ported, `BoardOutline` restored to build; **WP-1 contact graph done** (2026-09-18): `first_common_layer`, `get_connection_items`, `is_fanout_via`, `DrillItem` padstack-derived layer range, `normal_contact_point` for Trace/DrillItem; **WP-2 NetIncompletes done** (2026-09-18): `connected_group_count`, `join_connected_sets` aliasing fix; **WP-3 RatsNest done**; **WP-4 BoardStatistics+Scoring done** (2026-09-18): `init_from_board` (host/units/areaCm2/difficulty/fanout/totalLengthMm), Java-exact scoring, DSN/SES file ctor, 11 BoardStatistics gtests; **422/422 ctest**, headless `DSN→SES` verified on hw48na at 19%; headless blockers `ShoveTraceAlgo::spring_over`, `PullTight*` quality paths)
5. `autoroute` - Routing engine (21/35 compiled; 413/413 ctest tests pass)
6. `designforms/specctra` - DSN/SES I/O (32% done) — the missing link for real DSN->route->SES runs

## Top Blockers

| Blocker | Module | Impact |
|---------|--------|--------|
| SpecctraDsnStreamReader | DesignForms | Cannot read DSN files (note: Scanner.cpp read_dsn works; reader class parity partial) |
| SpecctraSesFileWriter | DesignForms | Cannot write SES output |
| ShapeTraceEntries / ShoveTraceAlgo | Board | Shove core (blocks ForcedPadAlgo/MoveDrillItemAlgo) |
| PullTightAlgo* | Board | Trace routing quality (blocks Trace.combine/split/pull_tight) |
| BatchOptimizer / DrillPage | Autoroute | Optimization pass / drill pages (Java only) |
| DesignRulesChecker | DRC | Design rule validation |
| RouterOptimizerSettings | Settings | Optimizer configuration |
