# TASK-1523 — [SET-ACTIVE-REPEAT-FILTER-5A] — build-master handoff (5a only)

marker `TASK-1523-SET-ACTIVE-REPEAT-FILTER-5A` · 2026-09-26 · build-master
law: `CLAUDE.md` rule 5a · `SC-§87` · `SC-§118` · `VER-§2` cl. 2
**Status reached: `TASK-1521` → `built`; `TASK-1523` → `done`.** ⛔ NO stage, NO commit, NO push. ⛔ This host ran no git of any kind.

Gates on entry: `TASK-1521` was `qa-passed` (`qa/TASK-1522.md` line 1 `PASS`, 0 BLOCKER · 0 WARN · 4 NIT). No verification, import or other compile was live.

---

## 1. Editor census and graceful quit (`SC-§118`)

| when | result |
|---|---|
| before the quit | One `UnrealEditor.exe`, **PID 7008**, created 17:07:58. Its cmdline is exactly `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"`, so it is the GUI editor. **`-game` = 0.** MCP `:8000` was owned by 7008. |
| his hand-check session? | **No.** `is_pie_active` → `is_active: false`. Read by PID 7008 over MCP: `is_in_play_in_editor=False`, `dirty_content=[]`, `dirty_maps=[]`. Nothing was his to lose, so the stop condition did not fire. |
| quit | Re-identified inside the same call (name + exact cmdline + no `-game`), then `CloseMainWindow()` → `True` at 23:03:15. No force was used. No save prompt was possible, and none appeared. |
| editor log tail | `LogExit: Exiting.` · `Log file closed, 09/26/26 23:03:22`. 0 fatal/unhandled lines. No new `Saved/Crashes` folder (newest is still 2026-09-04). |
| before the compile | `UnrealEditor*` count **0**. The old `CrashReportClientEditor.exe -MONITOR=7008` lingered and was gone by the post-relaunch census. |

## 2. Compile — `Result: Succeeded`, quoted from the log

Full `Build.bat` per `CLAUDE.md`, editor CLOSED. ⛔ Never Live Coding (the header changed).

```
[Adaptive Build] Excluded from GitClaudeUnrealTest unity file: DeckBuilderWidget.cpp, SiegeDeckSlotsTest.cpp
[1/7] Compile [x64] SiegeDeckSlotsTest.cpp
[2/7] Compile [x64] DeckBuilderWidget.cpp
[3/7] Compile [x64] Module.GitClaudeUnrealTest.16.cpp
[4/7] Compile [x64] Module.GitClaudeUnrealTest.1.cpp
[5/7] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[6/7] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
[7/7] WriteMetadata GitClaudeUnrealTestEditor.target [NoUba]
Result: Succeeded
Total execution time: 11.29 seconds
```

