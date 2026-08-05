# TASK-538 — build-master handoff (UNIT-PATHING batch gate)

**Commit `6ac058a`** — `TASK-538: UNIT-PATHING lands — the stuck ladder, the nav telemetry, and the concurrency flip (TASK-529..537)`
25 files changed, 7117 insertions(+), 14 deletions(-). ⛔ **NOTHING WAS PUSHED.**

**Verdict: compile `Result: Succeeded` · suite 73/73 green · committed.** Both gate conditions met.

⭐ **THE HEADLINE: `gatherOnGameThread=true`.** TASK-530's ini flip **DID** reach the serialized `L_Arena` nav actor. **TASK-540 branch A does NOT fire; the two ini keys stand.**

⚠️ **THE HONEST LIMIT, STATED FIRST.** The live capture was a **headless `-game` session**, not an editor PIE session. It loaded the level's **pre-baked** navmesh (425 active tiles already present at `pre-scatter`), so the ~326-tile rebuild that produced the **216 s** baseline **never occurred**. Consequently `running>1` was **NOT observed**, the settle number below is **NOT comparable to 216 s**, and **zero** `LogSiegeStuck` lines exist because no units were ever spawned or commanded. **TASK-539 still owes the rebuild-under-load numbers and the whole stuck-ladder runtime proof.** I did not substitute reasoning for a reading, and I did not escalate to an `L_Arena` save.

---

## 1. Git pre-flight — observed, not assumed

`git status --porcelain` → **25 entries, all of them this batch's own deliverables** (11 modified, 14 untracked). The board's "tree was clean" reading was a pre-batch snapshot; the delta is exactly TASK-529..536's work plus `qa/TASK-537.md`. **No foreign modification, no Jonathan self-commit since the board was written.**

`git rev-list --left-right --count origin/main...main` → **`0	4`** — main **4 ahead** of `origin/main`, **0 behind**. Matches the board exactly.
`git log --oneline` HEAD → **`75acf3d`** (TASK-526 handoff), prior **`5a07b96`** (TASK-526). ⇒ **Board state CONFIRMED; no STOP condition.** After this task main is **6 ahead**, still unpushed.

**Quiet module — cleared.** No `UnrealEditor*` process of any kind was running (only `blender-mcp` × 2). **TASK-481 is `in-progress` but is NOT mid-build** — its compile already passed on 2026-08-03 and it is parked on an unreachable MCP endpoint, not on a compiler. No second compile gate was in flight at any point.

⚠️ **One observation for the orchestrator, outside my lane:** TASK-481's board entry says its Stage-A source is **deliberately held, uncommitted**. That source is **not in the working tree today** — the tree contains only UNIT-PATHING files. Either it was committed inside a later commit or it is gone. **Worth a check before TASK-481 resumes; I did not touch it.**

---

