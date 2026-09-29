# TASK-1596: 5a for TASK-1592 + TASK-1594 (build-master handoff)

**Editor for `TASK-1597`: PID 18832.** It is the GUI editor on the plain `.uproject` (no `-game`), and it owns MCP `:8000`. It has `UnrealEditor-GitClaudeUnrealTest.dll` sha256 `d7bbea6b03a7ea1d5ccca8489c17da2a53f5c89b78419a3eb26895a0055e9776` (10,172,928 B) loaded, which is the final restored build. PIE is off. It is PIE-clean per `VER-§12` cl. 7g: `BS_ERROR` 0 of 19 resident. It opened on `L_Arena`. PID 15044 is gone.

**No commit.** `TASK-1598` is the host. The index is empty, and no git write was made.

marker `TASK-1596-HELP-NUMBERS-2-5A` · law: `CLAUDE.md` rule 5a · `SC-§87` · `SC-§118` · `SHIP-§9` · `VER-§2` cl. 2 · `VER-§12` cl. 7g · source of truth `qa/TASK-1595.md` §1–§4, plus `qa/TASK-1593.md` W2 and §3

## 1. Gates read at my instant (2026-09-29, before the 13:33:56 PDT quit)

| gate | read |
|---|---|
| `qa/TASK-1593.md` / `qa/TASK-1595.md` | line 1 `PASS` / `PASS` |
| `TASK-1592` / `TASK-1594` | `qa-passed` / `qa-passed` |
| `TASK-1597` / `TASK-1598` | `backlog` / `backlog` (no verifier, no commit host live) |
| index | empty; HEAD `b9db99c` |

## 2. (1) Byte anchors: 10 of 10 equal to `qa/TASK-1595.md` §1

Measured before the build, after every arm's restore, and before the final build. No file has a BOM.

| file | sha256 | bytes · LF · CR |
|---|---|---|
| `SiegeControlsHelpWidget.cpp` | `66418029…fa0d` | 346171 · 5496 · 0 |
| `SiegeControlsHelpWidget.h` | `25e80996…924c` | 82041 · 1412 · 0 |
| `Tests/SiegeControlsHelpTest.cpp` | `ee68a048…f0e8` | 210420 · 3649 · 0 |
| `SiegePlayerController.h` | `c0c558f3…b2b7` | 211687 · 3626 · 3626 |
| `SiegePlayerController.cpp` | `32e582f0…a7d9` | 389435 · 7789 · 0 |
| `HeroCharacter.h` | `b4826384…d7ec` | 95800 · 1519 · 1519 |
| `SiegeGameMode.h` | `af7d9996…13c2` | 48345 · 810 · 810 |
| `WarMapWidget.h` | `86ca2acd…63d1` | 109055 · 1738 · 0 |
| `WarMapWidget.cpp` | `4abd324d…8206` | 146170 · 2964 · 0 |
| `Tests/SiegeHelpAccessorsTest.cpp` | `ddfe3335…48ad` | 19745 · 394 · 0 |

The full 64-hex values were compared by the tool (`scratchpad/t1596/gate.py`).

## 3. (2) Editor census and graceful quit (`SC-§118`)

- **Before:** exactly one `UnrealEditor.exe`, PID **15044**, created 2026-09-29 00:50:42. Its command line was `"…\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\…\GitClaudeUnrealTest.uproject"` (no `-game`), and it owned `:8000`. The companions were `CrashReportClientEditor` 26652 (`-MONITOR=15044`) and `UnrealTraceServer` 488.
- **Read over MCP in PID 15044 (read-only):** `is_pie_active` → `false`, `IN_PIE=False`, `DIRTY_CONTENT=[]`, `DIRTY_MAPS=[]`, level `L_Arena`. `BS_ERROR` 0 of 39 (`BS_UNKNOWN` 17 / `BS_UP_TO_DATE` 22).
- **Quit:** re-identified inside the same call by PID, name and exact command line. `CloseMainWindow()` returned `True` at 13:33:56, and the process had exited by 13:34:05. No force, and no save prompt.
- **Log tail:** `LogExit: Exiting.` · `Log file closed, 09/29/26 13:34:04`. The newest `Saved/Crashes` entry is still 2026-09-04. `Unreal*` process count after the quit: 0.

