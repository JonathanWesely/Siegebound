# TASK-353 — M8 Phase-1 Network Architecture (the P1/P2 spec)

- **Author:** gameplay-programmer · 2026-07-28 · doc-only (no code, no editor, no Git, no board edits)
- **Evidence base:** `handoffs/TASK-352-audit.md` (all §-refs below are to it unless noted); TASKBOARD "M8 tasks" rulings 1–8 + the M8 PARALLEL LAW; CONVENTIONS "Networked 1v1 (M8)" (all clauses).
- **Gate this doc serves (ruling 5):** two clients, one `L_Arena` match, identical authoritative core state on both screens — castle HP both sides, own gold, match start/win/lose.
- **Sign-off ask:** the manager signs §0's decision table (each row cites its section). Two rows amend the board (§9.1, §9.6) — everything else is implementable exactly as TASK-354/356/357 are already specced.
- ⚠️ **Line refs into the TASK-349 lane** (`Castle.*`, `SiegePlayerController.*`, `SummonedUnit.*`, `HeroCharacter.cpp`) are HEAD `8a903c7` refs; the §7 addendum re-verifies them after TASK-350 lands.

---

## 0. Decision summary (the sign-off table)

| # | Decision | Where |
|---|---|---|
| D1 | Listen server; host = server + Blue; GameMode server-only; bot + ALL unit AI server-side; practice mode = the untouched standalone path | §1 |
| D2 | "Networked match" detection = `?listen` URL option at InitGame **OR** `GetNetMode() != NM_Standalone` at BeginPlay (dual latch `bNetworkedMatch`); gates SpawnBot + refuses Sandbox | §1.3 |
| D3 | Team seats assigned at `InitNewPlayer` by seat-latch (first login Blue, second Red, third+ warned Red); PS `Team` replicated PLAIN (not InitialOnly) + `OnRep_Team` | §2 |
| D4 | Hero `Team` assigned in `AHeroCharacter::PossessedBy` from the owning PS; replicated; melee damage authority-gated | §2.3, §3.6 |
| D5 | P1 client posture: the Red client is a **walking observer** — hero moves (engine replication), sees all core state; card/spell/discard/stance/group entries are locked out client-side with a refusal message | §4.1 |
| D6 | **Exactly ONE new RPC in P1: `ServerRequestPlayAgain`** (Reliable, WithValidation, on the PC). All 14 other mutation surfaces deferred to P2 with named RPCs + per-surface rationale | §4 |
| D7 | Match end/reset reach clients via **GameState replication** (`bMatchEnded` + `WinningTeam` + OnReps), NOT per-controller Client RPCs; GameMode's direct controller push is retired | §3.4 |
| D8 | Clock = replicated basis triple against `GetServerWorldTimeSeconds()` (3 writes/match), never a per-frame replicated float; client Tick is display-only; overtime latch replicated | §3.3 |
| D9 | Scatter: replicate `ChosenSeed` + `GenerationIndex`; client regenerates deterministically, **nav-validation culls stay server-only** (accepted rare over-block gap — provably never rubber-bands) | §3.5 |
| D10 | Per-player hero tracking (map of respawn timers, team-keyed start transforms, `RestartPlayer` override) lands in P1 — client heroes die in P1 sessions, the single-slot tracker breaks visibly | §3.4.4 |
| D11 | Session = `USiegeSessionSubsystem` + `USessionMenuWidget`, LAN/direct-IP only, no lobby/ready/deck-handoff; network/travel failures surface as FString BIEs + return to menu | §5 |
| D12 | `HeroCharacter.{h,cpp}` must be ADDED to TASK-356's file list (board omission); one WBP_VictoryScreen button rewire appended to TASK-355 (with a host-only fallback) | §9.1, §9.6 |
| D13 | No `DefaultEngine.ini` net config needed in P1 (parallel law d holds; event-driven rep fits defaults) | §6 |
| D14 | Victory presentation own-team-relative in C++ (music + optional by-name widget call); `SetWinner(ETeamId)` contract unchanged | §3.4.3 |

---

## 1. Authority model, concretely

**Listen server (ruling 1).** The host machine runs the ONE authoritative world and is also the Blue player. The joiner is a pure client and is Red. Dedicated servers, OnlineSubsystem/EOS/Steam, matchmaking: out of scope (ruling 2).

**What exists where:**

