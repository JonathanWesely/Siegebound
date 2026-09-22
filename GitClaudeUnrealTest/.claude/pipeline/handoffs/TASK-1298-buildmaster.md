# TASK-1298 — build-master handoff, **5a LEG ONLY**

marker `TASK-1298-DEFECT-WAVE-HOST` · 2026-09-18 · build-master
**Status reached: `built`.** ⛔ **NO COMMIT MADE.** ⛔ `TASKBOARD.md` deliberately **NOT** written on this leg — the orchestrator fenced it to one writer while the manager amends the row. The `built` flip and clause (7)/(A2-7)'s **eight** flips belong to the commit leg.

Riders: `TASK-1294` (suite lane) · `TASK-1296` (menu focus) · `TASK-1307` (deck-builder post-flush read-back). Gates `TASK-1295` / `TASK-1297` / `TASK-1308`, all PASS on disk.

---

## 1. A2-1 / acceptance (10) — **DISCHARGED**, and it took two attempts by two different hands

| | sha256 | size | mtime |
|---|---|---|---|
| before | `fd9817f004a5f476211a47ca2c47dc43926cdf15f7a20f0c67e4bf2266954962` | 41634 | 2026-09-18 19:31:20 |
| after | **`6c1435265f77d22ae51da5916e9efd97a3f5ccafb0c9fb522be8db5775e614fa`** | **41782 (+148)** | 2026-09-18 23:38:30 |

**The hash CHANGED** ⇒ the compiled class is on disk and the invariant (`SC-§124`) holds: the class inside the asset that will be committed is the class compiled from the fixed graph. Discharged by 🧑 **Jonathan's BP-editor Compile + Save**, re-measured here by `sha256sum` (`SC-§40` cl. 1 — the coordinator's figure was a citation, not this host's measurement). Corroborated three further ways: the editor's own on-disk read returned the same digest; `BP Status = BS_UP_TO_DATE`; **dirty content packages 0 / dirty map packages 0** before the quit, so no save prompt was possible and the never-save posture held. Porcelain carried **exactly one dirty `.uasset`** (`BP_MenuGameMode`) — no level, no other asset.

### ⭐ THE FINDING THIS LEG PRODUCED — A2-1's programmatic lane was **unsatisfiable**, and the failure looked like success

First attempt (this host, granted bridge, ~21:4x) executed A2-1's prescribed lane in order and **failed the predicate**:

1. `AssetTools.is_dirty` → `false`
2. `BlueprintTools.compile_blueprint` → clean return; **the compile DID land in memory** (`Status` `BS_DIRTY` → `BS_UP_TO_DATE`)
3. `is_dirty` again → **still `false`**; dirty census 0/0
4. `AssetTools.save_assets(["/Game/Blueprints/BP_MenuGameMode"])` → returned **`true`** and **wrote nothing** (byte-identical file, **mtime not even touched**)

**Cause: a UE5 Blueprint compile does not dirty the package** — by design, so that compile-on-load does not dirty every asset in a project. The named single-asset save therefore had nothing to write and *correctly* reported success. ⚠️ **A `true` return over an unchanged file is the purest form of "a success return is not evidence"** — a host trusting the return value alone would have proceeded, and the hash gate is the only thing that caught it.

The lane is **closed, not merely untried** (measured, read-only): `compile_blueprint` exposes exactly `blueprint` + `warnings_as_errors` — no save/mark-dirty parameter; a sweep for `save|dirty|flush|persist` across **all 53** `BlueprintTools` entries returned **zero** hits; `save_assets` has **no force flag**. ⇒ **only the BP editor's unconditional Save path discharges this.** Two routes were refused on principle and are recorded as refused: a metadata-tag write to force dirtiness (would inject an unsanctioned tag into a committed asset) and the `execute_unreal_python` route (the tool QA and the programmer were refused; `qa/TASK-1297-report.md` credits them for not probing, and that binds this row).

⭐ **A2-2's UNMEASURED question is now ANSWERED: the granted bridge is OPEN.** No classifier denial and no permission prompt on any of four `mcp__unreal-mcp__call_tool` calls. ⚠️ **Argument-shape correction for future hosts:** `call_tool` takes **`toolset_name` and `tool_name` as separate parameters**; the fully-qualified single-string form the row quotes returns `Tool 'x' not found`.