## 4. (2) Compile of the gated tree: `Result: Succeeded`

`Build.bat` per `CLAUDE.md`, editor closed, never Live Coding. Ran 13:34:24 → 13:34:55.

- 28 actions: 25 compiles, including `HeroCharacter.cpp`, `SiegeHelpAccessorsTest.cpp`, `SiegeControlsHelpTest.cpp`, `SiegeGameMode.cpp`, `WarMapWidget.cpp`, `SiegeControlsHelpWidget.cpp`, `SiegePlayerController.cpp` and unity blobs `Module.GitClaudeUnrealTest.1–18`; then the `.lib` link, the `.dll` link and `WriteMetadata`.
- **`Result: Succeeded`** · `Total execution time: 30.08 seconds`.
- The exit code (0) is recorded only. Diagnostics (`): warning|error <CODE>`): **0**.
- DLL `43fb2494…41c7` (superseded by the final build, §8).

## 5. (3) UHT shape: as `qa/TASK-1595.md` §2 and `qa/TASK-1593.md` §2 predicted

I copied the whole `UHT/` directory (234 files) aside before the quit and diffed it after the build. UBT `Log.txt` reads `UHT processed GitClaudeUnrealTestEditor in 1.8908307 seconds (8 generated files written)`. Exactly 8 files differ, with none added or removed:

- `GitClaudeUnrealTest.init.gen.cpp`
- `HeroCharacter.gen.cpp` / `.generated.h`
- `SiegePlayerController.gen.cpp` / `.generated.h`
- `SiegeControlsHelpWidget.gen.cpp`
- `WarMapWidget.gen.cpp` / `.generated.h`

Two files are byte-identical before and after: `SiegeControlsHelpWidget.generated.h` and `SiegeGameMode.gen.cpp`/`.generated.h`. `SiegeControlsHelpWidget.gen.cpp` changed only in the `Detail` property's `Comment`/`ToolTip` metadata, the struct CRC and the file-registration CRC.

**Exec-symbol sets, compared as sets in both directions:**

| class | `DEFINE_FUNCTION` pre → post | post-only | pre-only | `DECLARE_FUNCTION` in `.generated.h` |
|---|---|---|---|---|
| help widget (3 classes) | 9 → 9 | ∅ | ∅ | 9 → 9, ∅ / ∅ |
| `AHeroCharacter` | 29 → 36 | `execGetMeleeCooldown, execGetMeleeHalfAngleDegrees, execGetMeleeRange, execGetRallyCooldown, execGetRallyDuration, execGetRallyRadius, execGetRallySpeedBonus` | ∅ | 29 → 36, same 7 / ∅ |
| `ASiegePlayerController` | 44 → 45 | `execGetDiscardAllCost` | ∅ | 44 → 45, same 1 / ∅ |
| `ASiegeGameMode` | 6 → 6 | ∅ | ∅ | 6 → 6 |
| `UWarMapWidget` | 11 → 11 | ∅ | ∅ | 11 → 11 |

**Line macros:** only `WarMapWidget` moved, `_h_775/_h_778` → `_h_785/_h_788`. The other files are unmoved:
- help widget `127/450/453/633/636/933/936`
- `HeroCharacter` `148/397/400`
- `SiegePlayerController` `203/206`
- `SiegeGameMode` `139/142`

## 6. (4) Bounded suite on the gated build: **573 / 573 green**, and the per-class Blueprint statuses

