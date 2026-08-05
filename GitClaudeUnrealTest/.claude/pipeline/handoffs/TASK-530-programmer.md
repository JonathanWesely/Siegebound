# TASK-530 — the nav-rebuild concurrency ini flip — gameplay-programmer handoff

**Status:** `ready-for-qa` · **QA gate: TASK-537** · **Scope: `Config/DefaultEngine.ini` ONLY**
**Date:** 2026-08-04

> ### ⛔ **THIS CHANGE IS UNVERIFIED UNTIL A LOG LINE SAYS OTHERWISE, AND ITS REVERSAL IS A PLANNED BRANCH — NOT A FAILURE.**
> If the PIE log after this lands still reads `gatherOnGameThread=false`, the ini did **not** reach the serialized
> `L_Arena` `ARecastNavMesh` ⇒ **TASK-540 branch A reverts BOTH keys** and the stuck-detection ladder
> (TASK-531/532/533/534) carries the fix alone. That is **one of two expected outcomes** (`NAV-§9` clause 3).
> ⛔ I observed **no effect**. I ran no PIE, no compile, no editor. I claim nothing about behaviour.

**M8 DECLARATION (verbatim, as required):** *adds no replicated property, no new replicated class, no new relevancy tier.*
(Structurally true by inspection: this task adds **zero symbols** — it is a config-file edit with no C++ of any kind.)

---

## 1. Files touched — exactly one

| File | Change |
|---|---|
| `Config/DefaultEngine.ini` | 1 key edited, 1 key added, 39 comment lines added. **Nothing else.** |

⛔ **No `.h`, no `.cpp`, no `Build.cs`, no `.umap`, no other config file.** `git diff --stat` = `1 file changed, 41 insertions(+), 1 deletion(-)`.

## 2. The exact diff

**Only two non-comment lines in the whole diff** (verified by filtering the diff for non-`;` payload lines):

```diff
-bDoFullyAsyncNavDataGathering=True
+bDoFullyAsyncNavDataGathering=False
+MaxSimultaneousTileGenerationJobsCount=8
```

### The comment block, added verbatim at `:290-328` (between the end of the TASK-217 block at `:289` and `TileSizeUU` at `:329`)

```ini
; TASK-530 (UNIT-PATHING, 2026-08-04) - CONDITIONAL; revert clause below.
; This DELIBERATELY REVERSES part of the TASK-217 rationale above: "fully-async
; gathering keeps carve/heal rebuilds off the game thread" is true, but the
; MEASURED price of it is too high to keep.
; CAUSE: with bDoFullyAsyncNavDataGathering=True the engine submits EXACTLY ONE
; tile task at a time. RecastNavMeshGenerator.cpp:5892-5896, the engine's own
; comment verbatim: "this is a temp solution to enforce only one worker thread
; if GatherGeometryOnGameThread == false due to missing safety features" -
;   const int32 NumTasksToSubmit =
;       (bDoAsyncDataGathering ? 1 : MaxTileGeneratorTasks) - NumRunningTasks;
; MEASURED COST: the runtime scatter dirties 326 tiles and the queue takes
; ~216 s to drain at ~1.5 tiles/s (handoffs/TASK-366-buildmaster.md:143-152;
; corroborated by the independent ~178 s far-castle nav-mark figure at
; handoffs/TASK-350-buildmaster.md:19). Tiles sort by PLAYER-PAWN seed
; locations, so the far field settles LAST: a unit ordered across the map
; paths over the pre-scatter bake and walks into solid rock. That is the
; reported "unit gets stuck behind a rock" symptom, exactly.
; WHY AN EXPLICIT 8 AND NOT THE DEFAULT: with the flag False,
; MaxTileGeneratorTasks = min(max(NumWorkerThreads*2, 1),
; MaxSimultaneousTileGenerationJobsCount) (RecastNavMeshGenerator.cpp:5465) -
; uncapped that is ~32 on this machine. 8 bounds the game-thread gather burst
; at a nameable number. Jonathan was offered 4 / 8 / leave-alone with the
; game-thread cost stated and ruled 8 (CONVENTIONS NAV-§0 ruling 3) - it is
; his lever at the playtest, not an agent's default.
; !! THE RISK THIS CHANGE RUNS IS THE NOTE AT :281-284 ABOVE: the serialized
; RecastNavMesh actor in L_Arena carries its OWN copies of these params, so
; this ini may never reach it. That note predicted exactly this trap, which is
; why it stands unchanged. TASK-529's telemetry is what settles it: it prints
; gatherOnGameThread= on the first PIE run of the session.
; REVERT CLAUSE (TASK-540 branch A): if the PIE log's gatherOnGameThread= still
; reads false after this change, this ini did NOT reach the serialized L_Arena
; nav actor - REVERT both keys. Do NOT escalate to an L_Arena save.
; Fallback ladder for this change (in order):
;   1) MaxSimultaneousTileGenerationJobsCount 8 -> 4 (if the game-thread
;      gather burst hitches visibly at the scatter)
;   2) revert BOTH keys (bDoFullyAsyncNavDataGathering back to True, delete
;      the jobs-count key) and let the stuck-detection ladder (TASK-531/532)
;      carry the fix alone - it is independent and works either way.
; Like every key in this section, this takes effect at the NEXT editor boot.
```