⭐ **A2-1 was substantively right, and the suite could not have substituted for it.** The `.uasset` was written 19:31:20 while editor PID 2572 started only 21:07:07 — so QA's `BS_DIRTY` reading came from a **fresh** process that had loaded exactly those bytes. A fresh process arriving at `BS_DIRTY` (not `BS_Unknown`) means `PlayLevel.cpp:1290`'s guard does **not** exclude it, so **the headless `-nullrhi` suite would have auto-recompiled in memory too**: the false green reaches the suite lane, not just the live GUI editor. A green taken before the hash changed would have proved the fix and not the artifact — and a cooked build, which never recompiles on load, would have shipped the stale class.

## 2. Editor censuses — by command line, printed at every boundary (`SC-§118` cl. 8)

| when | result |
|---|---|
| before A2-1 | PID **2572** · GUI-editor · plain `.uproject` · started 21:07:07. **`-game` = 0.** No headless. |
| before the quit/compile | PID **2572**, unchanged (he used the same process). **`-game` = 0.** No headless. |
| immediately after the quit | **no `UnrealEditor` processes at all — field clear** |
| immediately before the relaunch | **field still clear — no headless auto-launch appeared behind the suite.** **`-game` = 0.** |

⚠️ Amendment (C)'s headless auto-launch **did not materialise tonight** — the census was owed regardless and is not evidence the hazard is gone. Self-match avoided by filtering `Win32_Process` on image name, never by grepping a shell's own command line.
**Graceful quit:** `CloseMainWindow` on PID 2572 → exited in **~8 s**, no force required, 0 dirty packages measured first. GUI editor only; no `-game` instance existed to protect.

## 3. Compile — `Result: Succeeded`, **from the log**

```
[1/16] Compile [x64] DeckBuilderWidget.cpp
[2/16] Compile [x64] SiegeMenuInputTest.cpp
[14/16] Link [x64] UnrealEditor-GitClaudeUnrealTest.lib
[15/16] Link [x64] UnrealEditor-GitClaudeUnrealTest.dll
Result: Succeeded
Total execution time: 18.81 seconds
```
Start 23:41:22 · end 23:41:41 · 19.1 s. `$LASTEXITCODE` was 0 and is **recorded only, never the verdict** (Build.bat returns 0 on a failed build). Not a ~2 s `0x800711C7` death ⇒ Smart App Control not in play.

**Acceptance (8) — warning delta:** total warning lines in the log **0** · `C4996` **0** (whole log, and on `DeckBuilderWidget.cpp`) · error lines **0**. Baseline at `1d4ae90` was `C4996 = 0` ⇒ **delta 0, and no new warning on any file**, so A2-3's by-file escalation and (E)'s "new warning on a file no rider touched" stop both stand down. Both rider `.cpp` files genuinely recompiled — this was not a no-op build.

## 4. Suite ×3, bounded — **561 / 561 / 561, zero reds**