`Tools/run_suite_bounded.ps1 -Porcelain`, editor closed: `RUNNER_STARTED=573 RUNNER_COMPLETED=573 RUNNER_SUCCESS=573 RUNNER_FAIL=0 RUNNER_SKIPPED=0`, echo 1/1, log `Saved/Logs/run_suite_bounded_suite_20260929-133542.log`.
- `Result={Success}` ×573 only. `Expected '` lines: 0. `Error:` lines: 0. `BS_Error`: 0. `ResolveCardActorClass`: 0.
- **Reconciled by name** against `TASK-1578`'s 568 run (`…20260929-004926.log`, 568 unique paths): **+5 added, −0 removed**:
  - `+ Siegebound.ControlsHelp.DiscardRallyAndAttackNumbersAreReadFromTheirOwners`
  - `+ Siegebound.ControlsHelp.EachBuildingTypeShowsItsOwnHeightLimit`
  - `+ Siegebound.HelpAccessors.BuildingCardsResolveThroughTheControllersOwnResolution`
  - `+ Siegebound.HelpAccessors.EachGetterReturnsItsOwnProperty`
  - `+ Siegebound.HelpAccessors.HeroClassResolvesThroughTheGameModesOwnAsset`

**Blueprint-status reading (`qa/TASK-1593.md` W2), in the suite's own process.** Every status is `BS_UpToDate`, so every line below is a reading. There is no `BS_Error`, so there is no STOP.

Test 21 (`[ControlsHelp] height limit: …`), 8 building lines:

| card | class | `GetMaxStackHeightMultiplier()` on own default | native parent | override | status |
|---|---|---|---|---|---|
| ArrowTower | `BP_Building_ArrowTower_C` | 5 | Tower = 5 | no Blueprint override | `BS_UpToDate` |
| Wall | `BP_Building_Wall_C` | 5 | Building = 5 | no Blueprint override | `BS_UpToDate` |
| BombTower | `BP_Building_BombTower_C` | 5 | Tower = 5 | no Blueprint override | `BS_UpToDate` |
| BallistaTower | `BP_Building_BallistaTower_C` | 5 | Tower = 5 | no Blueprint override | `BS_UpToDate` |
| Barracks | `BP_Building_Barracks_C` | 5 | Barracks = 5 | no Blueprint override | `BS_UpToDate` |
| DeepMine | `BP_Building_DeepMine_C` | 5 | DeepMine = 5 | no Blueprint override | `BS_UpToDate` |
| CrystalTower | `BP_Building_CrystalTower_C` | 5 | Tower = 5 | no Blueprint override | `BS_UpToDate` |
| WatchTower | `BP_Building_WatchTower_C` | 2 | ClimbableTower = 2 | no Blueprint override | `BS_UpToDate` |

Clause line: `Cards.StackUpgrade height-limit clause as composed: '5 times that original height for the Arrow Tower, Wall, Bomb Tower, Ballista Tower, Barracks, Deep Mine and Crystal Tower, and 2 times for the Watch Tower' (8 building type(s), 2 distinct limit(s), 2 of 2 rendering(s) carry it)`.

Test 22, the hero and the fee:
- Hero class: `'/Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C' -> '/Game/Blueprints/BP_HeroCharacter.BP_HeroCharacter_C', status BS_UpToDate`. The fallback was not taken.
- The 7 hero values on the spawned class default, each equal to native `AHeroCharacter` ("no Blueprint override"): `GetMeleeRange()` 150 · `GetMeleeHalfAngleDegrees()` 30 · `GetMeleeCooldown()` 0.5 · `GetRallyRadius()` 600 · `GetRallySpeedBonus()` 0.25 · `GetRallyDuration()` 5 · `GetRallyCooldown()` 20.
- Fee: `ASiegePlayerController::GetDiscardAllCost() = 20 on the controller's class default (native; no Blueprint subclass)`.
- Row lines, each "shown on 1 of 1 rendering(s)":
  - `Cards.Discard` 'fee is 20 gold'
  - `Hero.Rally` 'within 6 metre' · 'of your hero by 25%' · ' for 5 second' · 'a cooldown of 20 second'
  - `Hero.Attack` 'within 1.5 metre' · 'cone reaching 30 degree' · 'at most once every 0.5 second'

HelpAccessors tests 2 and 3 (`[HelpAccessors]`):
- The same 8 cards, each `-> BP_Building_<id>_C, status BS_UpToDate`; `DeepMine` is `(Economy)` and the other 7 are `(Building)`. Summary: `34 rows read, 8 building card(s), 8 class(es) resolved.`
- Hero class `BP_HeroCharacter_C`, status `BS_UpToDate`.
- The getter-vs-property pins are all equal (the 7 hero values above, plus `DiscardAllCost` 20). `7 distinct values among the 8 pinned; 0 equal pair(s) inside AHeroCharacter.`

