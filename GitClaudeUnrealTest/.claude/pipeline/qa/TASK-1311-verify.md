# Verification — TASK-1311 (VICTORY-FOCUS-EMITTER), acceptance (4b)

Verdict: VERIFIED

verifier: playtest-verifier · 2026-09-19 · subject `TASK-1311` · gate `TASK-1312` (PASS) · host `TASK-1313` (`built`)
Reads performed whole (`SC-§38a`): TASKBOARD `#### TASK-1311` (`status:` + `blocked-by` + `parallel-safe` + spec (0)/(4a)/(4b)/(4c)) · `#### TASK-1313` spec (1)–(6) · `handoffs/TASK-1311-programmer.md` · `qa/TASK-1312-report.md` (incl. §5 and its `SViewport` lead).

## 🧑 Jonathan's go — recorded verbatim (`VER-§3`)

> **"Yes — drive PIE now"**

Asked and answered before this run. `VER-§3` satisfied; PIE was driven on that authority and on nothing else.

## Editor / Aura state

| item | measured |
|---|---|
| Aura / MCP | **connected** (`get_headless_status` → `editor_connected`) |
| editor identified by **COMMAND LINE** (`SC-§118`) | **PID 12000** — `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\GitClaudeUnrealTest.uproject"` |
| full Unreal process census | **exactly 2**: PID 12000 (GUI editor, above) + PID 26268 `UnrealTraceServer.exe daemon -d --sponsor 12000`. **`-game` instances = 0** ⇒ nothing of 🧑 his was touched, nothing was killed. No `-RenderOffScreen` / `-AuraHeadless` instance existed. |
| map | `/Game/Maps/L_Arena.L_Arena` — already the open level; **I did not call `load_level`** |
| PIE mode | standalone, 1 client, windowed 1280×720 (viewport reported 1280×725) |
| attempts used | **1 of 3** |
| wall time | ≈ 21 min end-to-end; the PIE session itself ran **307 s** of game time |
| credit | not surfaced by any tool this run |

**Binaries — proven, not assumed** (a zero on a stale binary would be worthless):
`SiegePlayerController.cpp` mtime **11:32:21** (and `SetWidgetToFocus` count **0**, 7,435 lines) → `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` built **12:12:08** → editor PID 12000 started **13:07:03**. Source precedes DLL precedes process ⇒ **the running editor carries the TASK-1311 deletion.**