## 2. Compile

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project="C:/.../GitClaudeUnrealTest.uproject" -waitmutex
```

| | |
|---|---|
| **Verdict line (parsed from the log, NOT `$LASTEXITCODE`)** | **`Result: Succeeded`** |
| **Errors** | **0** |
| **Warnings** | **0** |
| **Duration** | **16.67 s** total execution (13.66 s in UBA), vs the TASK-526 baseline 20.29 s |
| **Relink** | `UnrealEditor-GitClaudeUnrealTest.lib` + `UnrealEditor-GitClaudeUnrealTest.dll`. ⛔ **`Plugins/SiegeLlama/` was NOT rebuilt** — no source of its changed, so no foreign diagnostic was possible. |

**16 actions.** Adaptive non-unity correctly excluded all seven changed TUs: `BattlefieldScatter.cpp`, `MinerUnit.cpp`, `SiegeNavAreas.cpp`, `SiegeNavDiagnostics.cpp`, `SiegeStuckStatics.cpp`, `SummonedUnit.cpp`, `SiegeStuckStaticsTest.cpp` — i.e. **every new file compiled standalone**, which is the strictest include-hygiene check available.

⛔ **16.67 s is not the ~2 s Smart App Control death** (`0x800711C7` absent). No Live Coding mutex contention — no editor was running.

**The watch list, answered item by item:**

- ⭐ **`ASiegeUnitAIController::OnMoveCompleted` — NO C4996, NO C4263, NO C4264, no `-Woverloaded-virtual`, no deprecation warning of any kind.** Grepped the console log and UBT's `Log.txt` for `warning`/`error`/`C42`/`C49`/`deprecat`: **zero matches.** ⇒ The non-deprecated `AIController.h:230` overload compiles clean **without** `using Super::OnMoveCompleted;`, exactly as QA's NIT-2 predicted. **The documented one-line remedy was not needed and must not be applied speculatively.**
- ⭐ **`FSiegeStuckTuning`'s same-line `UPROPERTY(...) float X = 1.f;` idiom is UHT-legal — now proven, not merely reasoned.** `SiegeStuckStatics.h` is the batch's first new `USTRUCT` in a non-`UCLASS` header and it passed. 🔎 **And the proof is stronger than expected: UHT itself runs with `-WarningsAsErrors`** (`Running Internal UnrealHeaderTool … -WarningsAsErrors -installed`), so a UHT *warning* on that idiom would have **failed** this build. It did not. ⚠️ Note the asymmetry for the record: **UHT** treats its warnings as errors; the **C++ compiler** does not — which is fully consistent with QA's WARN-3 finding of no `bWarningsAsErrors` anywhere in `Source/`.
- ⛔ **No `Build.cs` change was needed or made.**

---

## 3. The suite

```
UnrealEditor-Cmd.exe <project> -ExecCmds="Automation RunTests Siegebound" -unattended -nopause -nullrhi
  -testexit="Automation Test Queue Empty" -abslog=<scratchpad>\suite-538.log
  "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry.Entry"
