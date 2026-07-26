# TASK-217 handoff — Phase-0 Recast ini coarsening for ~9.8x area (gameplay-programmer, 2026-07-18)

## What was done
Single-file, minimal-diff edit to `Config/DefaultEngine.ini`, existing `[/Script/NavigationSystem.RecastNavMesh]`
section ONLY (+23 lines, 0 deletions; TASK-027 comment + `RuntimeGeneration=Dynamic` untouched; no other section
touched; `Source/` untouched — TASK-216 owns it). Per TASK-214 lane strategy: edited in the shared tree, NO
checkout, NO commit — TASK-218 checks out `m7.6-arena10x` and commits via explicit pathspecs.

## Verified NavMeshResolutionParams syntax (UE 5.8 — checked against installed engine source, not docs-from-memory)
- `ARecastNavMesh.NavMeshResolutionParams` is a **fixed C-array of 3** `FNavMeshResolutionParam`
  (`RecastNavMesh.h:726`), indexed by `ENavigationDataResolution` (`NavigationDataResolution.h`):
  **Low = 0, Default = 1, High = 2**. The Default-resolution entry is therefore **index [1]** — NOT [0]
  (the dispatch example's `[0]` would have coarsened the *Low* resolution and left Default at 19 uu).
- Struct fields: `CellSize`, `CellHeight`, `AgentMaxStepHeight` (`RecastNavMesh.h:551-568`). The loose
  top-level `CellSize`/`CellHeight`/`AgentMaxStepHeight` keys are `UE_DEPRECATED(all,...)` shims.
- Exact ini form confirmed against the engine's own `BaseEngine.ini:3055-3057`, e.g. baseline
  `NavMeshResolutionParams[1]=(CellSize=19.000000,CellHeight=10.f,AgentMaxStepHeight=35.f)`.
- Static-array config semantics: writing only `[1]` overrides only Default; Low stays (38, 10, 35) and
  High stays (19, 10, 35) from BaseEngine.ini — intended (nothing in the project requests Low/High tiles).
- Step height preserved: the project never overrode `AgentMaxStepHeight` anywhere, so current value =
  engine default **35.0** — kept exactly (`AgentMaxStepHeight=35.0` in the new entry).

## Param table for TASK-218 to MIRROR onto the L_Arena RecastNavMesh actor (decision 7: ini and actor MUST match)
| Param (actor property) | Value | Was (engine baseline) |
|---|---|---|
| RuntimeGeneration | Dynamic | Dynamic (project, TASK-027) — UNCHANGED |
| TileSizeUU | **2000.0** | 1000 |
| NavMeshResolutionParams[1].CellSize (Default resolution) | **32.0** | 19 |
| NavMeshResolutionParams[1].CellHeight | **20.0** | 10 |
| NavMeshResolutionParams[1].AgentMaxStepHeight | **35.0** (preserve) | 35 |
| bFixedTilePoolSize | **True** | False (C++ ctor) |
| TilePoolSize | **1024** | 1024 (C++ ctor `RecastNavMesh.cpp:513`; now pinned + active via bFixedTilePoolSize) |
| bDoFullyAsyncNavDataGathering | **True** | False (`RecastNavMesh.cpp:550`) |
| AgentRadius / AgentHeight / AgentMaxSlope | untouched | 34 / 144 / 44 (BaseEngine) — do NOT edit on the actor either |
| NavMeshResolutionParams[0] (Low) / [2] (High) | untouched | (38,10,35) / (19,10,35) |

## CRITICAL caveat for TASK-218 (recorded per spec)
The serialized RecastNavMesh actor in `L_Arena` carries its **own saved copies** of every param above.
Project ini applies to freshly spawned nav data only — if TASK-218 does not mirror the table onto the actor
AND run **Build > Navigation** + resave, the editor **silently keeps the old baked values** and the ini is a
no-op for L_Arena. Editor-python route first; else flag Jonathan's one click in 🚨 (manager decision 7).

## Fallback ladder (plan §1, recorded in the ini comment for future tuning — listed, not planned)
1. CellSize 32 -> 40, TileSizeUU 2000 -> 2500
2. Shrink nav-bounds X to +/-26,500 (gameplay corridor only)
3. Nuclear: `RuntimeGeneration=DynamicModifiersOnly` + obstacle-mark every carver (changes carve semantics).

## QA should scrutinize
- Index choice `[1]` vs the dispatch's `[0]` example — justified above from `NavigationDataResolution.h` (Default=1).
- Encoding: file re-verified post-edit — no BOM, plain single-byte start (`[/Sc...`), ASCII-only additions;
  `git diff` = +23/-0 confined to the one section; section header count in file = 1 (no duplication).
- FYI, not a defect: effective tile size quantizes to whole cells — `GetTileSizeUU()` truncates
  2000/32 = 62.5 -> 62 cells = **1984 uu effective tile** (`RecastNavMesh.cpp:1894-1897`). Plan's 2000 kept
  as written (plan = numeric truth); ~350-tile estimate unaffected.
- Engine sanity rule honored: TileSizeUU (2000) >> CellSize (32) (engine errors if TileSizeUU < CellSize).

## Files touched
- `Config/DefaultEngine.ini` (only file)
- `.claude/pipeline/TASKBOARD.md` (TASK-217 status line only)
- this handoff
