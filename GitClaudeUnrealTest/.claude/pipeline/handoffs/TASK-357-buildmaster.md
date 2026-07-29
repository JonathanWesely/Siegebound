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

## 6. Machine state left behind

Editor **PID 37444 RUNNING**, MCP up, no PIE, `L_Arena` loaded and unsaved (the art-director's session; python remote execution still enabled in-memory, reverts on restart). All three test `-game` processes terminated. Test logs kept for the programmer: `Saved/Logs/M8_host.log`, `M8_client.log`, `M8_solo.log` (+ `M8_host.log.run1` / `M8_client.log.run1` from the first pass).