```
Run from **PowerShell**, not Git Bash. The `-ini:` override was mandatory and did its job — startup map `/Engine/Maps/Entry.Entry`, so `L_Arena` was never opened.

> `LogAutomationCommandLine: Display: ...Automation Test Queue Empty **73 tests performed.**`

### ⭐ **73 / 73 — `Success = 73`, non-Success = 0.** Engine init 8.72 s; suite wall-clock 18.48 s.

**Regression check on the pre-existing 53, reported separately as required:**

| group | count | result |
|---|---|---|
| `Siegebound.Assistant.*` | 39 | all Success |
| `Siegebound.Input.*` | 7 | all Success |
| `Siegebound.Settings.*` | 7 | all Success |
| **pre-existing subtotal** | **53** | ✅ **exactly the baseline, zero regressions, zero drops** |
| `Siegebound.Nav.Stuck.*` | **20** | all Success |
| **total** | **73** | ✅ **= the 73 floor** |

### The 20 new tests, individually, by name — all `Result={Success}`

`BlockedClockBumpIsDiscardedWhenNotAdvancing` · `EscalationCooldownGatesTheLadder` · `EscapingProgressRadiusResetsTheLadder` · `IdleByDesignUnitIsNeverRescued` · `ImmediateReEvaluationIsRefused` · `LadderClimbsToWidenThenAbandon` · `MonotonicLevelHoldsWithoutTheCooldown` · `MovingUnitReAnchors` · `ResetClearsEveryField` · `RungOrdinalsMatchTheLogContract` · `ShippedTuningDefaults` · `SidestepFiresExactlyOnceAtThreshold` · `SidestepGoalAlternatesOnAttemptParity` · `SidestepGoalDegenerateInputsAreSafe` · `SidestepGoalIsDeterministic` · `SidestepGoalIsPerpendicularAtDistance` · `SidestepSideAlternatesAcrossStallsForOneUnit` · `SlidingUnitNeverEscalates` · `SustainedStallRespectsTheRequestCeiling` · `ZeroDeltaNeverAdvancesTheLadder`

### ⭐ The named three, called out individually

1. **`Siegebound.Nav.Stuck.EscalationCooldownGatesTheLadder` — ✅ Success.**
2. **`Siegebound.Nav.Stuck.MonotonicLevelHoldsWithoutTheCooldown` — ✅ Success.**
   Together these are the **only** live guard on `CONVENTIONS.md:535` / the corrected WARN-1 claim. They deliberately retune away from shipped defaults so that **each brake is load-bearing alone** — brake 1 bounding the rate, brake 2 bounding the count *and* the ladder's termination (QA's NIT-6). Both green ⇒ **neither brake is removable without going red.**
3. ⭐⭐ **`Siegebound.Nav.Stuck.SidestepSideAlternatesAcrossStallsForOneUnit` — ✅ Success.** This is the loop-0 blocker's regression test.
   ⛔ **And the landing signal, checked the way QA demanded — in the RUNNER OUTPUT, not the source:** grep of `suite-538.log` for `SidestepSideIsConstantForOneUnit` returns **0 matches**. The old registered name is **absent from the suite entirely**. ⇒ **The fix landed.** (The two deliberate history comments in `SiegeStuckStaticsTest.cpp:50` and `:1521` were correctly not consulted.)

**One automation warning, and it is not this batch's:** a `Warning`-verbosity line from the pre-existing `Siegebound.Input` lane — `[SiegeInputLayout] ⛔ AUTOMATION OVERRIDE: the translation was set by SetTranslationMapForAutomationTests…`. It is an intentional self-announcement emitted by a **passing** test. No other warning or error in the whole run.

---

## 4. ⭐ Evidence capture — all seven items

**How it was obtained, plainly.** No editor was running; there is **no Unreal MCP tool in this session** and `127.0.0.1:8000/mcp` was not up, so I could not drive an editor PIE session, and ⛔ launching the GUI editor onto the never-save `L_Arena` to close it myself is exactly what `NAV-§5` and the editor-close discipline forbid. I ran instead a **live headless `-game` session on `L_Arena`**, which satisfies the restart requirement (a fresh process reads the ini fresh), exercises the **same serialized nav actor**, **cannot** raise a save modal, and writes to `-abslog=<scratchpad>` so the QA-cited `Saved/Logs/GitClaudeUnrealTest.log` was **not rotated away**.

```
UnrealEditor-Cmd.exe <project> /Game/Maps/L_Arena -game -nullrhi -unattended -nopause -abslog=<scratchpad>\arena-run1.log
```
The process was terminated by me after the definitive verdict landed. It was **my own** headless process — no editor of Jonathan's was touched, closed, or killed.

### The complete `LogSiegeNavDiag` output, verbatim

```
[2026.08.05-03.43.21:901][  0]LogSiegeNavDiag: nav-config: actor='RecastNavMesh-Default' gatherOnGameThread=true maxTileJobs=8 cellSize=32.00 tileSizeUU=2000.00 agentRadius=35.00 poolCap=1024 fixedPool=true runtimeGen=Dynamic
[2026.08.05-03.43.21:901][  0]LogSiegeNavDiag: nav-build [pre-scatter]: remaining=0 running=0 dirtyAreas=2 hasDirty=true activeTiles=425 poolCap=1024
[2026.08.05-03.43.22:231][  0]LogSiegeNavDiag: nav-build [post-scatter]: remaining=0 running=0 dirtyAreas=2 hasDirty=true activeTiles=425 poolCap=1024
[2026.08.05-03.43.22:925][139]LogSiegeNavDiag: nav-build [at-confirmation]: remaining=0 running=0 dirtyAreas=0 hasDirty=false activeTiles=425 poolCap=1024
[2026.08.05-03.43.23:043][451]LogSiegeNavDiag: nav-build [at-confirmation]: remaining=0 running=0 dirtyAreas=0 hasDirty=false activeTiles=425 poolCap=1024
```

### (1) ⭐ `gatherOnGameThread=` — **`true`. THE BATCH'S DECISION POINT, ANSWERED.**

> `nav-config: actor='RecastNavMesh-Default' gatherOnGameThread=true maxTileJobs=8 …`

⇒ **TASK-530's ini flip reached the serialized `L_Arena` nav actor.** `maxTileJobs=8` confirms the *second* key landed too, and `poolCap=1024 fixedPool=true runtimeGen=Dynamic` confirm the untouched keys are intact.
⇒ ⛔ **TASK-540 branch A does NOT fire. Do not revert the two keys.**
⭐ **The ini's own revert clause predicted this trap and is now discharged in the affirmative** — the `:281-284` note warned the serialized actor carries its own copies; it evidently did **not** carry an overriding value, so the config default won at load. **That question is closed, first-hand, off a live log line printed by `FSiegeNavDiagnostics::LogNavConfigOnce`.** No escalation attempted, nothing further tried.

### (2) `[pre-scatter]` / `[post-scatter]` — **`remaining=0 running=0` at BOTH.**

⛔ **`running>1` was NOT observed, so concurrency is NOT confirmed live.** The reason is structural and is not a defect: `pre-scatter` **already reads `activeTiles=425`**, i.e. the session loaded a fully baked navmesh from the package and there was essentially no tile work to do — `dirtyAreas` sat at 2 before *and* after a scatter that placed **14,978 instances** (30 boulders, 26 hill, 40 slabs, 340 trees, 300 rocks, 12,242 grass, 2,000 plants). A `-game` session simply does not reproduce the editor's from-scratch rebuild.
⇒ **The concurrency flip is PRESENT (proved by item 1) but its EFFECT is UNMEASURED. TASK-539's editor session owes `running=8`.**

### (3) Settle wall-clock — **+1.024 s. ⛔ RECORDED, NOT GATED — and NOT comparable to the 216 s baseline.**

`GenerateScatter` at `03.43.21:901` → definitive `CONFIRMED` at `03.43.22:925` = **1.024 s**; the settle-poll's own `CONFIRMED` at `03.43.23:043` = **1.142 s**.
⛔ **Do not read this as "216 s → 1 s".** Per item 2 there was no rebuild to wait for. **This number measures an already-baked navmesh, and the 216 s baseline stands unchallenged until an editor session says otherwise.**

### (4) `activeTiles=` vs `poolCap=` — **425 vs 1024 (41.5 %), and `tile limit reached!` is ABSENT (0 matches).**

⇒ **`NAV-§6`'s tile-pool hazard did NOT trigger**, with ~600 tiles of headroom, on its first-ever instrumented run. Neither value is `-1`, so the reading is real rather than unavailable. ⚠️ Bounded by the same caveat: 425 is the loaded bake.

### (5) `[Stuck] blocked:` / `escalate:` — **ZERO `LogSiegeStuck` lines of any kind.**

⛔ **NOT satisfied, and I will not dress it up.** No match ran and no unit was ever spawned or commanded in a headless unattended session, so the ladder had nothing to act on. **The entire runtime proof of the livelock mechanism — every `blocked:`, every `escalate: level= action= stalled=`, WARN-6's `blocked:`-with-no-`escalate:` signature, and the two-`Sidestep`-lines-per-unit signature that shows the loop-1 fix working — is owed by TASK-539.**

### (6) `CONFIRMED` vs `PROVISIONAL` — **two `CONFIRMED`, zero `PROVISIONAL`.**

```
[2026.08.05-03.43.22:924][137]LogSiegeTerrain: [BattlefieldScatter 'BP_BattlefieldScatter_C_0'] Nav generation FINISHED ('RecastNavMesh-Default') — running the DEFINITIVE post-settle traversability check (one deferred re-check; NAV-§4).
[2026.08.05-03.43.22:925][139]LogSiegeTerrain: [BattlefieldScatter 'BP_BattlefieldScatter_C_0'] Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending; Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s)) [definitive: OnNavigationGenerationFinished].
[2026.08.05-03.43.23:043][451]LogSiegeTerrain: [BattlefieldScatter 'BP_BattlefieldScatter_C_0'] Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending; Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s)).
```

⚠️ **This deviates from the expected "one `PROVISIONAL` at ~+5 s, then one `CONFIRMED`" pattern, and the deviation is explained rather than smoothed:** nav was already settled, so the settle poll never had to print `PROVISIONAL` at all. **Ordering is the clean one** — the engine's own `Nav generation FINISHED` delegate fired **first**, and the definitive check (carrying `[definitive: OnNavigationGenerationFinished]`) ran off it; the settle poll's plain `CONFIRMED` followed 0.12 s later. **0 culls** on both verdicts.
✅ **`NAV-§4` is NOT violated:** every `CONFIRMED` sits beside `remaining=0`. **There is no `CONFIRMED` with a non-zero `remaining=` anywhere in this log.**

### (7) ⭐ The `[at-confirmation]` snapshot — QA's SECOND job, with the absence stated explicitly

**The four-number tuple, at both occurrences, identical:**

> **`remaining=0` · `running=0` · `dirtyAreas=0` · `hasDirty=false`** (with `activeTiles=425 poolCap=1024`)

⛔ **THE CAP LINE IS ABSENT. STATED EXPLICITLY, AS REQUIRED: grep of the whole session log for `MaxNavSettleWait` and for `still building` returns a count of ZERO — not one match, case-insensitive.**

**Reading it against QA's decision table:** the tuple is all-zero beside a `CONFIRMED`, and no cap line fired — so this is **neither** the proven cap route **nor** a `NAV-§4` failure. It is also **not** the lull route: QA's lull signature is a `Nav generation FINISHED` arriving *after* a `CONFIRMED` (work still owed), and here `FINISHED` arrived **0.001 s before** it and *caused* it. ⇒ **This session is the honest fourth case: nav genuinely was settled, and the code said so correctly.**

⚠️ **What this does and does not settle for `NAV-§12`.** It is a clean positive control that the all-zero tuple *can* be truthful. It does **not** resolve the `+5.03 s` datum from `Saved/Logs/GitClaudeUnrealTest.log:1891`/`:1977`, because **this session never had a queue to lull** — there was no rebuild in flight at any point. **The lull-vs-genuine question for that log remains open and is owed by TASK-539's editor session, which will have a real 326-tile queue to observe.**

### 🔒 `L_Arena` — never opened, never saved

| | SHA256 |
|---|---|
| **before** | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| **after the suite** | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| **after the arena run** | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |

**Identical all three times**, 535,522 bytes, and it matches the historical value on record from TASK-481. No save modal was possible in either run. `git status` after the arena run was **byte-for-byte the same 25 entries** as the pre-flight — **the runs dirtied nothing.**

---

## 5. Commit

**`6ac058a`** · 25 files · 7117 insertions(+) / 14 deletions(-).

⛔ **Staged by explicit file path only — no `git add -A`, no `.`, no directory pathspec** (GIT HAZARD LAW (d)). The index was verified **empty** before staging and **no leftover** files after.

`Siegebound/SiegeNavDiagnostics.{h,cpp}` · `SiegeStuckStatics.{h,cpp}` · `SummonedUnit.{h,cpp}` · `MinerUnit.{h,cpp}` · `SiegeNavAreas.{h,cpp}` · `BattlefieldScatter.{h,cpp}` · `Tests/SiegeStuckStaticsTest.cpp` · `Config/DefaultEngine.ini` · `.claude/pipeline/TASKBOARD.md` · `CONVENTIONS.md` · `handoffs/TASK-529..536-programmer.md` (8) · `qa/TASK-537.md`.
**No `Docs/` file was touched by the manager in this batch**, so none was staged. **No `.umap`, no `Build.cs`, no `.uasset`.**

⛔ **NOTHING WAS PUSHED.** `main` is now **6 ahead** of `origin/main`. The push is Jonathan's, standing law.

⚖️ **M8 DECLARATION, VERBATIM: adds no replicated property, no new replicated class, no new relevancy tier.**

---

## 6. Follow-ups for the orchestrator — reported, not actioned

1. ⭐ **TASK-540 branch A is DEAD; the conditional resolved the other way.** `gatherOnGameThread=true`. ⚠️ **But branch B (8 → 4) is still live and still un-evidenced** — the game-thread gather burst was never exercised here, so only Jonathan's TASK-539 hitch report can retire it.
2. ⛔ **TASK-539 now owes MORE than the board says, and it is the only route to any of it:** `running=8`, the real settle wall-clock against 216 s, every `LogSiegeStuck` line, and the lull-vs-genuine question for the `+5.03 s` log. **A `-game` session structurally cannot produce them.**
3. ⚠️ **TASK-481's "held" Stage-A source is not in the working tree** (see §1). Needs a check before that task resumes.
4. 🔎 **New, small, and outside this batch: 45 `SymmetryAssert` warnings fired during the scatter** — e.g. `Layer 'Trees' … twin ground Z mismatch at P=(-19145,-3145) Z=388.3 vs P'=(19145,3145) Z=0.0`, and `zMismatch=35` on Grass, `6` on Plants, `1` on Trees. Several report one twin at **exactly `Z=0.0`**, which reads more like a failed ground trace than a genuine asymmetry. **Pre-existing, unrelated to UNIT-PATHING, and it did not affect any gate** — but the rot180 mirror is a stated design property and this is the manager's to triage.
5. 📌 **For the record, against QA's WARN-3:** UHT runs with `-WarningsAsErrors` even though the C++ compiler does not. Both halves of that asymmetry are now observed facts and the law text should say so.