| Object | Server (host) | Client (joiner) |
|---|---|---|
| `ASiegeGameMode` | The one instance. Win handler, freeze, PlayAgain, SpawnBot, hero respawn, team seats | **Does not exist** (engine design). Everything a client needs from it arrives via GameState rep or a Server RPC (§3.4) |
| `ASiegeGameState` | Authoritative clock/overtime/match result | Replicated proxy; Tick is display-only (§3.3) |
| `ASiegePlayerState` ×2 (+bot's in standalone) | Authoritative gold/economy engines | Replicated proxies; `Gold` owner-only; **no accrual** (§3.2) |
| `ASiegePlayerController` ×2 | Server copy of both (validation home, future RPC bodies) | Each machine has exactly ONE local PC (its own); HUD/widgets/ghost/reticle live only there |
| Heroes ×2 | Server-spawned + authoritative (GameMode RestartPlayer) | Replicated ACharacter proxies; each client predicts its OWN hero's movement |
| Bot (`ASiegeBotController`) + ALL unit/building/projectile AI, spell resolution, spawning | Server-only. AAIController never replicates; unit sim runs where units exist — in P1 units exist ONLY on the server (bReplicates unset until P2) | Nothing (P1). P2 replicates the actors |
| Castles, GoldNodes, CaptureZone, Scatter | Level-placed, authoritative state | Level-placed local instances + replicated state (Castle §3.1, scatter seed §3.5; GoldNode/CaptureZone state = accepted P1 gaps, P2) |
| Widgets/feedback components | Host's local UI only | Client's local UI only. Never replicate; OnReps fire the existing delegates (audit §4 — zero widget changes) |

**1.2 Practice-mode coexistence rule (ruling 3).** A bot match IS the current single-player path, untouched: standalone NetMode ⇒ `HasAuthority()` is true everywhere, OnReps never fire (no connections), replicated UPROPERTYs cost nothing, `bNetworkedMatch` stays false ⇒ SpawnBot runs exactly as today. Every P1 edit must carry a per-site byte-identity argument (§10) — the TASK-307/327 no-op pattern, per the TASK-356 spec.

**1.3 The networked-match latch (audit §9 flag 1 — pinned here).** `ASiegeGameMode` gains `bool bNetworkedMatch` (Transient, not config):
- `InitGame`: `bNetworkedMatch = UGameplayStatics::HasOption(Options, TEXT("listen"))` — catches the real `open L_Arena?listen` flow before anything else runs, and orders correctly with the Sandbox latch: **if `bNetworkedMatch && bSandboxMatch`, force `bSandboxMatch = false` + one log line** (audit §9 flag 5 accepted: sandbox refuses to network).
- `BeginPlay` (before SpawnBot): `bNetworkedMatch |= (GetNetMode() != NM_Standalone);` — the belt that catches PIE "Play As Listen Server" (whose URL option plumbing through InitGame is not guaranteed) and any future travel path. The net driver exists by BeginPlay on every listen path.
- Consumers: SpawnBot gate (§2.2), sandbox refusal (above). Nothing else reads it in P1.

---

## 2. Team assignment (host=Blue / client=Red — ruling 4)

Fixes the audit's worst-3 hardcodes: §1b#2 (InitNewPlayer tags every human Blue), §1b#3 (SpawnBot unconditional), §1b#4 (hero Team never assigned).

**2.1 The seat latch — `InitNewPlayer` (SiegeGameMode.cpp:170-176 today).** Replace the unconditional `SetTeam(ETeamId::Blue)`:
```
Blue seat free  -> assign Blue, latch bBlueSeatTaken     (the host: first login on a listen server is always the local player)
else Red free   -> assign Red,  latch bRedSeatTaken      (the joiner)
else            -> assign Red + UE_LOG Warning (third+ login unsupported in P1; no kick logic)
```
Two new Transient bools on the GameMode. Logins serialize on the server game thread ⇒ deterministic. PlayAgain is in-place (no re-login) ⇒ seats persist correctly across resets. Standalone: exactly one login ⇒ Blue — byte-identical. `InitNewPlayer` (not PostLogin) keeps the tag at its existing site and runs BEFORE `RestartPlayer`, so the team is set before the hero spawns (spawn-transform resolve §3.4.4 and `PossessedBy` §2.3 both read a settled team).

**2.2 SpawnBot gate (SiegeGameMode.cpp:791-871).** New first line: `if (bNetworkedMatch) { UE_LOG(...); return; }` — in a networked 1v1 the Red seat is human; exactly one PS per team resolves (`GetPlayerStateForTeam` first-match ambiguity never arises). Standalone: latch false ⇒ byte-identical. The bot itself is UNCHANGED (ruling 3).

**2.3 Hero team at possess (HeroCharacter.h:336 — Team defaults Blue, never assigned today).**
- `AHeroCharacter::PossessedBy(AController* NewController)` override (server-side by engine contract): `Super`, then resolve `NewController->PlayerState` → `ASiegePlayerState` → `Team = PS->GetTeam()`; warn+keep-default when unresolvable. Runs on every possession including respawn repossess and Play-Again — idempotent.
- `Team` becomes `UPROPERTY(ReplicatedUsing = OnRep_Team)` (plus `GetLifetimeReplicatedProps` — new to the class; `#include "Net/UnrealNetwork.h"`, complete-type law). `OnRep_Team()`: P1 log-only seam (P2 hangs bar-tint refresh + CASTLE-3X capsule-channel stamping off it — §7).
- Ordering: InitNewPlayer (team set) → PostLogin → RestartPlayer → spawn+PossessedBy (reads it) → first replication of the pawn carries the correct Team in the initial bunch, so client-side BeginPlay on the proxy sees it (§7 hazard 1).
- **Single seam:** SetPlayerDefaults does NOT also write Team (no double-writer ambiguity).

**2.4 P1-deferred team hardcodes (explicitly NOT fixed in P1).** The PC placement/targeting/instant Blue literals (§1b#5/6/7), the unit-side Blue eligibility gates (§1b#9), viewer-relative team colors (§1b#13 — identity colors CONFIRMED for M8 per audit §9 flag 4), cheat defaults (§1b#12). Rationale: all sit behind entry points the P1 client cannot reach (§4.1 lockout) — fixing them without their Server RPCs would be untestable dead code. P2 fixes each alongside its RPC.

---

## 3. P1 replication design per class

House law applied throughout: `DOREPLIFETIME`/`_CONDITION` registration in `GetLifetimeReplicatedProps`; client reactions in `OnRep_<Property>` (bool props drop the `b` prefix in the handler name — `bDestroyed` → `OnRep_Destroyed`; this is the intended reading of the naming law, sign-off row D3/D7); `HasAuthority()` guard at every mutation site; OnReps fire the EXISTING delegates so no widget changes (audit §4).

### 3.1 ACastle — `Castle.{h,cpp}` [post-350]

| Property | Rep | OnRep | Fires |
|---|---|---|---|
| `CurrentHP` (float) | `ReplicatedUsing=OnRep_CurrentHP` | `OnRep_CurrentHP()` | `OnCastleHPChanged.Broadcast(CurrentHP, MaxHP)` — the existing bar/HUD delegate (Castle.h HEAD ~:37/:67), exactly what server-side mutation sites broadcast |
| `bDestroyed` (bool) | `ReplicatedUsing=OnRep_Destroyed` | `OnRep_Destroyed()` | `ApplyDestroyedState(bDestroyed)` — NEW private refactor extracting the visual/collision half shared by HandleDestroyed (hide, collision off, HP bar hide) and ResetCastle (restore). **Never** broadcasts `OnCastleDestroyed` (that is the server win-condition hook; match end reaches clients via §3.4) |
| `CrumbleStage` (int32; exact member name confirmed by the §7 addendum post-350) | `ReplicatedUsing=OnRep_CrumbleStage` | `OnRep_CrumbleStage()` | The landed stage-apply function, which MUST be absolute (stage N applied directly, not incremental) — join-in-progress delivers the final stage with no intermediate values; addendum verifies |
| `Team` | `DOREPLIFETIME_CONDITION(..., COND_InitialOnly)` | none | Level-authored identical both sides; belt-and-braces for client-side team checks (audit §2 row) |
| `MaxHP` | **not replicated** | — | Level/CDO-authored, identical both machines; OnRep_CurrentHP reads the local copy. One less property; revisit only if MaxHP ever becomes runtime-mutable |

- Constructor: `bReplicates = true;` (level-placed replicated actor — client instances match by name and receive updates; no dormancy tuning in P1).
- Authority guards: `TakeDamage` → `if (!HasAuthority()) return 0.f;` top line (belt — client call sites are also gated at their source, §3.6/§4.1); `HealOverTime` and `ResetCastle` → guard + log (server drives; clients converge via OnReps).
- Client cosmetics (hit-flash/shake/debris from OnRep deltas) are **P2 wiring** (audit §2 feedback row) — P1's OnReps drive ONLY the delegates/visual state above. Gate (c) needs the bar + crumble stage, nothing more.

### 3.2 ASiegePlayerState — `SiegePlayerState.{h,cpp}` [clean file]

| Property | Rep | OnRep | Fires |
|---|---|---|---|
| `Team` | `ReplicatedUsing=OnRep_Team`, **plain** (no condition) | `OnRep_Team()` | Log-only seam in P1 (no existing team delegate; P2 recolor hooks). |
| `Gold` | `ReplicatedUsing=OnRep_Gold`, `COND_OwnerOnly` | `OnRep_Gold()` | `OnGoldChanged.Broadcast(Gold)` — **directly, NOT via SetGold** (SetGold is the authority mutation choke; on the client the value already changed via replication — re-routing through the guarded choke would refuse or double-write). Documented in-code. |

**Why `Team` is plain, not `COND_InitialOnly` (deliberate divergence from the audit's suggestion):** InitialOnly snapshots at the actor's FIRST replication; any assign-after-first-rep ordering (the bot PS's spawn-then-SetTeam shape is exactly this, and login-edge timing is engine-internal) would freeze the wrong value on clients forever. Team never changes after assignment ⇒ plain replication has zero steady-state cost and no snapshot hazard.

**The accrual-fork kill (audit §3.1 — the worst timer hazard):** `BeginPlay` wraps `ResetGold()` (which seeds + starts the income timer) in `if (HasAuthority())`. The overtime-handler BIND stays on ALL copies — it is read-only display logic (`HandleOvertimeStarted` → `RefreshGoldRate` → HUD "+N/s"), and the client's latch arrives via §3.3's OnRep, so the client HUD rate updates correctly at 7:00. Client copies: no timer, no seed-write (the initializer `Gold = 10` matches `StartingGold` until the first owner-only rep lands).

**Mutation guards** (early-out `!HasAuthority()` + log; `SpendGold` returns false): `SpendGold`, `AddGold`, `AddIncome`, `RemoveIncome`, `AddMinerIncome`, `RemoveMinerIncome`, `RegisterMinerAlive`, `UnregisterMinerAlive`, `ResetGold`, `ResetEconomy`, `PauseIncome`, `ResumeIncome` (+ private `StartIncomeTimer` as belt). `SetGold` itself stays unguarded (private; reachable only through the guarded public surface — and OnRep must not be refused by it).

**Not replicated in P1:** miner counts / flat income / rate inputs. The P1 client cannot own miners (§4.1), so its `GetGoldRate()` composes base-only (+overtime) — truthful for what it owns. P2 replicates the counts owner-only.

**Client HUD seed race:** the client's HUD init path (PC BeginPlay-side) may run before the PS proxy arrives. TASK-356 adds a bounded next-tick retry loop (defer HUD init until `GetPlayerState<ASiegePlayerState>()` resolves; log on exhaustion). Standalone: PS exists on the first check ⇒ zero behavior change.

### 3.3 ASiegeGameState — `SiegeGameState.{h,cpp}` [clean file]

**Clock — replicated basis, never a per-frame float.** Three new/promoted members:

| Property | Rep | OnRep |
|---|---|---|
| `ClockBaseSeconds` (float — accumulated clock value at the last state change) | `ReplicatedUsing=OnRep_ClockBase` | `OnRep_ClockBase()`: recompute the derived display second and broadcast `OnMatchClockChanged` immediately (snaps displays after join and on reset) |
| `ClockBaseServerTime` (float — `GetServerWorldTimeSeconds()` at that moment) | Replicated (plain; same bunch as above — actor property bunches apply atomically before OnReps fire) | — |
| `bClockRunning` (promote the existing private bool to `UPROPERTY(Replicated)`) | plain | — |

- Authority: `MatchClockSeconds` accumulation in Tick is UNCHANGED (and now authority-gated); a private `PublishClockBasis()` updates the triple at exactly the state changes — BeginPlay-start, `StopClock`, `ResetClock` (~3 writes/match; leverages `AGameStateBase::GetServerWorldTimeSeconds()`, the engine's synced server clock).
- Non-authority Tick (display-only, audit §3.2 fork killed): derive `bClockRunning ? ClockBaseSeconds + (GetServerWorldTimeSeconds() - ClockBaseServerTime) : ClockBaseSeconds` (clamped ≥0), reuse the existing `LastBroadcastWholeSeconds` change detection to broadcast `OnMatchClockChanged` — and **never** latch overtime or play the sting locally.
- `GetMatchClockSeconds()` moves out-of-line: authority returns `MatchClockSeconds`; clients return the derived value. Late join: correct elapsed time by construction.

**Overtime:** `bOvertimeActive` promoted to `ReplicatedUsing=OnRep_OvertimeActive`. `OnRep_OvertimeActive()`: on the false→true edge broadcast `OnOvertimeStarted` (existing delegate → HUD indicator + sting + the PS rate re-derive via its bound handler, §3.2); the true→false edge (Play-Again reset) is silent — matching `ResetClock`'s re-arm semantic; reset displays converge via `OnRep_ClockBase`'s 0-broadcast and the §3.4 reset notify.

**Match result (new state — today match end lives only in GameMode + a direct C++ call chain, audit §1a/§2):**
- `bMatchEnded` (bool, `ReplicatedUsing=OnRep_MatchEnded`) + `WinningTeam` (ETeamId, Replicated plain — same-bunch atomicity makes the pair safe).
- Authority API: `SetMatchResult(ETeamId Winner)` — guard, set both, then call the shared `NotifyLocalControllersMatchEnd()` (server side covers the HOST's screen). `ClearMatchResult()` — guard, `bMatchEnded=false` (server-side notify NOT needed on clear: GameMode's PlayAgain step 6 already walks the server-side controllers, §3.4.2).
- `OnRep_MatchEnded()`: true-edge → `NotifyLocalControllersMatchEnd()`; false-edge → `NotifyLocalControllersMatchReset()`.
- `NotifyLocalControllers*()`: iterate `World->GetPlayerControllerIterator()`, act on `IsLocalController()` ASiegePlayerControllers only — **ban-compliant** (no first-controller assumption; on any machine at most one PC is local). End → `HandleMatchEnd(WinningTeam)`; reset → `PerformLocalMatchReset()` (§3.4.2).

**Defensive duplicate-team log (audit §9 flag 2, accepted):** `GetPlayerStateForTeam` warns (once per call site pattern, dev-only) when more than one PS claims the requested team.

### 3.4 Match flow — `SiegeGameMode.{h,cpp}` [clean] + the PC seam [post-350]

**3.4.1 Match end.** `OnCastleDestroyedHandler` (SiegeGameMode.cpp:201-248): the latch, respawn-cancel, and `FreezeWorldAtMatchEnd()` are unchanged; the direct `HandleMatchEnd` push loop (:234-241) is **retired**, replaced by `GameState->SetMatchResult(Winner)`. Host screen via the server-side notify; client screen via OnRep. Standalone: the notify resolves the same single local PC the old loop did — same observable call, new route (§10). A latency window where the client still presses inputs after the server ends the match is swallowed by: the client's own `bMatchEnded` PC latch (set by its local `HandleMatchEnd`), the §4.1 lockouts, and server-side authority guards (audit §3.7).

**3.4.2 Play Again.**
- Entry: WBP_VictoryScreen button → **`ASiegePlayerController::RequestPlayAgain()`** (new BlueprintCallable): authority (host/standalone) → resolve GameMode → `PlayAgain()` direct; client → `ServerRequestPlayAgain()` (the P1 RPC, §4.2). Requires a one-node WBP rewire — appended to TASK-355's editor session (§9.6; fallback: host-press-only still satisfies gate (e)'s minimal reading — "Play-Again resets both [screens]").
- Server body: `PlayAgain()` unchanged in ordering (clock before economy, :603 law), with two additions: `GameState->ClearMatchResult()` (right after the step-3b clock reset) and per-player hero restore (§3.4.4). The scatter re-roll (:693) drives clients automatically via §3.5.
- Client convergence: `OnRep_MatchEnded` false-edge → `PerformLocalMatchReset()` — NEW small PC helper = `HandleMatchReset()` + `ResetDeck()` on the PC's local UDeckComponent (the client-local mirror of PlayAgain step 6; the client's local deck exists per §4.3). All other client state converges via the Castle/PS/GameState/scatter OnReps (cross-actor OnRep order is not guaranteed — each reaction above is independent and order-tolerant; noted for QA).
- `HandleMatchEnd`/`HandleMatchReset` gain an `IsLocalController()` guard on their UI halves (CreateWidget/viewport/input work) so the server-side copies of the REMOTE client's PC (which GameMode step 6 still walks for their server-side state half) never build widgets for a non-local player. Standalone: local ⇒ guard is a no-op.
- Own-team-relative presentation (audit §1b#8, §9 flag 3): `HandleMatchEnd` picks Victory/Defeat music by `Winner == MyTeam` (own PS team, Blue fallback); the `SetWinner(ETeamId)` widget contract is UNCHANGED (absolute winner), plus an OPTIONAL by-name, null-safe `SetLocalVictory(bool)` call for the widget to consume when the art seam updates it (not gate-required). Standalone: MyTeam=Blue ⇒ identical.

**3.4.3 Match start.** No handshake in P1: the host's world IS the match; a client join lands in the running match and converges via initial replication (clock basis gives correct elapsed time). "Start" for gate purposes = both clients loaded + core state visible.

**3.4.4 Per-player hero tracking (D10 — replaces audit §1a#4/#5).** In a P1 session the HOST's units attack the client's hero, so client hero death/respawn WILL occur; the single-slot tracker visibly breaks (respawn targets the host only). All server-side, `SiegeGameMode.{h,cpp}`:
- Delete `FindLocalSiegeController()` (the ban-shaped "first = THE player" helper) and the single `TrackedHero`/`HeroRespawnTimerHandle`.
- `HandleHeroDied(DeadHero)`: resolve the owning controller at death time; per-controller timer map `TMap<TWeakObjectPtr<AController>, FTimerHandle>`; the timer delegate carries the weak controller into `RestoreHeroAtStart(AController*)` (now parameterized).
- `RestartPlayer(AController*)` override: resolve the controller's PS team → `GetHeroStartTransform(NewPlayer, Team, ...)` (already team-keyed, castle-relative fallback — SiegeGameMode.cpp:457-479) → `RestartPlayerAtTransform`. Red spawns by the Red castle (no Red PlayerStart exists in `L_Arena`; the fallback is the design — no P1 level edit). Standalone/Blue: the resolve returns the PlayerStart transform — same spawn as the engine path (§10; QA-scrutinize).
- PlayAgain restores ALL players' heroes (iterate player controllers); EndPlay clears all map timers.

### 3.5 Scatter seed — `BattlefieldScatter.{h,cpp}` [clean file] (P1-recommended per audit #7 — ADOPTED into P1)

Without it, host and client generate DIFFERENT battlefields (per-machine random seed, BattlefieldScatter.h:129-134) — mismatched visuals AND client-hero collision vs the server sim. Design:
- Constructor: `bReplicates = true;`
- `ChosenSeed` (int32, Replicated plain) — the authority's final chosen seed each generate (today logged only); `GenerationIndex` (int32, `ReplicatedUsing=OnRep_GenerationIndex`) — incremented on every authority `GenerateScatter` (fires client regen even when `bReRandomizeOnMatchReset=false` repeats a seed; join-in-progress gets index≥1 → regen on the initial rep).
- BeginPlay: generation is authority-gated. A client instance NEVER self-generates (its local random seed is the bug) — it waits for `OnRep_GenerationIndex()` → `ClearScatter()` + client-mode `GenerateScatter(ChosenSeed)`.
- **Client-mode generation runs the seed-deterministic passes only** (seeded placement, the reserved-corridor cull, the seeded mine-pair spawns — client-local mine actors land at identical positions). The **nav-reachability validation and its defensive widen-band/mine-radius culls (BattlefieldScatter.cpp:1412-1521) are authority-only**: they depend on live navmesh queries and an attempt counter and are NOT client-reproducible.
- **Accepted residual (sign-off row D9):** in the rare match where the server's defensive pass culled instances, the client keeps a handful the server removed — client obstacles are a SUPERSET of server obstacles. Provably never rubber-bands: CharacterMovement corrections fire when the client claims passage through a server obstacle, which a superset makes impossible; the client can only be locally over-blocked near the corridor. Log the attempt count on both sides; P2 upgrades to replicating the final cull bands if playtest feels it.
- P1 gap (noted): client-local mine/gold-node STATE (reserve, glow) is static — GoldNode replication is P2 (audit §2).

### 3.6 Hero (team only — the rest is P2) — `HeroCharacter.{h,cpp}` [post-350; §9.1 board amendment]

§2.3's PossessedBy/Team-rep, plus ONE combat guard: the melee damage application (`DoMeleeAttack` → `UGameplayStatics::ApplyDamage`, HeroCharacter.cpp:317 HEAD) gains `HasAuthority()` — the swing montage still plays locally; a client hero deals NO damage in P1 (§4.1 rationale). Rally/War-Banner pulses are left unguarded: on the client they iterate friendlies and find none (no client-side units exist in P1) — harmless vacuous no-ops, argued per-site (§10). Full hero replication (HP, upgrades, `ServerMeleeAttack`) is P2.

### 3.7 First-player-site fixes outside the above

- **SummonedUnit.cpp:1118 + :1145 [post-350] — the ruling-4 named offenders.** Replacement pattern (the owning-team-controller resolve, designed here as required): new `static ASiegePlayerController* ASiegePlayerController::FindControllerForTeam(UWorld* World, ETeamId Team)` — iterate `World->GetPlayerControllerIterator()`, cast, match `PlayerState` team, return first (null-safe; server-side it sees both PCs and resolves Blue→host / Red→client's server copy). Both unit sites call it with the unit's OWN `Team`. The `Team == ETeamId::Blue` stance/eligibility gates STAY (P2 scope — §2.4). Standalone byte-identity: one PC, Blue ⇒ the same controller the old first-controller call returned. Forward-compatible: in P2, group/stance state migrates to the SERVER copy of the owning controller (via that surface's Server RPCs), which this resolve already reads — the P1-safe treatment of the client's group orders is therefore "resolve by team on the server + client entry lockout (§4.1)", NOT a defer of the resolve itself.
- **SiegeFeedbackLibrary.cpp:156 [clean file] — the index-0 shake.** `PlayLocalCameraShake` re-resolves as a local-viewer loop: iterate `GetPlayerControllerIterator()`, `ClientStartCameraShake` on each `IsLocalController()` PC (exactly one per machine; no splitscreen). Ban-compliant local-viewer semantics. In P1 the client gets no castle shake (the call sites run server-side) — P2's OnRep cosmetic wiring adds it; standalone identical.
- **SiegeGameMode.cpp:768-789 `FindLocalSiegeController`** — deleted outright by §3.4.4.

---

## 4. The minimal P1 RPC surface

**4.1 The client posture decision (D5).** P1's gate is state visibility, not gameplay. The Red client is a **walking observer**: its hero moves (free via ACharacter replication + client prediction), it sees castle HP/crumble, its own gold accruing server-side, the clock/overtime, and the end/reset flow. It cannot mutate gameplay: the PC entry points `PlayHandSlot`, `DiscardHandSlot`, `SetUnitCommand`, and `BeginGroupPick` gain a `!HasAuthority()` early-out; the two card entries surface the existing refusal-message path with the constant `"Not available yet in online matches"` (manager may reword at sign-off), the stance/group entries log. Locking the four ENTRY points seals every downstream confirm (placement, spell target, instant, upgrades, Masons, group stages) without touching them. Standalone/host: `HasAuthority()` true ⇒ byte-identical. Rationale: any client-side execution of these surfaces forks local-only state (units the server can't see, gold writes the guards would refuse mid-flow) — dishonest exactly where the gate demands truth.

**4.2 RPCs that MUST exist for the gate — exactly one:**

| RPC | Site | Why the gate needs it |
|---|---|---|
| `ServerRequestPlayAgain()` — `UFUNCTION(Server, Reliable, WithValidation)`, on `ASiegePlayerController` | Called by `RequestPlayAgain()` when not authority. Validate: `return true;` (no input payload). Implementation: resolve GameMode → `if (HasMatchEnded()) PlayAgain();` (the matched-ended check IS the validation of intent) | Gate (e): the GameMode does not exist on the client — without this, the client's Play-Again button can reach nothing. Everything else in the start/win/lose flow rides GameState replication (§3.4), which is why ONE RPC suffices |

Hero melee needed a decision (task item 4): **the Red client damages NOTHING in P1** — the §3.6 authority gate. A client-side `ApplyDamage` on the castle would write the client's local `CurrentHP` under the replicated value (visible flicker-then-snap lie), and a Server-RPC'd melee is P2's combat-authority work, not a state-visibility need.

**4.3 The other 14 mutation surfaces (audit §1c) — all deferred to P2/P3, one-line rationale each:**

| # | Surface (audit §1c row) | P1 treatment | Deferred RPC (P2 unless noted) | Rationale for deferring |
|---|---|---|---|---|
| 1 | Card play entry (`PlayHandSlot`) | Entry lockout | `ServerPlayHandSlot(int32 SlotIndex)` | Needs server-side deck/hand truth for remote players (P3 deck handoff feeds it) — not a state-visibility need |
| 2 | Placement confirm (`TryConfirmPlacement`) | Unreachable (entry #1 locked) | `ServerConfirmPlacement(FName CardID, FVector Location)` + server re-validation (the miner-cap re-gate precedent) | Spawns must replicate before a client-visible spawn means anything — P2's actor replication |
| 3 | Discard (`DiscardHandSlot`) | Entry lockout | `ServerDiscardHandSlot(int32 SlotIndex)` | Same deck-truth dependency as #1 |
| 4 | Spell target confirm | Unreachable | `ServerConfirmSpellTarget(FName CardID, FVector AimPoint)` | `ResolveSpell` mutates world combat state — P2 combat authority |
| 5 | Instant spell | Unreachable | `ServerResolveInstant(FName CardID)` | Same as #4 |
| 6 | Hero upgrades | Unreachable | `ServerPurchaseUpgrade(FName CardID)` | Hero stat replication is P2 |
| 7 | Masons (castle heal) | Unreachable + `HealOverTime` authority belt (§3.1) | folds into #5 | Castle mutation must stay authority-only; the belt guarantees it |
| 8 | Group orders (pick stages + prune + clear) | `BeginGroupPick` locked; prune timer runs client-side over an empty array (harmless — §10) | `ServerConfirmGroupOrder(<final-stage payload>)` — pick stages 1–2 stay client-cosmetic, only the stage-3 group is gameplay state (audit §7a) | Group state must MOVE to the server-side owning controller (the §3.7 resolve reads it there) — a P2 store migration, not a P1 patch |
| 9 | Stance latch (`SetUnitCommand`) | Entry lockout | `ServerSetUnitCommand(uint8 Command)` | Controller-local stance the server units poll — same P2 migration as #8 |
| 10 | Hero melee | Authority gate at damage app (§3.6) | `ServerMeleeAttack(<target/seed payload>)` | Combat authority = P2; the gate keeps P1 honest |
| 11 | Rally | Vacuous no-op client-side (no client units) | folds into hero P2 surface | Nothing to buff client-side in P1 |
| 12 | War Banner aura | Vacuous no-op | folds into hero P2 surface | Same |
| 13 | Play Again | **THE P1 RPC** (§4.2) | — | — |
| 14 | Deck build (PC BeginPlay SaveGame load) | Left as-is: the client builds a client-LOCAL display deck (its plays refuse). **P2 flag:** the SERVER-side copy of a remote PC also runs this BeginPlay and would load the HOST's save file for the client's seat — before card play lands, the server deck for REMOTE controllers must build the curated DEFAULT deck (DeckCount column), not the host's save | Client deck → server handoff RPC (P3, audit §6 — `FDeckList` is RPC-serializable as-is) | Ruling 6 phase ladder; P1/P2 run the default deck for the client per the P2 deferral |
| 15 | Cheat manager | Untouched (dev tooling) | authority-gating sweep, P2+ | Audit §1c: flag, not P1 |

---

## 5. Session flow — TASK-354's contract

**`USiegeSessionSubsystem`** (`UGameInstanceSubsystem`, NEW `SiegeSessionSubsystem.{h,cpp}`) — survives travel by design; owns the M8 net log category `LogSiegeNet` (declared/defined here; TASK-356 code may use it — both lanes are in-tree at TASK-357's compile).
- `HostListenMatch()`: `UGameplayStatics::OpenLevel(this, TEXT("/Game/Maps/L_Arena"), true, TEXT("listen"))`. Status broadcast "Hosting — waiting for opponent". Null-safe (no world ⇒ log + error broadcast).
- `JoinMatch(const FString& Address)`: validate BEFORE any travel — trimmed non-empty, parses as IPv4 with optional `:port` (`FIPv4Address::Parse`; default port 7777); invalid ⇒ error broadcast + NO travel, never a hang. Valid ⇒ client travel to `<ip>[:port]` (the `open <ip>` equivalent via the local player controller's `ClientTravel(Address, TRAVEL_Absolute)` — the menu world's single local PC; session plumbing, not gameplay identity, noted vs the ban).
- `LeaveMatch()`: travel back to the menu map (`L_MainMenu`) on either role; on the host this drops the client, whose failure handler (below) returns it to its own menu. Clean + idempotent.
- Error surfaces: bind `GEngine->OnNetworkFailure()` + `OnTravelFailure()` at subsystem init → log verbatim + broadcast `FOnSessionStatusChanged(const FString&)` (one dynamic multicast the widget forwards) → auto-`LeaveMatch()` on hard failures (connection timeout = the bad-IP path — gate (f) graceful failure).
- Every transition logs on `LogSiegeNet`.

**`USessionMenuWidget`** (`UUserWidget` base, NEW `SessionMenuWidget.{h,cpp}`) — BlueprintCallable wrappers `HostPressed()` / `JoinPressed(const FString& AddressText)` / `BackPressed()`; BIEs `OnSessionStatusUpdated(const FString& StatusText)` + `OnSessionErrorShown(const FString& ErrorText)` (FString-only, widget law). TASK-355 builds `/Game/UI/WBP_SessionMenu` on exactly these six names and adds ONE "Multiplayer" entry to the existing main menu beside "Play (vs Bot)" (which stays untouched — ruling 3).

**What P1 does NOT do (stated per the task):** no lobby/ready flow, no deck handoff (the client plays the curated default deck when card play arrives in P2 — §4.3#14; SaveGame handoff is P3), no reconnect/rejoin, no seamless travel, no NAT traversal/internet matchmaking (LAN/direct-IP only, ruling 2), no player names/accounts, no spectator seats (third login = warned Red, §2.1).

---

## 6. File-ownership map for TASK-356

Per the M8 parallel law: [349-LANE] = blocked on TASK-350's code commit; [CLEAN] = not in the 349 lane (the board still blocks all of 356 on 350 — a clean-file carve-out is offered in §9.5, recommended AGAINST). Single owner per file within M8.

| File | Lane | Edits (all specced above) |
|---|---|---|
| `SiegeGameMode.{h,cpp}` | CLEAN | `bNetworkedMatch` latch + sandbox refusal (§1.3); seat-latch InitNewPlayer + SpawnBot gate (§2.1/2.2); `OnCastleDestroyedHandler` → `SetMatchResult` (§3.4.1); PlayAgain `ClearMatchResult` (§3.4.2); per-player hero map + `RestartPlayer` override + delete `FindLocalSiegeController` (§3.4.4) |
| `SiegeGameState.{h,cpp}` | CLEAN | Clock basis triple + display-only client Tick; `bOvertimeActive` OnRep; match-result state + notifies; duplicate-team warn (§3.3) |
| `SiegePlayerState.{h,cpp}` | CLEAN | `Team`/`Gold` rep + OnReps; BeginPlay authority gate; 12 mutation guards (§3.2) |
| `BattlefieldScatter.{h,cpp}` | CLEAN | `bReplicates`; `ChosenSeed`/`GenerationIndex` + OnRep regen; authority-gated validation split (§3.5) |
| `SiegeFeedbackLibrary.cpp` | CLEAN | Local-viewer shake resolve (§3.7) — the ONLY feedback-layer file touched |
| `Castle.{h,cpp}` | 349-LANE | Full §3.1 table + guards + `ApplyDestroyedState` refactor |
| `SiegePlayerController.{h,cpp}` | 349-LANE | `FindControllerForTeam` static (§3.7); `RequestPlayAgain` + `ServerRequestPlayAgain` (§4.2); 4 entry lockouts (§4.1); `HandleMatchEnd`/`HandleMatchReset` local guards + own-team music + `PerformLocalMatchReset` (§3.4.2); HUD PS-retry (§3.2) |
| `SummonedUnit.{h,cpp}` | 349-LANE | The two resolve replacements at :1118/:1145 — **nothing else** (no unit replication, no AI gates, no eligibility changes) |
| `HeroCharacter.{h,cpp}` | 349-LANE — **§9.1 board amendment required** | PossessedBy team assign; `Team` rep + OnRep; melee authority gate (§2.3/§3.6) |

**Explicitly NOT touched in P1:** `Building/Tower/Barracks/DeepMine`, `GoldNode`, `CaptureZone`, `Projectile`, `MinerUnit`, `DeckComponent`, `SpellLibrary`, `SiegeBotController` (ruling 3), `SiegeCheatManager`, all widgets/feedback components (beyond the one library function), `SiegeNavAreas` (349's new files), **`Config/DefaultEngine.ini`** (parallel law d — and per D13 no P1 net config is needed: all replication here is event-driven state fitting default actor net frequencies; if TASK-357's live session proves otherwise, the ini change lands THERE, post-350, documented). The group-orders first-player polls are RESOLVED (not deferred) per §3.7; group COMMANDS from the client are deferred via the §4.1 entry lockout.

---

## 7. CASTLE-3X gating addendum stub (audit §7b — code-delta addendum OWED after TASK-350 lands)

The two networked hazards this design already accounts for, to be RE-VERIFIED against the landed code:
1. **Client-proxy `Team`-at-BeginPlay ordering.** Both gating lanes (capsule object channels, gate-blocker response) read `Team` at BeginPlay. For dynamically spawned replicated actors the initial property bunch applies BEFORE client-side BeginPlay, so §2.3's possess-time hero assignment (set server-side pre-first-replication) and P2's `COND_InitialOnly`/spawn-data unit teams satisfy it — but the addendum must confirm the landed stamping site really is BeginPlay (not constructor) and that the hero's channel stamp re-runs if TASK-350 landed it at possess time (client proxies never run PossessedBy — an OnRep_Team hook may be needed there; the §2.3 OnRep seam is reserved for exactly this).
2. **Client-side gate-collision truth for the hero.** The client hero predicts movement locally: the gate blocker's per-team response and the hero capsule's own channel must configure IDENTICALLY on the client machine or the hero walks into the enemy castle locally and rubber-bands. Castle `Team` is level-authored (+ §3.1's InitialOnly belt), so BeginPlay-configured blocker responses should match — the addendum verifies the landed blocker config is NOT authority-gated and audits the hero-side stamp timing vs Team replication.
3. Nav filters stay safe by construction (server-only AI in P1 — no client proxy ever runs MoveTo); crumble-stage swaps ride §3.1's `OnRep_CrumbleStage` and by CASTLE-3X design can never disable the gating.

**The addendum lands as `handoffs/TASK-353-architecture-addendum.md` after TASK-350's code commit**, re-reading the landed `Castle/SiegePlayerController/SiegeBotController/SummonedUnit/HeroCharacter/SiegeNavAreas/DefaultEngine.ini` deltas, pinning the exact crumble-stage member/apply names for §3.1, and re-checking every 349-lane line ref in this doc. TASK-356 must not start its 349-lane files before reading it.

## 8. P1 verification plan (TASK-357)

Two lanes, per UE 5.8 listen-server realities (both already implied by the board's TASK-357 spec):
- **Lane A — multi-client PIE (the state gate):** Editor Multiplayer Options: Number of Players = 2, Net Mode = **Play As Listen Server** (instance 0 = host/Blue). PIE logins flow through the real `InitNewPlayer`/`PostLogin` path; the D2 NetMode belt latches `bNetworkedMatch` regardless of PIE's URL plumbing. Fast iteration for checks (b)–(e), (g)-adjacent.
- **Lane B — twin `-game` processes (the real travel/menu path):** two windowed `UnrealEditor.exe <uproject> L_MainMenu -game` instances on one machine; host via the menu (`?listen` travel), join via `127.0.0.1` (default port), bad-IP via an unroutable address (e.g. `10.255.255.1` — expect timeout → error text → menu). This exercises the ACTUAL `OpenLevel ?listen`/`ClientTravel` flow, the `?listen` half of D2, and gate (a)/(f). LAN loopback = the ruling-2 scope.

**Observable checklist (maps to the gate letters + this doc):**
| Gate | Observable |
|---|---|
| (a) join | Lane B: client reaches `L_Arena`; host log shows the second login; client log shows no travel errors |
| (b) teams | Host PS Team=Blue / client PS Team=Red on BOTH machines (log/`GetPlayerStateForTeam`); client hero spawns Red-side (castle-relative); heroes' `Team` correct on both machines; NO bot spawned (log line); standalone bot still spawns (g) |
| (c) castle HP | Host's units damage a castle ⇒ bar + numeric HP identical both screens; crumble stage visually identical; Masons heal (host) tracks on both |
| (d) gold | Client gold ticks up on the CLIENT's HUD (server-driven, owner-only); host cannot see client gold and vice versa; client gold NEVER drifts from the server value (spot-check logs); overtime doubles the displayed rate on both at 7:00 (sting fires once per machine) |
| (e) flow | Castle falls ⇒ end screen on BOTH screens (client via OnRep) with own-team-relative music; Play Again from the HOST resets both screens (clock 0, gold 10, castles full, scatter re-rolled identically); Play Again from the CLIENT works via `ServerRequestPlayAgain` (if the §9.6 WBP rewire landed; else record the fallback) |
| (f) menu | Host/Join/Back flow per §5; bad IP fails to an error message + menu, never a hang/crash |
| (g) standalone | Full Play-vs-Bot regression: bot spawns, economy, group orders + stances, camera shake, Play Again, sandbox — byte-identical (the §10 argument, spot-verified in play) |
| (h) logs | Message Log + both instances' logs: ensure/AccessedNone/Fatal = 0; net warnings recorded verbatim; `Failed to compile Material` grep |
| extra | Scatter: identical layouts (compare a distinctive landmark near the corridor on both screens; compare logged seeds + attempt counts); client hero death → respawns Red-side after 5 s (per-player tracking); client card play refuses with the §4.1 message; clock identical (±1 s display granularity) on both screens including after Play Again |

TASK-355 runs inside this session after the compile step (board precedent). Any genuinely required net ini config lands here, single-owner, documented (D13 expects none).

## 9. Risks / open questions for the manager's sign-off

1. **`HeroCharacter.{h,cpp}` is missing from TASK-356's board file list** (its "names" block) but P1 cannot meet gate (b) without §2.3/§3.6 (no hero SetTeam exists; audit §1b#4). **Recommend:** amend TASK-356's names to include it (post-350, single-owner — same lane it already waits on). The alternative (a separate micro-task) adds a QA loop for 3 small edits.
2. **The §4.1 observer posture** (client cannot play cards/spells/orders or deal damage in P1). **Recommend:** accept — it is the honest minimal gate; every alternative ships divergence. The refusal-message wording is yours to set.
3. **Scatter defensive-cull residual (D9):** rare client-side over-blocking near the corridor when the server's nav validation culled. **Recommend:** accept for P1 with logged attempt counts; P2 upgrades to replicating the final cull bands if felt in play.
4. **`bNetworkedMatch` detection (D2)** = `?listen` option ∥ NetMode belt — resolves your audit-§9 flag 1. **Recommend:** sign as-is (covers both the real travel path and PIE).
5. **Optional carve-out:** the five CLEAN-lane files (§6) are implementable before TASK-350 lands as a "TASK-356a". **Recommend AGAINST:** one task keeps the byte-identity argument and its QA review whole, and the P1 increment is untestable until the 349-lane files unlock anyway.
6. **WBP_VictoryScreen Play-Again rewire** (one node: button → `RequestPlayAgain` on the owning PC) needs an editor touch — **recommend appending to TASK-355's session** (it is already additive-editing a menu WBP that day). Fallback if refused: host-press-only Play Again still satisfies a minimal reading of gate (e); the client button stays dead until P2 — record which way you rule.
7. **Victory-widget relative text (D14):** C++ presentation goes own-team-relative now; the WBP's internal Victory/Defeat branch keyed on the absolute winner may still read "wrong" on a winning Red client unless the optional `SetLocalVictory` is consumed. **Recommend:** ship P1 with music/flow correct + flag the widget branch to the art seam (not gate-blocking).
8. **P2 flag (recorded, no P1 action):** server-side deck build for REMOTE controllers currently would load the HOST's SaveGame (§4.3#14) — must switch to the curated default deck when card play lands.
9. **`LogSiegeNet` lives in TASK-354's new files** and TASK-356 may reference it — safe because TASK-357 compiles both lanes together; if the manager wants zero cross-task coupling, 356 falls back to `LogTemp` (cosmetic).

## 10. The single-player byte-identity argument (ruling 3 — the load-bearing claim)

Standalone NetMode facts: `HasAuthority()` is true on every actor; `GetNetMode() == NM_Standalone`; there are no connections, so NO property ever replicates and NO OnRep ever fires; `IsLocalController()` is true for the one PC. Therefore, per edit class:
- **Authority guards** (PS mutators/BeginPlay, Castle TakeDamage/Heal/Reset, hero melee, GameState Tick fork, scatter BeginPlay gate, PC entry lockouts): the guard condition is TRUE ⇒ the guarded body runs exactly as today. Provable no-ops.
- **Replication registrations + OnReps** (all §3 tables): dead weight in standalone — registered but never sent, handlers never invoked.
- **`bNetworkedMatch`** is false (no `?listen`, NM_Standalone) ⇒ SpawnBot + sandbox paths byte-identical.
- **Seat latch:** one login ⇒ Blue, same as the old unconditional tag.
- **`FindControllerForTeam` / local-viewer shake / GameState match-end notify:** with exactly one (local, Blue) PC, each resolves the identical controller the old first-controller/index-0/direct-push code did — same observable calls via a compliant route.
- **`RestartPlayer` override / per-player hero map:** one controller ⇒ the map holds one entry; the transform resolve returns the same PlayerStart the engine path used (the respawn path already uses this exact resolve today). The ONE site where "identical route" is not literal — QA scrutinizes it (§8 gate g).
- **`RequestPlayAgain`:** authority branch calls `PlayAgain()` directly — same call the widget made.
- **Client-only additions** (HUD PS-retry first-check-passes, `PerformLocalMatchReset`, OnRep bodies, client scatter mode): unreachable in standalone.

TASK-356's handoff must restate this argument per edited site (the TASK-307/327 pattern), and TASK-356-QA verifies it site by site.

---

## P2 preview (non-binding, for the RPC-table completeness the board spec asks): unit/building/projectile/hero replication + movement smoothing; combat authority; `ServerPlayHandSlot` / `ServerDiscardHandSlot` / `ServerConfirmPlacement` / `ServerConfirmSpellTarget` / `ServerResolveInstant` / `ServerPurchaseUpgrade` / `ServerSetUnitCommand` / `ServerConfirmGroupOrder` / `ServerMeleeAttack` (all Reliable + WithValidation, on the owning PC; cosmetic-only multicasts for VFX); group/stance state migration to server-side controllers; GoldNode/CaptureZone state rep; OnRep cosmetic wiring (shake/flash/damage numbers); remote-deck default-deck fix. P3: deck handoff, lobby polish, disconnect/edge cases (ruling 6).