**Totals:** 9 distinct Blueprints (8 buildings + `BP_HeroCharacter`), all `BS_UpToDate`, printed 18 times across the two files. 8 of 8 buildings have no override (7 × 5, Watch Tower 2), and 7 of 7 hero values equal native. This matches `qa/TASK-1595.md` §3's expectation, so 5b uses the same values. No load-order artefact appeared: no Blueprint load or compile message showed up inside any help test.

## 7. (5) The 9 required arms: each seen red on its own (`SHIP-§9`)

**Procedure.** The tools were `scratchpad/t1596/arm.py` and `run_arm.sh`.
- **Before any arm:** the 3 target files (`SiegeControlsHelpWidget.cpp`, `HeroCharacter.h`, `SiegePlayerController.cpp`) and `SiegeGameMode.h` were copied aside by byte copy, at the gated sha.
- **Per arm:**
  1. Assert all 10 files are at the gated sha, and the target equals its aside copy.
  2. Assert the anchor count across all **297** source files under `Source/` is **1**.
  3. Python `bytes.replace(anchor, repl, 1)` on raw bytes. No regex, no PowerShell text I/O.
  4. Assert the replacement count rises by exactly 1 and the anchor count falls to 0. cap-i's replacement `GetDefault<ABuilding>()` already occurs twice in the widget `.cpp`, both inside `//` comments (`:451`, `:1504`), so the check is "+1", not "absent".
  5. Assert the byte delta, the mutant size, an unchanged CR count and no BOM.
  6. `Build.bat`, and read `Result: Succeeded` from its log.
  7. Run the FULL bounded suite and read every `Result={Fail}` with its `BeginEvents`/`EndEvents` error lines.
  8. Restore by `shutil.copyfile` from the aside copy, then assert the sha, the byte-equality and all 10 files at the gated sha.
- **Batching, said plainly:** one mutant at a time, each with its own build and its own full-suite run. A restore was not built on its own. The next arm's build compiled the restored bytes: the arm-R build recompiled the restored `HeroCharacter.h`'s dependents (19 compiles, including `HeroCharacter.cpp` and `SiegeControlsHelpWidget.cpp`). The last restore was built at the end (§8).

