# TASK-1510 — [DECK-KEYBOARD-SET-ACTIVE-5A] — build-master handoff (5a only)

marker `TASK-1510-DECK-KEYBOARD-SET-ACTIVE-5A` · 2026-09-26 · build-master
law: `CLAUDE.md` rule 5a · `SC-§87` · `SC-§118` · `VER-§2` cl. 2
**Status reached: `TASK-1507` → `built`; `TASK-1510` → `done`.** ⛔ NO stage, NO commit, NO push. ⛔ No git run at all by this host (a parallel `TASK-1506` host owns git).

Gates on entry: `TASK-1507` `qa-passed` (`qa/TASK-1509.md`, line 1 `PASS`). `TASK-1508` `done`, which its status line says meets the `ready-for-integration` gate; accepted as satisfied per the orchestrator's dispatch. No PIE was live (`is_in_play_in_editor` = False, read from PID 19396 before the quit).

---

## 1. Editor census and graceful quit (`SC-§118`)

| when | result |
|---|---|
| before the quit | one `UnrealEditor.exe`, **PID 19396**, created 14:32:34, cmdline `"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"`. That is the GUI editor. **`-game` = 0.** MCP `:8000` was owned by 19396. |
| dirty packages (read by PID 19396) | `dirty_content=[]` · `dirty_maps=[]` ⇒ no save prompt was possible. Nothing was discarded. |
| quit | Re-identified by command line inside the same call, then `CloseMainWindow()` → `True` at 17:05:18. The process exited within seconds. No force was used. |
| editor log tail | `LogExit: Exiting.` · `Log file closed, 09/26/26 17:05:25`. 0 fatal/unhandled lines. No new `Saved/Crashes` folder (newest is still 2026-09-04). |
| before the compile | `UnrealEditor*` count **0**. `CrashReportClientEditor.exe -MONITOR=19396 -RespawnedInstance` lingered briefly, then exited by itself before the suite. |

## 2. Compile — `Result: Succeeded`, quoted from the log

Full `Build.bat` per `CLAUDE.md`, editor CLOSED. ⛔ Never Live Coding.

```
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: DeckBuilderWidget.cpp, SiegeDeckSlotsTest.cpp
[2/11] Compile [x64] SiegeDeckSlotsTest.cpp
[3/11] Compile [x64] DeckBuilderWidget.cpp
[9/11] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[10/11] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
Result: Succeeded
Total execution time: 17.49 seconds
```

- Start 17:05:38, end 17:05:56. Exit code 0 is **recorded only, never the verdict**. The build ran well past ~2 s, so Smart App Control was not involved.
- Warning lines **0** · error lines **0** · `C4458` **0** · `C4996` **0**. QA's open shadowing question (Point 5) is ruled by the compiler: no shadow warnings.
- Both changed `.cpp` files genuinely recompiled; this was not a no-op build.
- New DLL: `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll`, mtime **17:05:55.657**, 10,040,320 bytes, sha256 `ab25f5dc281fb3c13e656142921967f753f26f21552fd13aad7391ffac6020b2`. The previous DLL was 09:14:54, 10,018,816 bytes.

## 3. Exec-symbol set — asserted as a SET, both directions

Pre-compile snapshot of `Intermediate/Build/Win64/UnrealEditor/Inc/GitClaudeUnrealTest/UHT/DeckBuilderWidget.generated.h` and `.gen.cpp` taken before the quit. Compared with the post-compile files.

| set | pre | post | pre-only | post-only |
|---|---|---|---|---|
| `DECLARE_FUNCTION(exec…)` in `.generated.h` | 31 | 31 | **∅** | **∅** |
| `DEFINE_FUNCTION(UDeckBuilderWidget::exec…)` in `.gen.cpp` | 31 | 31 | **∅** | **∅** |