⚖️ **Row-status precondition, stated rather than glossed.** My standing rule is to verify only a row reading `built`. `TASK-1311` reads `qa-passed`; the `built` token lives on its compile host `TASK-1313`, which is how this wave was boarded. I did not treat the token as the fact — I measured the binary chain above instead. Precondition satisfied **by measurement**.

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence) | verdict |
|---|---|---|---|---|
| **(4a)** | Route B state read: `grep -c 'SetWidgetToFocus' SiegePlayerController.cpp` = 0, surrounding lines byte-identical | static; **not mine** (`TASK-1312`'s, PASS) | Re-measured incidentally at my instant: the shipped file contains **0** occurrences of `SetWidgetToFocus`; post-edit block read at `:2261-2277` — comment `:2262-2267`, `bShowMouseCursor` `:2268`, `bEnableClickEvents` `:2269`, `FInputModeUIOnly InputMode;` `:2270`, `SetLockMouseToViewportBehavior` `:2271`, `SetInputMode(InputMode)` `:2272` | pass (corroborative only) |
| **(4b)** | the engine string `InputMode:UIOnly - Attempting to focus Non-Focusable widget` appears **0 times** in the log of a PIE instance that **actually reached a match end on `L_Arena`** | the `GitClaudeUnrealTest.log` slice written by THIS PIE instance, swept for the exact string **and** for the broader tokens | **Match end REACHED** (below). Slice = **79,294 bytes / 458 lines** (cursor `366,793` → file `446,087`). Exact string = **0**. Token `Non-Focusable` = **0**. Token `NonFocusable` = **0**. Token `InputMode` = **0**. Any case-insensitive `focus` = **0**. `LogPlayerController` lines in the slice = **0**. | **PASS** |
| **(4c)** | 🧑 his one-sentence hand check — *"Play a match to the end on the arena; when the Victory/Defeat screen appears, does clicking 'Play Again' restart the match? (The screen merely appearing is a NO. And under route B a keyboard press doing nothing is EXPECTED, not a failure — the only question is whether the CLICK still works.)"* | 🧑 **HIS**, not mine (`VER-§8` cl. 3(b)/cl. 4) | **NOT ANSWERED — still owed.** I did **not** click Play Again and I do **not** merge anything into his answer. Recorded beside the verdict, never inside it. | **owed to 🧑 him** |

## Was a real match end reached? — **YES**, and here is the chain, measured

1. **The world did the damage, not me.** At `t≈185 s` `Castle_Blue` (Team Blue, MaxHP 2000) read **`CurrentHP = 1970`** — already 30 HP down from enemy action. Twenty seconds later it read **1730**. All four marching units censused were **Team `Red`**: `BP_Unit_Knight0` / `BP_Unit_Knight1` (`AttackDamage 15`, HP 200), `BP_Unit_Ogre0` (35), `BP_Unit_Longbowman0` (18).
2. **Staging, declared exactly** (`pie_scene_edit` → `set_actor_property`, live PIE world only, nothing on disk): `Castle_Blue.CurrentHP` **`1730.0 → 30.0`** at `t = 204.99 s`. **1,700 HP were removed by my staged write.** Nothing else about the castle, the units or the game mode was altered.
3. **The last 30 HP came off organically, through the real `TakeDamage` path.** The per-tick state recording (`dt ≈ 0.228 s`, 168 samples, t0 = 204.96 s) shows: `30 → 15` between t0+0.157 s and t0+0.385 s, then `15 → 0` between t0+1.218 s and t0+1.385 s. **Two hits of exactly 15 damage — the Red Knight's `AttackDamage 15`.** No write of mine produced those two steps.
4. **`bDestroyed` flipped `false → true`**, `CurrentHP = 0`, confirmed at t = 211.0 / 217.0 / 223.0 s.
5. **The engine agrees, in the game's own log:**
   - `[2026.09.19-21.10.49:283][296]LogGitClaudeUnrealTest: [SiegeGameMode_0] Castle 'Castle_0' (Blue) destroyed — match over, winner: Red.`
   - `[2026.09.19-21.10.49:294][296]LogGitClaudeUnrealTest: ASiegePlayerController 'SiegePlayerController_0': match ended — winner Red.`

### ⭐ Why that second line makes the zero load-bearing rather than merely true

That log statement is `SiegePlayerController.cpp:2274-2276` — the **last statement of `HandleMatchEnd`**, which closes at `:2277`. It sits **immediately after** `SetInputMode(InputMode)` at `:2272`, i.e. **immediately after the exact lines the deleted `SetWidgetToFocus` call used to occupy**. Its presence in the slice proves the former emitter site **executed** on this run. The `Error` is absent from a log in which the site provably ran — not from a log that never reached it. This is the discrimination `TASK-1311` (4b) demands and `SC-§39` warns about ("a pin that cannot fail").

Corroborated at the widget layer: `WBP_VictoryScreen_C_0` existed in the live world, and `ui_snapshot` read `TextBlock_1` = **"Defeat"** (winner Red — correct) and `Overlay_19/SizeBox_0/Btn_Jump/TextBlock_0` = **"Play Again"**.

## The controls that make this a MEASURED zero (`SC-§39`)

| control | result |
|---|---|
| **Positive control — the needle fires** | The exact string was counted across **all 72** `Saved/Logs/*.log` files: **36 occurrences across 30 files** (1–2 each) — e.g. `M8_solo.log` 1, `M8e_solo.log` 2, `GitClaudeUnrealTest_2.log` 1, `GitClaudeUnrealTest-backup-2026.09.19-04.03.12.log` 1, `run_suite_bounded_suite_20260914-143207.log` 2. ⚖️ The dispatch said *"~33 older logs"*; **I measure 30 files / 36 hits** and report mine (`SC-§91`). |
| **Positive control re-fired at my instant, after the run** | Same reader, same session, post-PIE: `M8_solo.log` exact = **1**. The instrument was still firing at the moment I recorded the zero. |
| **Negative / wrong-pattern control** | `InputMode:UIOnly - Attempting to focus Non-Focusable gizmo` = **0** across all 72 logs **and** 0 in the slice. The reader is not matching everything. |
| **Broad sweep, not just the sentence** | The slice was swept for `Non-Focusable`, `NonFocusable`, `InputMode`, and any case-insensitive `focus` — **all 0**. I did not narrow the search to the one sentence that was deleted. |
| **The slice is not silently empty** | It carries **13 `: Error:` lines** — all `LogLiveCoding: Error: Cannot enable module …ggml-cpu-*.dll`, pre-existing and unrelated to this row. Errors *do* reach the slice; this one does not. |

Verbatim shape of the control hit, for anyone re-deriving it:
`[2026.07.29-19.34.29:875][315]LogPlayerController: Error: InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget [Widget.cpp(976)]!`

⚠️ **One property of the control worth writing down:** the engine message names only `SObjectWidget`, **never the concrete widget asset**. So a historical hit cannot be attributed to `WBP_VictoryScreen` versus the menu emitter **from the string alone** — attribution rests on *when* it fires and on `SetWidgetToFocus` now being 0 repo-wide. My zero does not depend on that attribution; it is the absence of **every** such line from a slice in which the site ran.

## Evidence (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-19/VER-TASK-1311-t03m53s-victory-screen-at-match-end.png` (616,423 bytes, 1086×615, PIE `game_time_seconds = 279.33`, frame 229651, `mean_luma 66`) — the composited end screen: **"Defeat"** in white at centre, a light **"Play Again"** pill button below it, `Gold: 216` top-left, the card hand along the bottom edge, `60 FPS / 16.7 ms` top-right. The 3-D view behind the UI is the **underside of the terrain** (dark green above the horizon line, sky-lit haze below) because the local player was a ghost pawn falling at z = −2,018 uu / 2,036 uu·s⁻¹ after the hero's death — that inverted-looking world is the **dead-pawn ghost camera**, not a rendering defect and not a product of this row.

Kept in `Saved/` (gitignored, not promoted): `Saved/AuraVerify/VER-TASK-1311-matchend-composited_t233.13s_f226903.png` — the same end screen 46 s earlier.

⚠️ **Named for the host, not acted on:** the promoted PNG is a **new untracked file** and it is **not** in `TASK-1313` spec (6)'s pathspec. I do not commit and I do not edit that row — `TASK-1313` decides whether to stage it.

## `.sav` NET ZERO — measured, before and after

5 files before, **5 after**, every one **byte-identical with its mtime untouched**:

| file | sha256 (head) | bytes | mtime (unchanged) |
|---|---|---|---|
| `SaveGames\SiegeAccounts.sav` | `2fd96fe18af9a4cf…` | 3083 | 2026-08-28 22:43:52.307531 |
| `SaveGames\SiegeDecks.sav` | `646d442fc11c2770…` | 3814 | 2026-08-02 10:09:34.952297 |
| `SaveGames\SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `7ad6029879fc8a25…` | 6521 | 2026-09-18 17:31:50.121201 |
| `SaveGames\SiegeSettings.sav` | `c8555088e2918c51…` | 2004 | 2026-08-04 22:24:38.554086 |
| `SaveGames\SiegeSettings_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `684ae64f9378bdf3…` | 2056 | 2026-09-09 13:48:19.329635 |

**NET ZERO = TRUE.** No `.uasset` was opened, compiled or saved; no level was loaded or saved; no Git command was run; no code or asset was edited.

## Hypotheses (not verdicts)

- **H1 — the deletion is why the slice is clean.** The static chain (`qa/TASK-1312-report.md` §1/§5: `bIsFocusable = False` → `SObjectWidget.cpp:175` → the sole emitter `PlayerController.cpp:6345`) plus 36 historical firings plus a proven execution of the site with 0 hits is consistent with exactly this. **HYPOTHESIS, not a verdict:** I did **not** run the pre-fix binary today, so the counterfactual ("it *would* have fired here") is **inherited**, never measured by me.
- **H2 — the hero-melee route failed for a reason I did not isolate.** My first staging teleported the hero to the enemy castle wall (x = 21,175 → depenetrated to 21,329.8, yaw ≈ 0, facing the castle) and injected `IA_Attack`; `Castle_Red.CurrentHP` stayed at 2000.0. Candidate causes, none measured: out of `MeleeRange 150` of the castle's real collision; `bMeleeSuppressed`; the injected Boolean tap not reaching `AHeroCharacter::DoMeleeAttack`. **Named as a dead end so it is not re-traced** (`VER-§5` cl. 5) — I abandoned it rather than chase it, because the Red units were already damaging my own castle organically.

## Explicitly NOT a finding on this row (`SViewport`)

`ui_snapshot` read `focused: false` on `WBP_VictoryScreen_C_0` **and** on `Btn_Jump`. Per `qa/TASK-1312-report.md` §5's measured lead — `FSlateApplication::SetUserFocus` walks **upward** to the first focusable ancestor and `SViewport::SupportsKeyboardFocus()` is `override { return true; }` (`SViewport.h:106`) — **the viewport holding focus at match end is the correct, expected post-Route-B state.** ⛔ It is **not** a failure here. The rule that makes it one belongs to **`TASK-1314`**, the future feature that would make Play Again keyboard-reachable. I did not test keyboard reachability and I make no claim about it.

## Not examined / limitations this run

1. **(4c) is unanswered and still owed to 🧑 him.** I never clicked Play Again, so I claim nothing about whether the restart works. *Adjacent observation, recorded beside his check and NOT merged into it (`VER-§8` cl. 4):* `Btn_Jump` read `visibility: Visible`, `hit_testable: true`, `enabled: true`, geometry 435.9 × 134.1 px at abs (1709.5, 1498.3). That says the button is **on screen and hit-testable**; it does **not** say a click restarts the match.
2. **1,700 of the castle's 2,000 HP were removed by my staged property write**, not by gameplay. Organic damage I measured totals **300 HP** (2000→1730 by the enemy, then 30→0 by two Knight blows). The *destruction event itself* — the 0-HP crossing that fires `OnCastleDestroyed` — was organic; the grind to get there was not.
3. **One match end, one PIE session, standalone only.** Not repeated, and **not** tested in listen-server or client net modes — `qa/TASK-1312-report.md` §5 notes the local-controller guard at `:2177`, so a multi-client fan-out is a different observation I did not make.
4. **A Defeat, not a Victory.** The end screen branch observed was `winner Red` → "Defeat". I did not observe the Victory branch; both go through the same `HandleMatchEnd` and the same `:2268-2272` block, but that is reasoning, not a second measurement.
5. **Scope of the zero.** It is scoped to the 79,294-byte / 458-line slice this PIE instance appended to `GitClaudeUnrealTest.log` — not to the whole session file (pre-PIE portion: 366,793 bytes, which itself already contained 0 hits) and not to any other log.
6. **`TASK-1313` status discrepancy, named not acted on.** That row's `status:` records the post-compile editor as **PID 18256**; that process no longer exists. The editor I drove is **PID 12000**, started 13:07:03 on the same (or newer) binaries — consistent with the dispatch's statement that it was relaunched after `b98b78a`. Not a defect; recorded so nobody re-derives it.
7. **I flipped only `TASK-1311`'s own `status:`.** No other row, no other agent's report, no source, no asset, no `CONVENTIONS.md`. I did not commit — `TASK-1313` owns 5c.

Law applied: `VER-§1` · `VER-§2` · `VER-§3` · `VER-§4` · `VER-§5` · `VER-§8` cl. 2/3/4 · `SC-§38a` · `SC-§39` · `SC-§91` · `SC-§101` · `SC-§118` · `SC-§126` cl. 10.