| arm | file | mutant sha256 · size (Δ) | build | suite (started / success / fail) · log `…20260929-` | red test (the ONLY `Result={Fail}`) | errors | first error line | restore = gated |
|---|---|---|---|---|---|---|---|---|
| **cap-i** | help `.cpp` | `e6ba3e67…6ae5` · 346140 (−31) | `Result: Succeeded` 7.70 s, 0 diag | 573 / 572 / **1** · `133833` | `Siegebound.ControlsHelp.EachBuildingTypeShowsItsOwnHeightLimit` | **1** | `Expected '(a-cap) Building type 'Watch Tower' (card 'WatchTower') is listed under the height limit its OWN class default holds (the nearest '<N> time' before its name)' to be 2, but it was 5.` (`(3343)`) | ✅ |
| **cap-ii** | help `.cpp` | `bd0eb114…9136` · 346218 (+47) | `Result: Succeeded` 7.23 s, 0 diag | 573 / 572 / **1** · `133952` | `Siegebound.ControlsHelp.EachBuildingTypeShowsItsOwnHeightLimit` | **1** | `Expected '(c-cap) Building type 'Deep Mine' (card 'DeepMine') is named exactly once in Cards.StackUpgrade's height-limit clause' to be 1, but it was 0.` (`(3318)`) | ✅ |
| **TD-Rally** | help `.cpp` | `89b3f0ef…a8e9` · 346164 (−7) | `Result: Succeeded` 9.41 s, 0 diag | 573 / 572 / **1** · `134100` | `Siegebound.ControlsHelp.DiscardRallyAndAttackNumbersAreReadFromTheirOwners` | **1** | `Expected '⛔ Row 'Hero.Rally' detail TEMPLATE types NO number - every number the page shows is read from its owner when the page is composed' to be false.` (`(3640)`) | ✅ |
| **DF-Discard** | help `.cpp` | `718fdeb1…ccbc` · 346175 (+4) | `Result: Succeeded` 7.67 s, 0 diag | 573 / 572 / **1** · `134212` | same test 22 | **2** | `Expected 'Row 'Cards.Discard' shows the discard fee (GetDiscardAllCost, controller class default, gold) as its owner holds it: 'fee is 20 gold'' to be true.` (`(3568)`) | ✅ |
| **DF-Rally** | help `.cpp` | `29d123ea…5aa9` · 346173 (+2) | `Result: Succeeded` 7.18 s, 0 diag | 573 / 572 / **1** · `134319` | same test 22 | **2** | `Expected 'Row 'Hero.Rally' shows the rally radius (GetRallyRadius, spawned hero class default, uu / 100 = metres) as its owner holds it: 'within 6 metre'' to be true.` (`(3568)`) | ✅ |
| **DF-Attack** | help `.cpp` | `e4d2a565…5b7f` · 346171 (0) | `Result: Succeeded` 7.65 s, 0 diag | 573 / 572 / **1** · `134423` | same test 22 | **2** | `Expected 'Row 'Hero.Attack' shows the melee cooldown (GetMeleeCooldown, spawned hero class default, seconds) as its owner holds it: 'at most once every 0.5 second'' to be true.` (`(3568)`) | ✅ |
| **N3** | help `.cpp` | `813a2030…eaf0` · 346171 (0) | `Result: Succeeded` 7.60 s, 0 diag | 573 / 572 / **1** · `134529` | `Siegebound.ControlsHelp.ShownNumbersAreReadFromTheirOwners` | **1** | `Expected 'Row 'Interface.MapMarks' names the circles in the plural when the owner's cap is not one: ' circles at once'' to be true.` (`(3027)`) | ✅ |
| **arm-G** | `HeroCharacter.h` | `e5f66e73…612a` · 95800 (0) | `Result: Succeeded` 20.70 s, 0 diag | 573 / 572 / **1** · `134648` | `Siegebound.HelpAccessors.EachGetterReturnsItsOwnProperty` | **1** | `Expected 'GetRallyCooldown() returns RallyCooldown (AHeroCharacter class default; the property read by reflection by its name)' to be 20.000000, but it was 5.000000 and outside tolerance 0.000000.` (`SiegeHelpAccessorsTest.cpp(171)`) | ✅ |
| **arm-R** | `SiegePlayerController.cpp` | `e4c603fe…aa73` · 389497 (+62) | `Result: Succeeded` 22.37 s, 0 diag | 573 / 569 / **4** · `134810` | **4 tests**, see below | **6** | see below | ✅ |

**arm-R's red set: exactly the 4 tests and 6 errors of `qa/TASK-1595.md` §3.**
- `Siegebound.HelpAccessors.BuildingCardsResolveThroughTheControllersOwnResolution` · 1: `Expected 'At least one card in the table is a building card by IsBuildingCard (the set the Controls help lists is non-empty)' to be true.` (`SiegeHelpAccessorsTest.cpp(343)`)
- `Siegebound.ControlsHelp.DetailKeysDeriveThroughTheAccessor` · 3:
  - `Expected 'Row 'Cards.StackUpgrade' leaves no unresolved token in its body on QWERTY' to be false.` (`(1293)`)
  - `… on Dvorak' to be false.` (`(1295)`)
  - `Expected 'Row 'Cards.PlacementResize' related block 'Cards.StackUpgrade' leaves no unresolved token' to be false.` (`(1300)`)
- `Siegebound.ControlsHelp.ShownNumbersAreReadFromTheirOwners` · 1: `Expected 'Row 'Cards.StackUpgrade' leaves no unresolved number token in its composed detail' to be false.` (`(3010)`)
- `Siegebound.ControlsHelp.EachBuildingTypeShowsItsOwnHeightLimit` · 1: `Expected 'At least one placeable building type was enumerated through the game's own resolution, so (a-cap) and (c-cap) are measurements' to be true.` (`(3271)`)
- `ResolveCardActorClass` lines in the log: **0**. Other `Error:` lines: 0.