Executor `Tools/run_suite_bounded.ps1 -Porcelain` (`TL-§6`'s named executor), three serial runs, GUI editor closed throughout so nothing contended for the DDC, `Saved/` or the mutex.

| run | log | started | completed | success | fail | skip | `RUNNER_EXIT` |
|---|---|---|---|---|---|---|---|
| 1 | `run_suite_bounded_suite_20260918-234245.log` | 561 | 561 | **561** | **0** | 0 | 0 |
| 2 | `run_suite_bounded_suite_20260918-234334.log` | 561 | 561 | **561** | **0** | 0 | 0 |
| 3 | `run_suite_bounded_suite_20260918-234424.log` | 561 | 561 | **561** | **0** | 0 | 0 |

**The triples:**

| run | `Response code: 401` | `LogAura` total lines | reds (by name) | `Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder` |
|---|---|---|---|---|
| 1 | **0** | **0** | **none** | `Result={Success}` — **exactly 1 occurrence** |
| 2 | **0** | **0** | **none** | `Result={Success}` — **exactly 1 occurrence** |
| 3 | **0** | **0** | **none** | `Result={Success}` — **exactly 1 occurrence** |

Distinct `Result={}` state across all three logs: **`Success` only** — asserted as state, not inferred from a tally (`SC-§104`). The engine string `Attempting to focus Non-Focusable widget` occurs **0** times in all three ⇒ `TASK-1296`'s coupling is proven from both directions: the pin is gone (`grep -c AddExpectedError` = 0 in source) **and** the test is green, which is only possible if the Error is genuinely absent.

**Delta reconciled BY NAME, not by total:** the set of `Path={…}` values was extracted from the pre-wave baseline log (`…-161724`) and from all three runs — **561 distinct paths each, zero added, zero removed, and the three runs' name sets are byte-identical to each other.** Expected delta 0, measured 0, by name.

⚠️ **Two honesty notes on the `401`:**
1. The `-DisablePlugins=Aura` flag was verified **present on the live suite command line** (not merely in the script): `-ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -DisablePlugins=Aura`. `LogAura` at **0 lines** is the strong form of `TASK-1294`'s claim — the plugin is not merely quiet, it is **absent from the lane**, so the race has no surface.
2. ⛔ **Tonight's three runs show the `401` ABSENT; they are not themselves the before/after contrast** — the `…-161724` baseline log is already a post-fix run (`401` count 0 there too). The evidence that the `401` ever redded a random test lives in `TASK-1291`'s four-run measurement and `TASK-1294`'s handoff. The "`401` fires and claims no victim" state (`TASK-1294` CONSEQUENCE 3) **did not occur** — there was no `401` at all.

⚠️ **My own instrument failed once and the positive control caught it.** A first extraction reported the subject test's verdict as `NOT FOUND` in all three logs. That was the **regex**, not the data: the log writes `Name={DownTwiceThenAcceptOpensDeckBuilder} Path={Siegebound.MenuInput.DownTwiceThenAcceptOpensDeckBuilder}` — the short name in `Name`, the qualified path in `Path` — so a pattern expecting the full path in `Name` can never match. Re-extracted by `Path`, with a deliberately-wrong-path control returning 0 to prove the pattern discriminates. ⛔ **An empty grep is not a zero**, and a `NOT FOUND` on a subject test must never be waved through as a pass.

## 5. Relaunch + Aura — acceptance (4)

Relaunched **PID 14432** at 23:47:01, GUI, project path, ⛔ no `-game`, ⛔ never Live Coding. MCP `:8000` answering after ~12 s.
**On the new binaries, confirmed twice:** command-line classification, **and** the module actually mapped into the running process — `Binaries\Win64\UnrealEditor-GitClaudeUnrealTest.dll`, mtime **23:41:40**, newer than the 23:41:22 compile start. (The in-process module read is the stronger check: a file on disk being new does not prove the process loaded it.)

**Aura still loaded — method quoted.** A read-only python probe served **by Aura's own MCP lane** against `pid = 14432` returned:
```
/Script/Aura package object:                <Object '/Script/Aura' … Class 'Package'>
/Script/AuraModelGenerator package object:  <Object '/Script/AuraModelGenerator' … Class 'Package'>
Aura UClasses resolvable:                   ['/Script/Aura.AuraEditorSubsystem']
```
Both modules `Aura.uplugin` declares are present, and the call being served at all proves the lane `VER-§7` depends on is reachable. ⇒ **`TASK-1294`'s exclusion is correctly scoped to the suite lane and did not disable the editor's plugin.** The contrast is clean: suite command line carries `-DisablePlugins=Aura` and logs 0 `LogAura` lines; the GUI editor carries no such flag and has Aura loaded.

## 6. Fences observed

⛔ Nothing staged, nothing committed, nothing pushed. Index clean; `HEAD` `89752eb`; `main` 3 ahead of origin. ⛔ `TASKBOARD.md` not written (orchestrator's concurrency fence). ⛔ `CONVENTIONS.md` not touched. ⛔ `Tools/run_suite_bounded.ps1` not touched — its stale "SANCTIONED IS NOT EXERCISED / NEVER launched UnrealEditor-Cmd.exe" comment block is now contradicted by four real launches (16:17 plus tonight's three) and is `TASK-1310`'s, fenced behind the commit. ⛔ `settings.local.json` · `Saved/**` · `Config/SiegeCloudDev.ini` · `GitClaudeUnrealTest.uproject` · `IMC_MainMenu` / `IA_Menu*` absent from porcelain throughout. ⛔ Only the GUI editor was ever closed, by command-line classification; `-game` count was 0 at every census.

## 7. What remains — for the orchestrator

1. **5b leg for `TASK-1296`** (`qa/TASK-1296-verify.md`) — **BINDING**, and per `qa/TASK-1297-report.md` §G it may **not** return `UNOBSERVABLE`. Its log read must come from a PIE that ran **after** the compile; PID 14432 satisfies that. 🧑 ~~He must **not** be asked to press Down/Enter on `L_MainMenu` — `SetIgnoreInput(true)` makes it impossible.~~
    - 🚨⛔ **STRUCK 2026-09-21 — ⛔ STRUCK, ⛔ NOT DELETED (`SC-§136` cl. 6). ⛔ SOURCE: 🧑 Jonathan's own hands, 2026-09-21, verbatim:** ***"ok, I opened a match, I saw the outline on the top menu option, hit the down arrow twice, and hit enter, and I was able to open the deck builder, so that menu navigation seems to be working fine."*** ⛔ **He did the thing the sentence above forbade anyone to ask him to do.**
    - ✅⛔ **THE PREMISE ⛔ STANDS, ⛔ UNRELAXED — ⛔ quoted back because a corrector who deletes it destroys a ⛔ TRUE, ⛔ LOAD-BEARING FACT:** `FInputModeUIOnly::ApplyInputMode` calls `GameViewportClient.SetIgnoreInput(true)` ⇒ a real key press does **not** reach **Enhanced Input** on `L_MainMenu`. ⛔ **Nothing he did measures that, and nothing he did refutes it.** ⛔ What is refuted is ⛔ ONLY the ⛔ HUMAN CONCLUSION drawn from it.
    - ⚖️⛔ **THE ⛔ ENTITLED SENTENCE (`VER-§8` cl. 11) — ⛔ this handoff now cites ⛔ THIS and ⛔ NOTHING BROADER:** ***a real key press on `L_MainMenu` ⛔ DOES reach the menu; ⛔ WHICH LAYER CARRIED IT IS ⛔ UNMEASURED.***
    - 🚨⛔ **⛔ MECHANISM: ⛔ UNMEASURED. ⛔ *"It reached Slate"* is ⛔ NOT asserted here as a finding.** ⛔ **BOTH routes predict his ⛔ IDENTICAL observable and his sentence discriminates ⛔ NEITHER: ⛔ (i)** the focused `SButton` → Slate's navigation config → `SButton::OnKeyDown`'s Accept path — ⛔ the ⛔ better-supported hypothesis and ⛔ **STILL A HYPOTHESIS** (`SC-§101`); ⛔ **(ii)** `USiegeMenuInputSubsystem`'s `IA_Menu*` handlers — ⛔ which would mean the ⛔ PREMISE above is ⛔ wrong. ⛔ **Matching a prediction is ⛔ NOT discriminating two mechanisms that predict the same thing.**
    - ⚖️⛔⭐ **PROVENANCE — ⛔ THE LESSON THIS CORRECTION EXISTS TO RECORD, AND IT IS A ⛔ METHOD LINE ABOUT ⛔ THIS HANDOFF, ⛔ NOT A RE-VERDICT OF `TASK-1298`:** the struck sentence ⛔ **cited `qa/TASK-1297-report.md` §G and ⛔ re-derived ⛔ NOTHING** — it is a ⛔ faithful copy of that report's `:200` (*"he must **not** be asked to press Down/Enter on `L_MainMenu`: `SetIgnoreInput(true)` makes that impossible regardless of this row"*), ⛔ a sentence that was ⛔ ALREADY WRONG at the source it was copied from. ⇒ ⚖️ ***⛔ A HOST THAT CARRIES A CLAIM FORWARD ⛔ INHERITS ITS ⛔ ERROR BAR, ⛔ NOT ITS ⛔ AUTHORITY*** (`SC-§101`). ⛔ **Citing an upstream gate is ⛔ not re-derivation, and a copy adds ⛔ no evidence to what it copies.**
    - ⛔ **SCOPE OF THIS CORRECTION (`TASK-1386`): ⛔ the struck sentence ⛔ ONLY. ⛔ NO hash, ⛔ NO pathspec, ⛔ NO compile result, ⛔ NO PID, ⛔ NO gate token in this file was altered — ⛔ the `BINDING` / `UNOBSERVABLE` clause of this very item, `PID 14432`, `HEAD` `89752eb`, the `6c143526…e614fa` digest and `Result: Succeeded` all stand ⛔ byte-unchanged.**
2. **5b leg for `TASK-1307`** (A2-5, `qa/TASK-1309-verify.md`) — pin the **full** token `POST-FLUSH read-back: IDENTITY=MATCH`, exactly once (PIN 1: the PRE-FLUSH line can legitimately contain the bare `IDENTITY=MATCH` substring); on any `NO-MATCH`, PIN 2 requires the focused `SWidget` address with a builder/parent/child classification. Net zero on his real deck: both `.sav` hashes byte-identical across the run.
3. **`TASK-1294` has no runtime criterion — the suite IS its instrument** (`VER-§5`), and it is satisfied three times over.
4. **5c commit** by exact pathspec from the git root (**one level up**, `SC-§102`), A2-6's message line, exactly one `.uasset` with its LFS read-back **oid-vs-sha256 against the post-A2-1 digest `6c143526…e614fa`** (never the pre-compile one, never size), then A2-7's **eight** flips with `TASK-1309` as `absorbed`, never `done`.