- Corroboration, not the assertion (hash and size are unsound on their own): UBT's log says **`UHT processed GitClaudeUnrealTestEditor in 1.9300294 seconds (0 generated files written)`**. So UHT did parse the changed header and found no reflected change. `GENERATED_BODY()` is at `DeckBuilderWidget.h:55`, and the generated macros read `DeckBuilderWidget_h_55`, which matches.
- The header's `UFUNCTION(` count is **35**, matching QA's 35→35. The 31 exec thunks are fewer than 35 because events without a native thunk carry no `exec`. The difference is the same before and after.

## 4. Bounded suite (`SC-§87`) — **565 / 565 green, 0 failed**

Executor `Tools/run_suite_bounded.ps1 -Porcelain` (`TL-§6`). One run, editor closed. The command line carried `-nullrhi -unattended -NoLiveCoding -DisablePlugins=Aura`.

```
RUNNER_EXIT=0
RUNNER_ECHO_MATCHED=1 / RUNNER_ECHO_EXPECTED=1
RUNNER_STARTED=565  RUNNER_COMPLETED=565  RUNNER_SUCCESS=565  RUNNER_FAIL=0  RUNNER_SKIPPED=0
RUNNER_LOG=...\Saved\Logs\run_suite_bounded_suite_20260926-170635.log
```

The run's wall clock was 50 s, and boot was observed at 10 s. No bound tripped.

- **Distinct `Result={}` states: `Success` ×565 only.** That is asserted as the state of every test, not inferred from a tally (`SC-§104`).
- **The subject test was EXECUTED and is green:** log lines 4387/4390 read `Test Completed. Result={Success} Name={KeyboardSetActiveOnAFocusedBarSlotDoesWhatRightClickDoes} Path={Siegebound.Deck.KeyboardSetActiveOnAFocusedBarSlotDoesWhatRightClickDoes}`, exactly once. **Control:** a deliberately wrong path (`…DoesX`) returns **0** hits, so the pattern discriminates. An empty grep would not have counted as a zero.
- **Delta reconciled BY NAME** against this morning's run (`…20260926-091944.log`, 564/564 Success): **+1 added = `Siegebound.Deck.KeyboardSetActiveOnAFocusedBarSlotDoesWhatRightClickDoes`; 0 removed.** The expected delta was +1, and the measured delta is +1.
- `Response code: 401` **0** · `LogAura` lines **0**.
- **Save hygiene:** `Saved/SaveGames/` after the suite contains no `SiegeDecks_AutomationScratch.sav` (`FDeckScratchGuard` deleted it). Every real `.sav` mtime predates this session's work. `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` was last written 14:49:57 and reads sha256 `0b0bd2986ce8cc963247f3c1255b61b24c39bebe42589551c474c2993df3fce0` after the suite. This is a sighting for `TASK-1511`'s before-hash, not a replacement for the orchestrator's own.

## 5. Relaunch on the new binaries

- Launched **PID 7008** at 17:07:58 with the same command line as the closed editor (plain `.uproject`, ⛔ no `-game`, no extra flags). The census shows exactly one `UnrealEditor*` process, 7008.
- **MCP `:8000` is owned by PID 7008**, answering by 17:08:12. The read-only probe served over MCP returned `pid=7008`.
- **The new DLL is loaded in-process.** This is the stronger check: a new file on disk does not prove the process mapped it. `(Get-Process 7008).Modules` shows `...\Binaries\Win64\UnrealEditor-GitClaudeUnrealTest.dll` with mtime 17:05:55.657 and sha256 `AB25F5DC…6020B2`, the same as the build output. There are no `-0001` patch DLLs on disk. The editor log line 1694 reads `LogModuleManager: InternalLoadLibrary: 'GitClaudeUnrealTest' ('…/Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll')`.
- **`IA_MenuSecondary` resolves** (read-only): `/Game/Input/Actions/IA_MenuSecondary.IA_MenuSecondary`, class `InputAction`, `ValueType = BOOLEAN`.
- **`IMC_MainMenu`**: `DefaultKeyMappings.Mappings` = **14**. Rows 0–11 are the prior set. Row **12** is `IA_MenuSecondary <- Home` and row **13** is `IA_MenuSecondary <- Gamepad_FaceButton_Top`, each with 0 triggers and 0 modifiers. The on-disk sha256 is `1a1ee5af…b1db8dbaa` (7379 bytes), matching `TASK-1508`'s "after" value.
  - ⚠️ Instrument note: the deprecated `Mappings` property on `InputMappingContext` reads **0** in UE 5.8. The live rows are under `DefaultKeyMappings.Mappings`. A reader using the old property sees an empty IMC. That empty result is a **void, not a zero**.