**The rest of each DF arm's error lines.** Each is the (a2) line at `(3602)`, `Expected 'Every rendering of row '<row>' shows the <number> (…) (its own page and each related block that renders it)' to be 1, but it was 0.`, for `Cards.Discard`, `Hero.Rally` and `Hero.Attack` in turn.

**Exact-text check.** The 17 observed `Expected '…'` messages, with the ` [file(line)]` suffix stripped, were compared by exact string equality against every backticked full `Expected '…'` line in `handoffs/TASK-1594-programmer.md` §6, `qa/TASK-1595.md` §3 and `qa/TASK-1593.md` §3.
- **16 of 17 match exactly.**
- The 17th, arm-R's Dvorak line, is published only in the abbreviated form `… on Dvorak' to be false.`. The observed text is the QWERTY line with `Dvorak` in its place, which is what the abbreviation stands for.

**Reading rules applied:**
- Every arm other than arm-R reddened exactly the named test with exactly the named count. There was no other red test, so no leak.
- **cap-i DISCRIMINATES.** Under cap-i, (d-cap) stayed green, and the composed clause still read "2 distinct limit(s)" from the reads while rendering one "5 times" group. Also, (d-cap) was green in both 573 runs, where the reads were 5 and 2.
- **cap-ii discriminates at today's data.** `DeepMine` is `Economy` in `DT_Cards` and resolves as a building (`[HelpAccessors] card 'DeepMine' (Economy) -> …`).
- **The DF arms each gave 2 errors**, so (a) and (a2) are both live.
- **TD-Rally did not turn test 22 (a) red**, and **N3 did not turn test 20 (a)/(a2) red.**
- **arm-G left test 22 green**, by design.

**Optional arms not run.** These are TD-Discard, arm-H and the six carried help arms (1576-T/D1/D2, 1585-A/B, 1574-guard). The row names `TASK-1592`'s arm-G and arm-R, and `qa/TASK-1595.md` §3 marks the rest optional. The dispatch said to run optional arms only if the row requires them.

## 8. (4) Restored build and final bounded suite: **573 / 573 green**

- **Re-hash before the final build:** 10 of 10 equal to §2.
- **Build:** `[1/4] Compile [x64] SiegePlayerController.cpp` · `[2/4] Link … .lib` · `[3/4] Link … .dll` · `[4/4] WriteMetadata` · **`Result: Succeeded`** · `Total execution time: 7.37 seconds` · 0 diagnostics. The DLL was written at 13:49:35: 10,172,928 B, sha256 `d7bbea6b03a7ea1d5ccca8489c17da2a53f5c89b78419a3eb26895a0055e9776`. That is the DLL PID 18832 loaded.
- **Suite:** `RUNNER_STARTED=573 RUNNER_COMPLETED=573 RUNNER_SUCCESS=573 RUNNER_FAIL=0 RUNNER_SKIPPED=0`, log `…20260929-134951.log`.
  - `Expected '` lines 0, `Error:` lines 0, `BS_Error` 0, `ResolveCardActorClass` 0, `LogAura` 0, `Response code: 401` 0.
  - The name set is identical to the gated run's 573.
  - Status tokens: `status BS_UpToDate` ×18, `(no Blueprint override)` ×15. The §6 reading is reproduced.
- **Save hygiene:** all 5 `Saved/SaveGames/*.sav` files have the same (sha256, size, mtime) before the quit and after the final suite.
- **Worktree:** `git status --porcelain` shows the same path set as at session start. Nothing new was created in the worktree apart from this handoff and the Build/Test logs under the ignored `Saved/`.

## 9. (6) Relaunch: PID 18832, PIE-clean (`VER-§12` cl. 7g)

- Launched at 13:51:13 with the same command line (plain `.uproject`, no `-game`, no extra flags). `:8000` was owned by 18832 at 13:51:27.
- **Editor log markers:**
  - `InternalLoadLibrary: 'GitClaudeUnrealTest'` :1694
  - `Starting MCP server on port 8000` :2231
  - `Total Editor Startup Time, took 13.445` :2412