- Start 23:03:30.8, end 23:03:42.5. Exit code 0 is **recorded only, never the verdict**. The run was well past ~2 s, so Smart App Control (0x800711C7) was not involved (0 hits).
- Warning lines **0** · error lines **0** · `C4456`/`C4458`/`C4459` **0** · `C4996` **0**. QA's name census is confirmed by the compiler: no shadowing.
- Both changed `.cpp` files genuinely recompiled, so this was not a no-op build. The two unity modules that include `DeckBuilderWidget.h` also rebuilt.
- **New DLL:** `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll`, mtime **23:03:41.639**, 10,063,360 bytes, sha256 `e06ff68ca93a8edc9dad91b4f74e3084352d34dfc73e6ce09e02388494125db4`.
- **Previous DLL** (`TASK-1510`'s): 17:05:55.657, 10,040,320 bytes, `ab25f5dc…6020b2`.

## 3. Exec-symbol set — asserted as a SET, both directions

The pre-compile `DeckBuilderWidget.generated.h` / `.gen.cpp` were copied aside before the quit and compared with the post-compile files.

| set | pre | post | pre-only | post-only |
|---|---|---|---|---|
| `DECLARE_FUNCTION(exec…)` in `.generated.h` | 31 | 31 | **∅** | **∅** |
| `DEFINE_FUNCTION(UDeckBuilderWidget::exec…)` in `.gen.cpp` | 31 | 31 | **∅** | **∅** |

- **Corroboration only; this is not the assertion.** UBT's `Log.txt` reads `UHT processed GitClaudeUnrealTestEditor in 1.3896659 seconds (0 generated files written)`. UHT parsed the changed header and found no reflected change. The generated files keep their old mtimes (`.generated.h` 2026-09-25 01:45:32).
- In the working-tree header, `UFUNCTION(` = **35** and `UPROPERTY(` = **8**, matching QA's 35/8. The new declaration is the plain `static bool IsHeldDeckBarActivationRepeat(const FKeyEvent& InKeyEvent, int32 FocusedBarSlotIndex);` at **`h:195`**. `GENERATED_BODY()` is still at `h:55`.
- Control: `execIsHeldDeckBarActivationRepeat` has **0** hits. The new static gained no thunk, as expected for a non-`UFUNCTION`.

## 4. Bounded suite (`SC-§87`) — **566 / 566 green, 0 failed**

Executor `Tools/run_suite_bounded.ps1 -Porcelain` (`TL-§6`). One run, editor closed. Command line: `-ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -DisablePlugins=Aura`.

```
RUNNER_EXIT=0
RUNNER_ECHO_MATCHED=1 / RUNNER_ECHO_EXPECTED=1
RUNNER_STARTED=566  RUNNER_COMPLETED=566  RUNNER_SUCCESS=566  RUNNER_FAIL=0  RUNNER_SKIPPED=0
RUNNER_LOG=...\Saved\Logs\run_suite_bounded_suite_20260926-230413.log
```

Wall clock 54 s; boot was observed at 14 s. No bound tripped.

- **Distinct `Result={}` states: `Success` ×566 only.** That is 566 `Test Completed` lines over 566 distinct paths. It is asserted as the state of every test, not inferred from a tally (`SC-§104`).
- **Both named tests were EXECUTED and are green:**
  - `Siegebound.Deck.HeldSetActiveKeyFiresOncePerPress` (new): one hit, log line 4363, `Test Completed. Result={Success} Name={HeldSetActiveKeyFiresOncePerPress} Path={Siegebound.Deck.HeldSetActiveKeyFiresOncePerPress}`. `Test Started` is at 4360, so it ran and was not merely discovered.
  - `Siegebound.Deck.KeyboardSetActiveOnAFocusedBarSlotDoesWhatRightClickDoes`: one hit, log line 4403, `Result={Success}`.
  - **Control:** the wrong path `…FiresOncePerPressX` returns **0** hits, so the pattern discriminates. An empty grep would not have counted as a zero.
- **Delta reconciled BY NAME** against `TASK-1510`'s run (`…20260926-170635.log`, 565, `Success` ×565): **+1 added = `Siegebound.Deck.HeldSetActiveKeyFiresOncePerPress`; 0 removed.** The expected delta was +1, and the measured delta is +1.
- `Response code: 401` **0** · `LogAura` lines **0**.

### Save hygiene: every `.sav` hashed before the quit and after the suite

| file | size | mtime | sha256 (before = after) |
|---|---|---|---|
| `SiegeAccounts.sav` | 3083 | 2026-08-28 22:43:52 | `2fd96fe18af9a4cfc4b1174497f992a5d841a21011787f3efe88624454d442b1` |
| `SiegeDecks.sav` | 3814 | 2026-08-02 10:09:34 | `646d442fc11c27704ef1e30962470c1a4db475667679c3268af7a5ee4039a071` |
| `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | 6520 | 2026-09-26 22:25:13 | `5bdd2fc0ffbf6a38d5b54d5a5a6a6b092e7b0621b57f94812487b7660f2bdd66` |
| `SiegeSettings.sav` | 2004 | 2026-08-04 22:24:38 | `c8555088e2918c517fdc88e6e877a1bd2ab860c38e5fecbd9ff5b27acc06f2e7` |
| `SiegeSettings_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | 2056 | 2026-09-09 13:48:19 | `684ae64f9378bdf34d5aa45a4b14c8048f46787c41952ad35202262feeb41793` |

- `Compare-Object` over (name, size, mtime, sha256) for every `.sav` shows the before and after sets **IDENTICAL**. **Zero change**, which is what QA predicted.
- `SiegeDecks_AutomationScratch.sav` was absent before and absent after. `FDeckScratchGuard` cleaned up after itself.
- The guest deck save moved since `TASK-1510`'s sighting: `0b0bd298…f3fce0` at 14:49:57 then, `5bdd2fc0…2bdd66` at 22:25:13 now. That write happened **before this row started**, most likely during his hand-check session. It was not caused by this row, because the before-hash was taken before the quit.

## 5. Relaunch on the new binaries

- Launched **PID 3108** at 23:05:31 with the same command line as the closed editor (plain `.uproject`, ⛔ no `-game`, no extra flags). The census shows exactly one `UnrealEditor*` process (3108) and `-game` = 0.
- **MCP `:8000` is owned by PID 3108**, listening by 23:05:48. The read-only probe served over MCP returned `pid=3108`.
- **The new DLL is loaded in-process.** `(Get-Process 3108).Modules` maps `...\Binaries\Win64\UnrealEditor-GitClaudeUnrealTest.dll`: mtime 23:03:41.639, 10,063,360 bytes, sha256 `E06FF68C…125DB4`. That equals the build output. There are no `-NNNN` patch DLLs on disk. Editor log line 1694: `LogModuleManager: InternalLoadLibrary: 'GitClaudeUnrealTest' ('…/Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll')`.
- After the relaunch: `is_in_play_in_editor=False`, `dirty_content=[]`, `dirty_maps=[]`.
- The new editor log has 2 `Error:` lines, both the `GameFeatureData` asset-manager settings pair. These are **pre-existing** (`TASK-1510` §5 found exactly the same 2 in each prior log) and unrelated.

## 6. Fences observed

- ⛔ No git of any kind. Nothing staged, committed or pushed.
- ⛔ Not touched: `CONVENTIONS.md`, `CLAUDE.md`, `.claude/agents/*`, and any code, asset or recipe.
- `TASKBOARD.md`: only the `status:` lines of `TASK-1521` and `TASK-1523` were written. Each was re-read immediately before its `Edit`, anchored on text unique to its own row (`SC-§127`), and grepped back afterwards. The manager was editing in parallel.
- ⛔ Only the GUI editor was closed, after command-line classification. `-game` count was 0 at every census.
- ⛔ No Live Coding. No save was performed or declined, because nothing was dirty.

## 7. For 5b (`TASK-1524`)

1. The editor PID is **3108** and it runs the new binaries (§5). Any PIE started now runs the `TASK-1521` code, including the adapter's repeat filter.
2. **Deck-save baseline:** at 23:03 the guest deck save `SiegeDecks_4E46A9EE…sav` read `5bdd2fc0…2bdd66` (6520 bytes, mtime 22:25:13), and it was untouched by the suite. That is newer than any value in `TASK-1510`/`1511`. Treat it as a **sighting**; the orchestrator's own before-hash governs. His hand check wrote it after `TASK-1510`'s sighting, so **`ActiveDeckName` and deck5's contents are UNMEASURED by this row.** Read both from disk before PIE, as the row's pre-registration already demands.
3. From QA (`qa/TASK-1522.md` §5b), restated only as a pointer:
   - A1/A2 are a first-press regression check only, never filter evidence.
   - A2's two refusal lines for two presses would have been two before this row too. Report them as baseline.
   - A3 (🧑 his hold) is the filter's sole instrument, and deck5 must read **empty (illegal)** before it is put to him.
4. **The filter is still unexercised at runtime.** The suite's new test pins the pure helper and a composed effect. It never calls the private adapter (QA N1). A reverted adapter would still pass 566/566.

## Not examined / limitations

- **No runtime path was exercised by this row.** No Slate key-repeat was generated, and no PIE was started. The adapter's wiring is carried by QA's read (N1) and by 🧑 `TASK-1524` A3.
- **One suite run, not three.** The row asked for a bounded run, not replication.
- **The exec-symbol comparison is against the pre-compile generated files on disk.** Those came from an earlier compile (generated.h mtime 2026-09-25). I held no git, so they were not diffed against a `HEAD` build.
- **The deck-save write at 22:25:13 was not attributed by measurement.** "His hand-check session" is an inference from the timing and the dispatch's note.