- **Builder bind line:** 0 hits for `IA_Menu* action(s) bound` and 0 for `ABSENT…IA_MenuSecondary` in the new editor log. That is **expected**, because the line only prints when the Deck Builder opens. It is not a failure, and not yet a pass. `TASK-1511` owns the observation.
- Dirty packages after relaunch: `[]` / `[]`. PIE is not active.
- The new editor log has 2 `Error:` lines, both `GameFeatureData` asset-manager settings errors. These are **pre-existing**: exactly 2 appear in each of the three prior editor logs (00:05:25, 18:41:46, 16:10:19 backups). They are unrelated to this row.

## 6. Fences observed

- ⛔ No git of any kind was run by this host.
- ⛔ Nothing staged, committed or pushed.
- ⛔ Not touched: `CONVENTIONS.md`, `CLAUDE.md`, `.claude/agents/*`, and any code or asset.
- `TASKBOARD.md`: only the `status:` lines of `TASK-1507` and `TASK-1510` were written, with an `Edit` on anchors that were unique when I measured them.
- ⛔ Only the GUI editor was closed, after command-line classification. `-game` count was 0 at every census.
- ⛔ No Live Coding. No save was performed or declined, because nothing was dirty.

## 7. For 5b (`TASK-1511`)

1. The editor PID is **7008** and it is on the new binaries (§5). Any PIE started now runs the `TASK-1507` code.
2. **Bind line:** when A1 opens the Deck Builder, the log should show `7 IA_Menu* action(s) bound … every IA_Menu* asset resolved`. `ABSENT … IA_MenuSecondary` means failure. **An absent line means the builder never opened (or the log category is filtered). It is not evidence of a bind.**
3. **Relay trace:** door 3's bar walk logs one `Log`-level line per step: `RelayDeckBarNavigationKeyToSlate: IA_MenuRight -> 'Right' handed to Slate's own key route on deck-bar slot N …`. An absent line means the relay never ran; it is not a zero (QA note 6).
4. **No mouse click inside the builder before A1's injected steps** (QA N4: a clicked bar slot plus an armed `FocusedCardIndex` makes the injected route act on the grid).
5. **IMC reads:** use `DefaultKeyMappings.Mappings` (or `get_input_mapping_context_keys`). The deprecated `Mappings` property reads 0.
6. **W1 (held-key auto-repeat)** is still open for a manager ruling. QA suggests adding a hand sentence to A5: *"hold Home 2 s on deck4: does anything stutter?"*
7. The deck-save sighting after the suite is `SiegeDecks_4E46A9EE…sav` = `0b0bd298…f3fce0`, mtime 14:49:57 (§4). The orchestrator's own before-hash governs.

## Not examined / limitations

- The new code path is **not exercised at runtime** by this row. The suite's subject test covers the static resolver and the in-memory activation only (QA N5). Door 3's live binding, the relay's landing, and `Home` through the Slate doors all belong to `TASK-1511`.
- One suite run, not three. The row asked for a bounded run; it did not ask for replication.
- The exec-symbol comparison is against the pre-compile generated files on disk. Those were produced by the last compile (DLL 09:14:54). I did not diff them against a `HEAD` build, because I held no git.