## 3. ⚠️ PLACEMENT — a decision I made, stated rather than buried

Spec (3) said **"EXTEND THE EXISTING TASK-217 COMMENT BLOCK (`:272-289`)."** I placed the new block
**immediately after `:289`, contiguous with the TASK-217 block and above the key list** — not down beside
`bDoFullyAsyncNavDataGathering` at the bottom. **Reason: that IS this section's house style.** The TASK-217
block already documents `bDoFullyAsyncNavDataGathering` from 8 lines away ("fully-async gathering keeps
carve/heal rebuilds off the game thread"), and TASK-027's comment likewise sits directly above the key it
governs. A comment-block-then-key-list layout is what the section already is; interleaving would have been
the departure. ✅ **Side benefit, and it is not an accident: because the insertion is BELOW `:289`, the
`:281-284` note KEPT ITS EXACT LINE NUMBERS** — every existing citation to `DefaultEngine.ini:281-284`
(CONVENTIONS `NAV-§9`, TASKBOARD `:6994`, the TASK-529 spec) still resolves correctly after this edit.

⛔ **The `:281-284` note is UNCHANGED, byte-for-byte — not deleted, not softened, not reworded.** My block
points AT it and says it predicted this trap.

## 4. GIT HAZARD LAW (a) — byte-level proof, not `git diff`

Targeted string edits only. ⛔ **No parse/re-serialise round-trip.**

| | before | after |
|---|---|---|
| **byte size** | **12,855** | **15,685** (+2,830) |
| **sha256** | `281a15b044e836cc4ab253ec354272b797b45bb59c6573c7a693d3b7abb1a808` | `80a87e6b4826377ff680e1d580d31142acb8a42b019814e2004a29ac120ffb3d` |
| **CR / LF** | 308 / 308 | 348 / 348 |
| **lone LF** | 0 | **0** |
| **lone CR** | 0 | **0** |
| **BOM** | none | **none** |
| **non-ASCII bytes** | 3 (one U+2014 em-dash at `:252`) | 5 (that em-dash + one U+00A7 `§` in `NAV-§0`) |

✅ **Pure CRLF preserved (348 CR = 348 LF, zero lone terminators of either kind)** — the `autocrlf` blindness
GIT HAZARD LAW (a) warns about cannot hide here because I checked the bytes, not the diff.
✅ **+40 lines = 39 comment lines + 1 new key line.** The arithmetic closes.
✅ **File still decodes as valid UTF-8.** The `§` is precedented — this file already carried a multi-byte
char inside a comment at `:252` before I touched it, and UE skips comment lines wholesale during
`FConfigFile` parsing.

## 5. ⛔ The prohibitions — discharged, each one greppable

| ⛔ | Status |
|---|---|
| `AgentRadius` / `CellSize` / `TileSizeUU` / `TilePoolSize` / any other key (`NAV-§2a`, `NAV-§6`) | **UNTOUCHED.** `RuntimeGeneration=Dynamic` `:271`, `TileSizeUU=2000.0` `:329`, `NavMeshResolutionParams[1]` `:330`, `bFixedTilePoolSize=True` `:331`, `TilePoolSize=1024` `:332` — all byte-identical. The filtered diff proves it: **the only non-comment lines in the entire diff are the two above.** |
| Open / edit / save `L_Arena` (`NAV-§5`) | **NOT DONE.** No `.umap` was opened or written; no editor, no MCP, no PIE. The spent exception (`TASKBOARD.md:9485-9489`) is **not cited** by me and stays spent. |
| Runtime mutation of the live `ARecastNavMesh` at `BeginPlay` (`NAV-§5`) | **NOT DONE, and NOT PROPOSED.** Whether the generator re-reads `GatherGeometryOnGameThread()` after construction is unestablished; it stays a future verification-first task with Jonathan's word. |
| Any C++ change | **NONE.** Zero source files touched. |
| Compile / PIE / Git | **NONE.** TASK-538 owns the compile. |

## 6. Engine claims — I re-verified all four first-hand before writing them into a permanent comment

Read directly from `C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\NavigationSystem\`:

1. ✅ `Private/NavMesh/RecastNavMeshGenerator.cpp:5892` — the *"temp solution to enforce only one worker
   thread…"* comment is **exactly** as cited, and `:5896` is the `(bDoAsyncDataGathering ? 1 :
   MaxTileGeneratorTasks)` ternary. **Cause 1 confirmed at the source.**
2. ✅ `:5465` — `MaxTileGeneratorTasks = FMath::Min(FMath::Max(NumberOfWorkerThreads * 2, 1), GetOwner() ?
   GetOwner()->GetMaxSimultaneousTileGenerationJobsCount() : INT_MAX);` — confirmed, and note the **`INT_MAX`
   fallback**: without an explicit key the cap is worker-threads-driven, which is exactly why an explicit
   number is the deliverable and not a nicety.
3. ✅ **Both keys are genuinely `config` `UPROPERTY`s on `ARecastNavMesh`** (`Public/NavMesh/RecastNavMesh.h:765-766`
   and `:865-866`) — so the ini key **names are correct and will bind**. This is the half of the mechanism
   that was worth checking: the names are right; whether they reach the *serialized instance* is §7.
4. ✅ The measured baseline is real and quoted correctly — `handoffs/TASK-366-buildmaster.md:143-152` gives
   **run 2, verbose from t=0: 216.1 s, 326 tiles**, and `handoffs/TASK-350-buildmaster.md:19` gives the
   independent **T=178.07** far-castle re-mark. ⚠️ **TASK-366 flags its own honest caveat** that 178 s was a
   *different probe definition* (nav-probe convergence, not queue drain) — I wrote "corroborated by the
   independent … figure" rather than treating them as the same measurement, deliberately.

## 7. ⚠️ WHAT QA SHOULD SCRUTINISE — the things I flagged rather than decided

1. ⭐ **THE UNPROVABLE PREMISE, NAMED: I cannot show this ini reaches the serialized `L_Arena` nav actor.**
   `.umap` is binary; a config `UPROPERTY` on a level actor takes the CDO value **unless a differing value was
   serialized**, and I have no static way to see which. **This is the whole conditional.** ⛔ **Do not let a
   PASS on this task be read as "the concurrency fix works" — a PASS here means the two keys and the comment
   are correct, nothing more.**
2. ⚠️ **AN INI CHANGE NEEDS AN EDITOR RESTART TO BE PICKED UP AT ALL.** If TASK-538 or anyone else evaluates
   this in an already-running editor, the result is meaningless and must not be recorded as evidence either way.
3. ⚠️ **THE GAME-THREAD COST IS REAL AND UNMEASURED.** `False` moves geometry gathering **onto the game
   thread**, and 8 tiles may now gather in one burst during the scatter. **No FPS baseline exists** (M7's
   TASK-183 never ran), so I cannot bound the hitch — that is what ladder rung 1 (`8 -> 4`) is for, and it is
   Jonathan's lever at the playtest. ⛔ I did not pre-emptively pick 4 to be safe; **8 is his ruling.**
4. ⚠️ **`NAV-§6`'s tile-pool hazard interacts with this change and I did NOT touch it.** More tiles in flight
   is not more tiles resident, so I do not believe concurrency moves the `TilePoolSize=1024` risk — **but I
   did not verify that**, and I was forbidden from changing it regardless. TASK-529's `activeTiles=` /
   `poolCap=` telemetry is the instrument; the gate greps `tile limit reached!`.
5. **Placement of the comment block (§3 above) is a judgement call I made** — if QA reads spec (3) as
   requiring the text be interleaved with the keys instead, that is a finding I will take, and it is a
   one-move fix.

## 8. Nothing in the spec was wrong — one thing was imprecise

✅ The spec's engine quote and both line citations are accurate. **One cosmetic note:** the spec renders
`:5892-5896` as four contiguous lines, but the real source has a **blank line at `:5895`** between the
`bDoAsyncDataGathering` declaration and the `NumTasksToSubmit` line. The range `5892-5896` is still correct;
the quote just isn't contiguous. I quoted the engine's comment verbatim and the ternary verbatim without
implying they are adjacent.

## 9. Downstream

- **QA gate: TASK-537** (`qa/TASK-537.md`), criteria 6 (`no TilePoolSize / AgentRadius / .umap change`) and
  `NAV-§10`.
- **TASK-538** owns the compile. ⛔ I did not compile. *(An ini change compiles nothing, but the editor must
  still be restarted for it to load.)*
- **TASK-529** is the acceptance instrument. **TASK-540 branch A** is this task's boarded rollback.