- **Census:** `UnrealEditor.exe` 18832 only, plus `CrashReportClientEditor` 4880 (`-MONITOR=18832`) and `UnrealTraceServer` 28224 (`--sponsor 18832`).
- **The binary is live:** `(Get-Process 18832).Modules` maps `…\Binaries\Win64\UnrealEditor-GitClaudeUnrealTest.dll`, 10,172,928 B, sha256 `D7BBEA6B…9776`. That is §8's restored build.
- **Liveness:** `get_headless_status` → `editor_connected`, which is not a liveness proof on its own. `is_pie_active` → `is_active: false`, which is the game-thread-bound proof.

| reading (read-only `execute_unreal_python_readonly`, served by PID 18832) | value |
|---|---|
| `is_loading_assets()` (asset registry) | `False` |
| level | `/Game/Maps/L_Arena.L_Arena` |
| PIE / dirty | `IN_PIE=False` · `DIRTY_CONTENT=[]` · `DIRTY_MAPS=[]` |
| **in-memory `BS_ERROR` walk** (`unreal.ObjectIterator(unreal.Blueprint)`) | **0 of 19 resident**. Liveness: total 19 > 0; histogram `BS_UNKNOWN` 17 · `BS_UP_TO_DATE` 2 |
| live-reader control `find_object(None, <level>)` | found (`True`) |
| log frame index | `[772]` at 13:51:54 local (see §9a) |

- New log `Error:` lines: only the pre-existing `GameFeatureData` pair (:2072, :2077).
- **No PIE has been run on 18832, and nothing was loaded.** Every call was read-only. The 8 building Blueprints and `BP_HeroCharacter` loaded only in the suite's processes. In 18832 they are not resident beyond the editor's own startup set: 2 `BS_UP_TO_DATE` of 19.

### 9a. Frame counter

| sample (local) | log lines | last line's frame index | note |
|---|---|---|---|
| 13:52:02 | 2639 | `[772]` (20:51:54 UTC) | right after the first walk |
| 13:53:58 | 2639 | `[772]` | the idle editor wrote nothing in between, so the index could not move |
| 13:54:09 | 2642 | `[560]` (20:54:04 UTC) | after a second `is_pie_active` (→ `false`) and a second walk (→ `BS_ERROR` 0 of 19, same histogram) |

The index changed, so the game thread was ticking at 20:54:04. The bracketed index is the frame counter modulo 1000, so this shows that it moved, not by how much. On an idle editor the log grows only when something logs, so the three samples are not the "minutes apart, no new call" discriminator. The game-thread proof carried here is `is_pie_active` answering with a real object twice (13:51 and 13:54), plus the index moving across a call.

## 10. Board

- `TASKBOARD.md`: this row's `status:` line only → `built` (with `Edit`).
- The row's `names:` also lists "plus `built` on 1592 / 1594". The dispatch restricted me to this row's status line, so **`TASK-1592` / `TASK-1594` were NOT flipped**. That is for the orchestrator or manager to route.
- The 24 lag lines were untouched.

## Not examined / limitations

- **No runtime path was exercised.** 5b (PIE, the rendered sentences, page fit, and the W1 hitch observation) is `TASK-1597`'s.
- **One suite run per arm state**, not replicates. The restored bytes were built and suite-run once, at the end. The gated tree was also suite-run once, before the arms.
- **Optional arms were not run** (§7).
- **The `BS_ERROR` walk carries a liveness control only.** No discriminating fixture exists since `ab57522` (`VER-§12` cl. 7g, control strength).
- **The Blueprint statuses are the suite process's statuses** (`-nullrhi`, compile-on-load in that process). They are not a reading of PID 18832's memory, where these classes are not yet resident. `TASK-1597`'s own before-walk counts for the GUI editor.
- **Two same-source DLLs differ:** the gated-tree build (`43fb2494…`) and the restored build (`d7bbea6b…`), from byte-identical sources and of the same size (10,172,928 B). I did not examine which bytes differ (MSVC link non-determinism, as in `TASK-1578` §limitations). Source identity is carried by the file sha256.
- **The digits in the error lines** come from the `-nullrhi` suite process's culture (`en`).
