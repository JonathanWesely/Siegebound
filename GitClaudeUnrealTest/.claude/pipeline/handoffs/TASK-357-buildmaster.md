# TASK-357 — [M8-P1-int] M8 Phase-1 integration — build-master handoff

- **Author:** build-master · 2026-07-29 · base HEAD `08c4a24` (CASTLE-3X complete)
- **Outcome:** compile **GREEN** · two-client P1 gate **FAILED** (3 blockers + 1 flagged, all TASK-356) · **no code commit** · docs committed
- **Scope note:** run MENU-FREE — TASK-355 is BLOCKED (rootless `WidgetTree`, `handoffs/TASK-355-artist.md`), so host/join was driven from the command line instead of the WBP.

---

## 1. Phase A — compile + editor

- **Joint build of TASK-356 + TASK-354** (first time both lanes compiled together): `Result: Succeeded`, 16 actions, clean link of `UnrealEditor-GitClaudeUnrealTest.dll`. `LogSiegeNet` (declared in 354's header, consumed by six 356 files) resolved at link — the sign-off §9.9 cross-lane coupling works.
- **`git status --short` new-files belt (TASK-354-QA's deferred Mandate 1): PASS.** The four 354 files were the only new source entries; every `M` was one of 356's enumerated 9 file pairs (+ board/CONVENTIONS). `Build.cs` and `Config/DefaultEngine.ini` untouched (D13 intact).
- Editor closed no-save (never-save law; no PIE was active), relaunched, MCP up, `L_Arena` loaded with MapCheck 0/0 and zero BP compile-on-load errors.

## 2. THE RECIPE — how to re-run the two-client gate without a menu and without a desktop

Worth keeping: this is now the project's repeatable networked-verification lane, and it works **headless and desktop-locked**.

**Constraint hit this session:** the desktop was LOCKED the whole run. Per the lane's injection law a locked desktop blocks `SendInput`, so there was **no console and no keyboard/mouse lane** — screenshots return the Windows lock screen, not the game. Everything below is input-free.

**(a) Twin processes** — the command-line equivalents of `HostListenMatch()` / `JoinMatch()` (the subsystem only composes these URLs):

```
# host (listen server)
UnrealEditor.exe "<proj>.uproject" "/Game/Maps/L_Arena?listen" -game -windowed -ResX=1100 -ResY=620 -WinX=0    -log -LOG=M8_host.log   -NOSPLASH
# joining client
UnrealEditor.exe "<proj>.uproject" "127.0.0.1:7777"            -game -windowed -ResX=1100 -ResY=620 -WinX=1120 -log -LOG=M8_client.log -NOSPLASH
# standalone practice regression
UnrealEditor.exe "<proj>.uproject" "/Game/Maps/L_Arena"        -game -windowed …                              -log -LOG=M8_solo.log   -NOSPLASH
```
Separate `-LOG=` names are essential — otherwise the processes fight over one log file.

**(b) Drive + read each process over UE python remote execution.** `PythonScriptPlugin` loads in `-game` (it is the editor binary), and the setting can be flipped per process on the command line — **no ini file is edited**:

```
-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRemoteExecution=True,[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:RemoteExecutionMulticastGroupEndpoint=239.0.0.1:<PORT>
```
Give each process its **own multicast port** (host 6768, client 6769, solo 6770; the editor keeps the default 6766) so they can be addressed separately, and pass a matching `RemoteExecutionConfig` (`multicast_group_endpoint`, plus a unique `command_endpoint` port) to the bundled `remote_execution.py` client. Runner used: scratchpad `ue_exec_port.py <snippet> <mcast_port> <cmd_port>`.

**(c) Getting at live game state from python in a `-game` process** (no editor subsystems exist there):

```python
w = unreal.find_object(None, "/Game/Maps/L_Arena.L_Arena")     # the live world
actors = unreal.GameplayStatics.get_all_actors_of_class(w, unreal.Actor)
castle.call_method("GetCurrentHP")                              # reflected UFUNCTIONs work
unreal.GameplayStatics.apply_damage(castle, 1500.0, None, None, unreal.DamageType)   # authoritative world damage on the host
gi  = unreal.GameplayStatics.get_game_instance(w)
sub = unreal.find_object(None, gi.get_path_name() + ".SiegeSessionSubsystem_0")       # GameInstance subsystems resolve BY PATH
```
Gotchas: `unreal.SubsystemBlueprintLibrary` does not exist in this build (resolve subsystems by object path instead); plain `UPROPERTY(Replicated)` members without Edit specifiers are **not** readable via `get_editor_property` (use the BlueprintPure getters — `GetCurrentHP`, `GetMaxHP`, `IsCastleDestroyed`); `role` reads fine, `remote_role` does not.

**(d) What this lane can and cannot verify.** It verified every gate below without a single keystroke. It cannot see UI (no screenshots while locked) — HUD/bar/end-screen appearance still needs a human or an unlocked desktop; those were verified through their underlying state + logs instead.

## 3. Gate results

**PASSED (verified live, both machines):** D2 networked latch + `SpawnBot skipped — networked 1v1` (zero bot controllers) · D3 seats (host `seated as Blue`, client `seated as Red`) · gold `COND_OwnerOnly` (client sees own 34; the host's true 116 reads a stale 10 on the client — enemy gold never crosses) · replicated clock 106.70 vs 107.57 s (±1 s) · D7 match end on BOTH (~70 ms apart) · D14 own-team-relative music (winning Blue host → `S_VictoryMusic`, losing Red client → `S_DefeatMusic`) · hero `Team` rep + OnRep capsule re-stamp (F2) · D5 observer lockouts (exact refusal log lines, zero client-side units) · host-initiated Play Again resets both · zero non-authority warns / ensure / AccessedNone / Fatal on both.

**Standalone / practice regression (gate g): FULL PASS** — bot spawned and played a real match (cards + miners, 28 units, player castle down to 980), economy live, match end, Play Again reset everything, and the §10/F7 `RestartPlayer` route reproduced the pre-M8 spawn exactly: `(-23800, 0, 98)` rot `(0,0,0)`. The byte-equivalence claim holds empirically.

**TASK-354's deferred parser sweep: 11/11 rejected, zero travels**, each with distinct user-facing text (details appended to `qa/TASK-354.md`). TASK-354 has **no defects** — it is uncommitted only because the M8 P1 lane lands as one unit.

**FAILED — 3 blockers + 1 flagged, ALL in TASK-356's files** (full evidence appended to `qa/TASK-356.md`):
1. **Scatter never replicates** → client has no battlefield at all (0 gold nodes vs 6). Cause: point actor at origin, `bAlwaysRelevant=False`, 150 m cull radius, players at 250 m.
2. **Enemy castle HP/crumble/destroyed never replicate** (host 500 / client 2000 at 488 m) while the near castle replicated perfectly (500/500) — the OnRep code is correct, relevancy is not.
3. **Red client hero spawns at the Blue PlayerStart** — `GetHeroStartTransform` calls `FindPlayerStart` before consulting `HeroTeam`, so the castle-relative Red branch is dead code.
4. **(flagged)** client `ServerRequestPlayAgain` ran locally, never reached the host; the client's own PC uniquely reads `bReplicates=False`. Channel proven alive (server corrected a client hero teleport). Needs one confirm through the real button once 355 lands.

**The through-line for 1+2:** M8 P1 replicates match-critical state on actors using UE's default 150 m distance relevancy, inside a **500 m** (M7.6 10×) arena. Recommend `bAlwaysRelevant`/arena-sized `NetCullDistanceSquared` on `ACastle` + `ASiegeBattlefieldScatter`, then a sweep of every M8-replicated actor against arena scale.

## 4. Git state

- **No code committed.** TASK-356 must change; the M8 P1 code lane (356's 9 pairs + 354's 4 new files) stays in the working tree for the programmer's loop-1 fix.
- **Docs committed separately** (pipeline `.md` only — audit, architecture + addendum, handoffs, both QA reports, board, CONVENTIONS). Rationale: ~30 KB of signed architecture and QA work was sitting untracked, and this repo has already lost never-committed docs once to a stray `reset --hard` (commit `1a04971`). The commit claims no integration. **No push.**
- **`Content/UI/WBP_SessionMenu.uasset` UNSTAGED** (`git restore --staged`) — it was auto-staged by the editor's source-control integration, is incomplete (rootless), and is left on disk for Jonathan's ~2-minute UMG step. `WBP_VictoryScreen` confirmed **not** dirty (no `Content/` diff beyond that one untracked file).

## 5. What the menu still owes (unchanged by this run)

`WBP_SessionMenu` needs a root panel + the six exactly-named widgets (`HostButton`, `JoinButton`, `BackButton`, `AddressTextBox`, `StatusTextBlock`, `ErrorTextBlock`) — then route (A) auto-wires with zero graph work. Still owed after that: the main-menu "Play Online" entry, the §9.6 VictoryScreen Play-Again rewire (`GetOwningPlayer → CastToSiegePlayerController → RequestPlayAgain`), and the §9.7 optional `SetLocalVictory` event. Menu-driven Host/Join and the bad-IP timeout surface remain the only TASK-354 items unverified — blocked, not failed.

## 7. RE-RUN #1 (2026-07-29, on TASK-356's loop-1 fixes) — gate ❌ again, but the relevancy law is PROVEN

**Compile: GREEN.** `Result: Succeeded`; `SetNetCullDistanceSquared` compiled clean (the `UE_DEPRECATED(5.5)` raw-field trap avoided). Editor closed no-save first (no PIE active), relaunched after.

**Both headline proof points PASS — loop-0 blockers 1 and 2 are CLOSED:**
- **Identical battlefields.** Client logs `Client regen: replicated seed=1429973633 generation=1` then regenerates with byte-identical `seed` / `mineStream=410182852` / `layers=7` / `corridorHalfY=1000` / `pairsPlanned=3` / `minesSpawned=6`. **6 gold nodes on the client (was 0).** Host↔client actor delta narrowed 14 → 8 (remainder is server-only).
- **The far castle tracks.** Same 488 m repro: host 500 / client **500** (was 500 / 2000). Near castle still 500/500. Crumble stage also crosses (`SM_Castle_Crumble03` on both, read off the mesh component) and so does destroyed state (`hp=0.0 destroyed=True` on the client after the kill).

**One new blocker (B5) — the whole gate now hangs on a single constant.** The blocker-3 reorder is *correct* (Red resolves castle-relative: `X=24400`, `Yaw 180` — the intended branch), but the spawn lands inside the Red castle: `SpawnActor failed because of collision … [X=24400 Y=0 Z=100]`, `SpawnDefaultPawnAtTransform: Couldn't spawn Pawn`, and `SiegePlayerController_1 pawn=None`. `HeroSpawnCastleOffset=(600,0,100)` vs the castle's measured colliding half-extent **1219** (span 23,781…26,219). The level's own Blue PlayerStart sits **1200 uu** out and spawns cleanly every time — that is the empirically validated floor (~1500 for margin). Note the same resolver feeds `RestoreHeroAtStart` / `HandleHeroRespawnTimer` / PlayAgain step 5, so every Red (re)spawn fails, not just login.

**Re-verified PASS:** D2 latch + SpawnBot gate · D3 seats · gold `COND_OwnerOnly` · D7 match end on both (~150 ms) · D14 own-team music · D5 lockouts · client `Team replicated: Red` · zero non-authority warns/ensure/AccessedNone/Fatal. **Standalone regression FULL PASS** — byte-equivalence survived the reorder: hero `(-23800, 0, 98)` rot `(0,0,0)`, bot spawned and played, economy live, zero spawn failures, match end + Play Again reset everything.

**⚠️ The one item this lane CANNOT close: real-button RPC routing.** The new law (remote-exec forces callspace Local via `FEditorScriptExecutionGuard`, so a scripted invoke proves nothing about routing) is correct and retires my loop-0 finding 4 — but it also means the D6 check needs genuine OS input, and **the desktop has been LOCKED for this entire session** (`LogonUI` present, console idle 2+ days). I proved the limit rather than assuming it: real `1`/`2` keypresses injected into the focused client window produced **no** `PlayHandSlot refused` line (counter stayed at 2). So: attempted, impossible, reported NOT-CLOSED — no scripted substitute. **Standing requirement for whoever closes it:** unlocked desktop (or Jonathan), client presses Play Again on the victory screen, expect `ServerRequestPlayAgain — client-initiated Play Again accepted` in the HOST log; file `executed WITHOUT authority — the RPC resolved LOCAL` if it appears on the client instead.

**Git:** still **no code commit** (P1 lands as one unit). `Content/UI/WBP_SessionMenu.uasset` remains unstaged on disk for Jonathan's UMG step. The `WBP_VictoryScreen` Play-Again rewire is in the tree, uncommitted, waiting on the same commit.

## 8. RE-RUN #2 (2026-07-29 evening, on TASK-356 loop-2 `qa-passed`) — **CODE GATE ✅ GREEN, D6 CLOSED, COMMITTED** · ONE NEW BLOCKER (art: `WBP_SessionMenu` layout)

**Outcome:** compile GREEN · every item on the `qa/TASK-356.md` loop-2 checklist **PASS** · **D6 CLOSED with genuine OS input** (the item carried across two runs) · standalone regression FULL PASS · **M8 P1 committed as one unit** · **NEW BLOCKER: `WBP_SessionMenu` renders no usable layout — its three buttons cannot be clicked by a human** (TASK-355's asset; code is not at fault).

### 8.1 Compile — GREEN, and the dispatch's premise was stale
`Result: Succeeded`, **0 actions, "Target is up to date"**. That is normally a red flag, so I did not accept it on trust: the module DLL `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` is stamped **14:37:00** and the newest loop-2 source (`SiegeGameMode.cpp`) **12:44:19** — the binary postdates the fix. Confirmed **empirically by string-scanning the DLL**: `"was refused for collision"`, `"AdjustIfPossibleButAlwaysSpawn"`, `"HeroSpawnCastleClearance"`, `"spawn distance"` are all **PRESENT** in the compiled image. **The loop-2 B5 fix was already compiled** — both latent traps QA cleared (the `FMath::Max(double,float)` deduction failure and the `const` `GetActorBounds`) are confirmed non-issues by a real successful link, not by argument.
**What actually happened:** an **interrupted RE-RUN #2 attempt** ran at 14:37→15:34 and was killed by the reboot before it could write a record (`Saved/Logs/M8c_*.log`, `M8d_*.log`). Its `M8d_host.log` already showed `spawn distance 1519 … -> (23481, 0, 100)` **four times** with zero collision refusals. So the reboot cost a report, not work.
**Editor bounce (Jonathan-authorized):** pre-flight verified `IsPIERunning=false` and `is_dirty=false` on `WBP_SessionMenu`, `WBP_MainMenu`, `WBP_VictoryScreen`, `L_Arena`, `L_MainMenu`. Closed PID 5304 **gracefully** (`CloseMainWindow`, no force-kill), exited clean with **zero save prompts** after 58 min uptime — nothing discarded. In hindsight the bounce was not needed (no compile was pending); it cost one relaunch and nothing else.

### 8.2 WHICH LANE PRODUCED WHICH RESULT (the dispatch asked for this explicitly)
| Result | Lane |
|---|---|
| Main menu render + **Multiplayer** entry | **REAL MENU** — real mouse click, real render |
| Standalone **Play (vs Bot)** entry | **REAL MENU** — real mouse click |
| **D6** client Play Again (victory screen) | **REAL MOUSE** on the real widget |
| Standalone Play Again (authority path) | **REAL MOUSE** on the real widget |
| **D5** observer lockout | **REAL KEYBOARD** (`1` keypress) |
| **Host / Join** | **§7 FALLBACK** (`HostListenMatch()` / `JoinMatch("127.0.0.1:7777")` invoked on the live `USiegeSessionSubsystem`) — **forced**, because the WBP's buttons are unreachable (§8.5). Not an RPC, so the callspace-Local law does not apply; what remains unproven is only the *button → C++ handler* hop. |
| All state readback (positions, HP, gold, seeds, overlap tests) | python remote-exec, read-only |

New processes: host **PID 8312** (multicast 6768), client **PID 14456** (6769), solo **PID 12708** (6770) → `Saved/Logs/M8e_host.log`, `M8e_client.log`, `M8e_solo.log`. Windows were repositioned with `MoveWindow` to fit the 3840×2400 desktop (UE multiplies `-ResX/-ResY` by the 2.5× OS DPI scale — worth knowing for the next run).

### 8.3 The loop-2 checklist — 5 of 5 PASS
1. **B5 is DEAD.** Client login logs the derivation verbatim: `castle X 25000, measured colliding half-extent 1219 + clearance 300 => spawn distance 1519 (authored floor 1500) -> (23481, 0, 100)`. **`SiegePlayerController_1 pawn=BP_HeroCharacter_C`** (not `None`), **two `BP_HeroCharacter_C` in the world on both machines**, Red at `(23481, 0, 98)` **yaw 180**, Blue untouched at `(-23800, 0, 98)` yaw 0. **Zero** `SpawnActor failed because of collision`, **zero** `Couldn't spawn Pawn` — on both logs.
2. **The fallback stayed ASLEEP.** `"was refused for collision"` = **0 occurrences** in all three logs. The defense-in-depth ruling holds on its stated condition.
3. **The respawn paths — the SILENT class — verified POSITIONALLY, not by absence of warnings.** Killed the client hero authoritatively → it respawned Red-side after the 5 s timer at `(23481, 0, 98)` yaw 180; Play Again → **both** heroes restored to their own sides, host and client agreeing. And rather than infer clearance, I ran a **direct geometry test** (`IsOverlappingActor` hero↔each castle) after every teleport: **`overlappingCastles=NONE` in all four readings on both machines**, against a measured near face of **23,781** vs the hero at **23,481** — exactly the 300 uu QA computed. The host log carries **exactly 3** derivation lines: login + 5 s respawn + PlayAgain step 5, i.e. all three live routes exercised.
4. **Standalone regression FULL PASS** (entered through the real **Play (vs Bot)** button): hero `(-23800, 0, 98)` rot `(0,0,0)`, bot spawned with its curated deck and played a real match (Cleric, Knight, Ogre, Deep Mine; economy ticking 51/33), match won, real-mouse Play Again → `PlayAgain: full match reset` → hero restored to `(-23800, 0, 98)`. **Zero `Castle-relative hero start` lines in the whole standalone log** — the Blue PlayerStart branch returns before the derivation is ever reached, which is the byte-equivalence claim QA made *by reading*, now confirmed *by measurement*. Zero spawn failures, zero ensures/AccessedNone/Fatal/material failures/net warnings.
5. **Loop-1 items stay green.** **B1:** the host and client `MinesPass` lines are **identical character-for-character** — `seed=1601093377 mineStream=304607556 pairsPlanned=3 minesSpawned=6 reserve=300` and every mine coordinate incl. the hill Z (`479`/`483`) and cull counts; layer counts match (Trees 340, Rocks 300, Grass 12248, Plants 2000); **6 `GoldNode` actors on BOTH**. **B2:** far castle at ~485 m damaged on the host reads **host 500 / client 500**, `SM_Castle_Crumble03` on both, and `hp=0 destroyed=True` crossed at the kill. **Tier-A control group:** replicated clock 98.90 vs 99.77 s (±1 s), castles both back to 2000/2000 after the reset on both machines, gold `COND_OwnerOnly` holding (client's own Red 109 tracks the server's 108; the enemy Blue stays stale at 10 against a server truth of 108 — enemy gold never crosses).
**Also re-verified:** D2 latch + `SpawnBot skipped — networked 1v1`; D3 seats (host Blue / client Red); hero `Team replicated: Red` + capsule re-stamp; D7 match end on both (**84 ms** apart); D14 own-team-relative music (host Blue-loser → `S_DefeatMusic`, client Red-winner → `S_VictoryMusic`); host↔client actor delta 139/131 = 8 server-only. Sweeps: **0** ensure / AccessedNone / Fatal / `LogNet: Warning` / `Failed to compile Material` on all three logs.

### 8.4 D6 — **CLOSED.** Real button, real routing
Two runs carried this open because the desktop was locked. It was unlocked, so I closed it with a genuine OS mouse click on the client's **Play Again** button on the real `WBP_VictoryScreen`:
- **client** `02:14:17.359` — `SiegePlayerController_0: RequestPlayAgain — client relay via ServerRequestPlayAgain (M8 doc §4.2).`
- **HOST** `02:14:17.365` (**6 ms later, across the wire**) — `SiegePlayerController_1: ServerRequestPlayAgain — client-initiated Play Again accepted (M8 gate e).` → `PlayAgain: full match reset`
- **Zero** `executed WITHOUT authority` anywhere.

**The RPC routes correctly.** This empirically confirms loop-1's refutation of my loop-0 finding 4: the code was always right and the defect was the *test lane* (python remote-exec resolving callspace Local). The same button on the standalone build took the authority branch instead (`PlayAgain` called directly, no relay line) — so the §9.6 VictoryScreen rewire is proven on **both** paths. **D5 also closed with real input** this run: a real `1` keypress on the client produced `PlayHandSlot(0) refused — P1 client observer posture (M8 doc §4.1)` — the exact probe that moved nothing when the desktop was locked, which retroactively validates that diagnosis too.

### 8.5 🔴 NEW BLOCKER (ART, not code): `WBP_SessionMenu` has no usable layout
**The bindings are real and the buttons are wired — the layout is not.** All six controls render **stacked at the viewport origin (0,0)**: at 1860×1802 the visible result is one ~165×48 px grey box plus two overlapping `"Text Block"` default labels in the top-left corner, and **nothing anywhere else on screen**. The `AddressTextBox` is painted last among the hit-testable controls, so it **completely covers `HostButton`, `JoinButton` and `BackButton`**.

Evidence (all real OS input, all negative):
- **200 real mouse clicks** on a grid over viewport `x=2..300, y=2..90` (the whole visible cluster and well beyond): **zero** `[SessionMenu]` log lines. The C++ logs on *every* press (`Host pressed.` / `Join pressed (address text '%s').` / `Back pressed`), so a reached button would have said so.
- **5 × Tab+Space** keyboard-navigation cycles: nothing.
- **Positive control proving input and bindings are fine:** clicking that same spot focuses `AddressTextBox`, and real typed `127.0.0.1:7777` **lands in it** (verified in pixels — dark text + caret inside the box). So the widget tree is live, the `BindWidgetOptional` bindings resolve, and input reaches the widget; only the geometry is wrong.

**Consequence:** a human cannot host or join from this menu — the whole point of TASK-355. TASK-355 is therefore **NOT `done`** (see §8.7). The fix is layout-only and small: give each of the six a real `CanvasPanelSlot` position/size (a vertical stack, the idiom `WBP_MainMenu` already ships and which renders correctly). The 6/6 binding readback the art-director reported was accurate — a binding readback simply cannot see geometry, which is exactly why this needed a render.
**Related, same asset, carried to Jonathan:** `AddressTextBox` has an **empty `HintText`** and C++ only reads it, so even once the layout is fixed the player sees a blank box with no `127.0.0.1:7777` format cue.

### 8.6 The commit
**`10c7b3d`** — `TASK-357: M8 P1 networked 1v1 (TASK-354 + TASK-356 code, menu/victory UI) — code gate GREEN, D6 closed; WBP_SessionMenu ships with a KNOWN layout defect`. One unit, on `main`, **no push**. Contents exactly as specified: **5 new sources** (`SessionMenuWidget.{cpp,h}`, `SiegeNetLimits.h`, `SiegeSessionSubsystem.{cpp,h}`) + **17 modified sources** + **all three UI assets** (`WBP_SessionMenu` 30,192 B, `WBP_MainMenu` 303,406 B, `WBP_VictoryScreen` 153,489 B — all LFS-filtered, confirmed via `git check-attr`) + the pipeline docs (this handoff, `qa/TASK-356.md`, `TASKBOARD.md`, `CONVENTIONS.md`, the 355/356 handoffs).
`WBP_SessionMenu` **ships despite the defect**, deliberately: it is a real asset with correct bindings, it is the artist's baseline to fix in place, and leaving 30 KB of hand-authored work untracked is how this repo lost docs once before to a stray `reset --hard`. The commit message says so plainly rather than implying the menu works. Laws honored: `L_Arena` never saved or committed (verified clean in the editor before the bounce and never touched after), no `reset --hard` / `clean -fd`, no `__ExternalActors__` residue (checked with `--untracked-files=all`), no push, `HEAD` verified at `6f82bf1` before staging.

### 8.7 Board flips and what I deliberately did NOT flip
`TASK-354 → done` · `TASK-356 → done` · `TASK-357 → done`. **`TASK-355` is NOT flipped to `done`** despite the dispatch's blanket instruction — its deliverable does not work for a human, and marking it done would bury a blocker that a later task would have to rediscover. It is set to **`needs-rework — layout defect`** with the evidence above; the manager owns whether that is a TASK-355 reopen or a new art task. Everything else about 355 landed and is verified: the **Multiplayer** main-menu entry works end-to-end by real click, all four pre-existing entries render in the recorded order (Play (vs Bot) → Sandbox (No Bot) → Deck Builder → Multiplayer → Quit), and the `WBP_VictoryScreen` Play-Again rewire is proven on both routing paths.

### 8.8 Carried to Jonathan (reported, not fixed)
1. **`WBP_SessionMenu` layout — the blocker above.** The owed pixel-check is now *done and it failed*; no further eyeballing is needed to know the verdict.
2. **The winning Red client reads "Defeat" on its victory screen** — I have it in pixels. Music was correct (`S_VictoryMusic`), only the text is absolute/Blue-relative. This is the already-recorded §9.7 / M8-P2 item (b) (`SetLocalVictory`), now empirically confirmed rather than predicted. Not gate-blocking.
3. **`AddressTextBox` empty `HintText`** — no `127.0.0.1:7777` cue for the player (§8.5).
4. **Inert empty `OnClicked_Event_8` stub** in `WBP_MainMenu`'s graph — cosmetic sweep candidate (8th of its kind).
5. **Main-menu pixel-check: PASSED** (by me, this run) — the 5-button list renders correctly and the Multiplayer → session-menu transition fires. The session-menu half of that check is item 1.

## 6. Machine state left behind

Editor **PID 37444 RUNNING**, MCP up, no PIE, `L_Arena` loaded and unsaved (the art-director's session; python remote execution still enabled in-memory, reverts on restart). All three test `-game` processes terminated. Test logs kept for the programmer: `Saved/Logs/M8_host.log`, `M8_client.log`, `M8_solo.log` (+ `M8_host.log.run1` / `M8_client.log.run1` from the first pass).
