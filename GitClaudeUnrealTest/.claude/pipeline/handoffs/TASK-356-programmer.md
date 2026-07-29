# TASK-356 — [M8-rep1] Core-state replication pass 1 — programmer handoff

Status: **ready-for-qa** (file-only; NOT compiled — TASK-357 compiles). Implemented against the SIGNED `handoffs/TASK-353-architecture.md` (D1–D14; the doc wins over board summaries), CONVENTIONS "Networked 1v1 (M8)", and my STEP-0 addendum `handoffs/TASK-353-architecture-addendum.md` (the 349-lane edits comply with its verdicts; QA checks both). Base tree: HEAD `08c4a24`, all target files clean at start.

**STEP 0 delivered:** `handoffs/TASK-353-architecture-addendum.md` — the §7 CASTLE-3X hazards resolved evidence-based (file:line) against the shipped `949c252` code. Verdicts: (1) hero `OnRep_Team` re-stamp hook IS needed → implemented; units have no P1 hazard (no proxies) with the P2 hook named; (2) client-side gate truth HOLDS — compliance fence: the castle's gating config path stays authority-UNgated (obeyed); (3) crumble pins: `CrumbleStage` (Castle.h:399) / `ApplyCrumbleStage` absolute / stage-0 = `ApplyTeamVisuals` → the OnRep branches exactly so.

---

## Per-file diff summary

### CLEAN lane

**`SiegeGameState.{h,cpp}`** — the match-flow replication home (doc §3.3, D7/D8).
- Replicated set + `GetLifetimeReplicatedProps` (cpp:33–45): clock basis triple `ClockBaseSeconds` (`OnRep_ClockBase`) + `ClockBaseServerTime` (plain; explicit float cast from the engine's double `GetServerWorldTimeSeconds`) + `bClockRunning` (promoted from a plain bool to `UPROPERTY(Replicated)`); `bOvertimeActive` → `OnRep_OvertimeActive`; NEW `bMatchEnded` (`OnRep_MatchEnded`) + `WinningTeam` (plain, same-bunch atomic pair).
- `PublishClockBasis()` — written at exactly the ~3 state changes: NEW `BeginPlay` (authority), `StopClock`, `ResetClock` (both now authority-guarded + publish). NEVER a per-frame replicated float.
- `Tick` non-authority fork (cpp:80–104): display-only derivation + the existing whole-second broadcast; never accumulates, never latches overtime, never plays the sting — the audit-§3.2 client clock/overtime fork is dead. `GetMatchClockSeconds()` out-of-line: authority = accumulated, client = derived (late join correct by construction).
- `OnRep_OvertimeActive`: true edge = sting + `OnOvertimeStarted` (once per machine — gate d); false edge silent (ResetClock semantic).
- Match-result API: `SetMatchResult(Winner)` (guard + latch + server-side local notify = the HOST's screen), `ClearMatchResult()` (guard; no notify — PlayAgain step 6 walks server-side controllers), `OnRep_MatchEnded` → `NotifyLocalControllersMatchEnd/Reset()` — `GetPlayerControllerIterator` + `IsLocalController()` only (ban-compliant; at most one local PC per machine), end → `HandleMatchEnd(WinningTeam)`, reset → `PerformLocalMatchReset()`.
- `GetPlayerStateForTeam`: duplicate-team defensive warn (audit §9 flag 2), one-shot latch, first match still returned unchanged.

**`SiegePlayerState.{h,cpp}`** — gold server-authoritative (doc §3.2).
- `Team` → `ReplicatedUsing=OnRep_Team`, **plain** (the doc's deliberate not-InitialOnly divergence, rationale in-code); `OnRep_Team` = P1 log-only seam. `Gold` → `ReplicatedUsing=OnRep_Gold`, **`COND_OwnerOnly`**; `OnRep_Gold` fires `OnGoldChanged` DIRECTLY — documented in-code why never via the guarded `SetGold` choke.
- BeginPlay accrual-fork kill: `ResetGold()` (seed + income timer) wrapped in `HasAuthority()`; the overtime BIND stays on ALL copies (read-only display; the client's 7:00 arrives via the GameState OnRep firing the same delegate); `CachedGoldRate` seed stays on all copies.
- **13 authority guards** (12 public mutators + the private `StartIncomeTimer` belt): `SpendGold` (returns false), `AddGold`, `AddIncome`, `RemoveIncome`, `AddMinerIncome`, `RemoveMinerIncome`, `RegisterMinerAlive`, `UnregisterMinerAlive`, `ResetGold`, `ResetEconomy`, `PauseIncome`, `ResumeIncome` — one shared warn helper on `LogSiegeNet`. `SetGold` itself stays unguarded (private; per the doc — OnRep must not be refused by it).

**`SiegeGameMode.{h,cpp}`** — seats, latches, match flow, per-player heroes.
- `bNetworkedMatch` dual latch (D2): InitGame `?listen` half + sandbox-refusal ordering (`bSandboxMatch` forced off + log); BeginPlay NetMode belt (`|= GetNetMode() != NM_Standalone`) BEFORE the `SpawnBot()` call.
- Seat latch (D3, `InitNewPlayer`): first login Blue / second Red / third+ warned Red; `bBlueSeatTaken`/`bRedSeatTaken` members; per-login seat log on `LogSiegeNet`. Replaces the unconditional Blue tag (audit §1b#2).
- `SpawnBot()` first-line networked gate (doc §2.2) — bot byte-identical otherwise (ruling 3).
- `OnCastleDestroyedHandler`: cancels ALL per-controller respawn timers; the direct `HandleMatchEnd` push loop RETIRED → `SiegeGameState->SetMatchResult(Winner)`; defensive fallback keeps the old direct push when no ASiegeGameState resolves (flagged F3).
- Per-player hero tracking (D10): `TrackedHero`/`HeroRespawnTimerHandle`/`FindLocalSiegeController` DELETED (retirement notes in place); `TMap<TWeakObjectPtr<AController>, FTimerHandle> HeroRespawnTimers`; `HandleHeroDied` resolves the owning controller at death time (weak ptr rides the timer delegate → `HandleHeroRespawnTimer`); `RestoreHeroAtStart(AController*)` parameterized; NEW `RestartPlayer` override → team-keyed `GetHeroStartTransform` → `RestartPlayerAtTransform` (Red spawns castle-relative — no Red PlayerStart is the design); PlayAgain step 5 iterates player controllers (upgrades cleared per hero, restore per controller); step-1 + EndPlay clear the whole map (timer policy intact).
- PlayAgain: `ClearMatchResult()` added right after the step-3b `ResetClock` (client reset rides the false-edge OnRep).

**`BattlefieldScatter.{h,cpp}`** — deterministic shared battlefield (D9, adopted-into-P1).
- ctor `bReplicates = true`; `ChosenSeed` (Replicated) + `GenerationIndex` (`ReplicatedUsing=OnRep_GenerationIndex`) + `GetLifetimeReplicatedProps`.
- BeginPlay authority-gated (client logs + waits); `GenerateScatter()` gains the authority guard, publishes `ChosenSeed`/`++GenerationIndex` after the (unchanged) seed selection, then calls the NEW `RunScatterPasses(Seed, bAuthoritativeGenerate)` — a byte-identical extraction of the old tail (keep-clear rebuild → repro log → two layer passes → `PlaceMines`), with the nav-settle poll + reachability validation running ONLY on the authority path.
- `OnRep_GenerationIndex`: `ClearScatter()` + `RunScatterPasses(ChosenSeed, false)` — client-mode: deterministic passes only, validation/culls authority-only (accepted D9 superset residual, argued in-code); logs the client seed line for the TASK-357 layout comparison. Join-in-progress regen via index ≥ 1 vs CDO 0.

**`SiegeFeedbackLibrary.cpp`** — `PlayLocalCameraShake` index-0 → local-viewer loop (`GetPlayerControllerIterator` + `IsLocalController`, exactly one per machine). The only feedback-layer file touched (doc §6).

### 349 lane (per the addendum)

**`Castle.{h,cpp}`** — the §3.1 table, complete.
- ctor `bReplicates = true`. Replicated: `CurrentHP` (`OnRep_CurrentHP` → the existing `OnCastleHPChanged` broadcast — zero widget changes), `bDestroyed` (`OnRep_Destroyed` — bool naming law), `CrumbleStage` (`OnRep_CrumbleStage`), `Team` (`COND_InitialOnly` belt). `MaxHP` deliberately NOT replicated (CDO/level-authored, per the doc).
- `ApplyDestroyedState(bool)` refactor: the shared visual/collision half (hide/show + collision toggle + HP-bar visibility) extracted from `HandleDestroyed`/`ResetCastle` and reused by `OnRep_Destroyed`; the server-only halves (heal-stop, sting, `OnCastleDestroyed` broadcast — NEVER fired by the OnRep) stay put.
- `OnRep_CrumbleStage`: stage > 0 → `ApplyCrumbleStage(stage)` (addendum-verified absolute); stage == 0 → `ApplyTeamVisuals()` (the addendum-pinned pristine-restore branch).
- Authority guards: `TakeDamage` top-line return 0 (belt), `HealOverTime`, `ResetCastle` (guard + log).
- **Addendum compliance:** `BeginPlay`/`ConfigureTeamGating`/`PostInitializeComponents` left authority-UNgated (client-side gate truth); the gating components untouched.

**`SiegePlayerController.{h,cpp}`** — the PC seam.
- THE ONE P1 RPC: `ServerRequestPlayAgain` (`Server, Reliable, WithValidation`; `_Validate` = true — no payload; `_Implementation` gates on `GameMode->HasMatchEnded()` = the intent validation) + `RequestPlayAgain()` (authority → `PlayAgain()` direct — the exact call the widget made; client → the RPC). TASK-355 rewires the WBP button to `RequestPlayAgain`.
- D5 observer lockouts (4 entries): `PlayHandSlot` + `DiscardHandSlot` → `!HasAuthority()` early-out + `BroadcastRefusal` with the APPROVED text `"Not available yet in online matches"` (function-local-static FText — never module-static-init); `SetUnitCommand` + `BeginGroupPick` → early-out + `LogSiegeNet` log. Locking the entries seals every downstream confirm untouched (doc §4.1).
- `HandleMatchEnd`: state half (mode exits + `bMatchEnded` latch) on ALL copies; `IsLocalController()` guard around the UI half (music/widget/input); music now OWN-TEAM-relative (`Winner == MyTeam`, Blue fallback — D14); `SetWinner` contract unchanged + the OPTIONAL by-name null-safe `SetLocalVictory(bool)` call (sign-off §9.7).
- `HandleMatchReset`: `IsLocalController()` guards on the widget/cursor/input halves; state halves (latch, deck reset, stance reset) on all copies. NEW `PerformLocalMatchReset()` = the OnRep-driven client mirror of PlayAgain step 6 (delegates to `HandleMatchReset`, which already owns the single deck-reset entry point — noted divergence from the doc's literal "+ ResetDeck" wording, flagged F4).
- NEW static `FindControllerForTeam(UWorld*, ETeamId)` — the ruling-4 replacement pattern (doc §3.7).
- HUD PS-retry (doc §3.2): the BeginPlay HUD block moved into `TryInitHUD()` — bounded next-tick retries (cap 120) until `GetPlayerState<ASiegePlayerState>()` resolves; exhaustion logs + creates anyway; `!IsLocalController()` never builds a HUD. Standalone: first check passes ⇒ synchronous in BeginPlay, same order.

**`SummonedUnit.cpp`** — ONLY the two poll sites (now :1172/:1199 pre-edit): both `GetFirstPlayerController` resolves → `ASiegePlayerController::FindControllerForTeam(World, Team)` (the unit's OWN team). The `Team == Blue` stance/eligibility gates STAY (P2, doc §2.4). Local `const UWorld*` → `UWorld* const` (signature fit); nothing else touched — no unit replication, no AI gates.

**`HeroCharacter.{h,cpp}`** — §2.3/§3.6 + the addendum-§1 hook.
- NEW `PossessedBy` override (server): Team from the owning `ASiegePlayerState` (warn + keep-default when unresolvable) + capsule channel RE-STAMP (the server-side BeginPlay stamp ran pre-possession — SpawnDefaultPawnFor begins play before Possess). Single seam: `SetPlayerDefaults` does NOT also write Team.
- `Team` → `ReplicatedUsing=OnRep_Team` + `GetLifetimeReplicatedProps`. `OnRep_Team` = log + capsule re-stamp — the addendum-resolved closure of doc §7.1 (client gate-collision truth robust against any BeginPlay-vs-rep ordering).
- `DoMeleeAttack` combat-authority gate (D4/§3.6): montage + whoosh still play locally; `!HasAuthority()` returns BEFORE the target sweep — a P1 client hero deals NO damage. Rally/War-Banner left unguarded (vacuous client-side — no client units exist in P1; per-site argument below).

**NOT touched (doc §6 fence, verified):** `SiegeBotController`, `Building/Tower/Barracks/DeepMine`, `GoldNode`, `CaptureZone`, `Projectile`, `MinerUnit`, `DeckComponent`, `SpellLibrary`, `SiegeCheatManager`, all widgets/feedback components (beyond the one library function), `SiegeNavAreas.{h,cpp}`, **`Config/DefaultEngine.ini`** (D13 — zero net config), `Build.cs` (no new module deps — `Net/UnrealNetwork.h` is Engine).

## D-decision compliance table

| D | Compliance |
|---|---|
| D1 | GameMode stays server-only (engine); no client path spawns AI; unit sim untouched (units don't replicate in P1) |
| D2 | Dual latch implemented verbatim (InitGame `listen` + BeginPlay NetMode belt); consumers = SpawnBot gate + sandbox refusal only |
| D3 | Seat latch at `InitNewPlayer` (first Blue / second Red / third+ warned Red); PS `Team` plain-replicated + `OnRep_Team` |
| D4 | Hero Team assigned in `PossessedBy` from the owning PS; replicated; melee damage authority-gated (montage/whoosh local) |
| D5 | 4 entry lockouts; approved refusal string character-for-character on the two card entries; stance/group log-only |
| D6 | Exactly ONE new RPC: `ServerRequestPlayAgain` (Reliable, WithValidation, on the PC). No other RPC added |
| D7 | Match end/reset = GameState rep (`bMatchEnded`+`WinningTeam`+OnReps); the GameMode direct push retired (defensive no-GameState fallback only — F3) |
| D8 | Basis-triple clock (3 `PublishClockBasis` writes); client Tick display-only; overtime latch replicated; never a per-frame float |
| D9 | `ChosenSeed`+`GenerationIndex` rep; client deterministic regen; nav validation authority-only; superset residual accepted + logged both sides |
| D10 | Per-player hero map + parameterized restore + `RestartPlayer` override; `FindLocalSiegeController` deleted |
| D11 | Untouched (TASK-354's lane); `LogSiegeNet` consumed cross-lane per sign-off §9.9 |
| D12 | `HeroCharacter.{h,cpp}` edited per the board amendment; the WBP rewire is TASK-355's |
| D13 | Zero `DefaultEngine.ini` edits |
| D14 | Music own-team-relative; `SetWinner(ETeamId)` contract byte-unchanged; optional `SetLocalVictory(bool)` by-name call added |

## Single-player byte-identity (the load-bearing claim — per site)

Standalone facts: `HasAuthority()` true everywhere; `GetNetMode()==NM_Standalone`; no connections ⇒ nothing replicates, no OnRep ever fires; `IsLocalController()` true for the one PC.
1. **Every authority guard** (PS ×13, Castle ×3, GameState ×2, scatter generate, hero melee, PC lockouts ×4): condition passes ⇒ the guarded body runs exactly as before. Provable no-ops.
2. **Replication registrations/OnReps** (all files): registered-but-never-sent dead weight; handlers unreachable.
3. **`bNetworkedMatch`**: no `?listen` + NM_Standalone ⇒ false ⇒ SpawnBot + sandbox byte-identical.
4. **Seat latch**: one login ⇒ Blue — the same tag the old unconditional line wrote.
5. **Match end**: `SetMatchResult` → server-side notify → the SAME single local PC `HandleMatchEnd(Winner)` the old direct loop called (same observable call, compliant route). Music: MyTeam=Blue ⇒ the identical `Winner==Blue` branch.
6. **PlayAgain**: `ClearMatchResult` = a state write nothing reads; step-5 loop visits the one controller/hero the old TrackedHero path restored (divergence: upgrades now cleared via the controller's pawn instead of the stale-tracked pointer — defensive-path-only delta, F5).
7. **`RestartPlayer` override**: PlayerStart resolve returns the same location; rotation flattened to yaw (L_Arena's start has zero pitch/roll ⇒ identical transform). THE one non-literal route — QA-scrutinize (doc §10 pre-flags it).
8. **Hero PossessedBy**: PS is Blue ⇒ Team stays Blue; the capsule re-stamp re-applies the identical channel (idempotent set).
9. **`FindControllerForTeam` / local-viewer shake / GameState notify**: with exactly one (local, Blue) PC each resolves the controller the old first/index-0/direct code did. Unit site 2's Red-team theoretical resolve: the old code ALSO reached a Blue-only gate first — and site 1 only ever runs on Blue units (eligibility) — identical outcomes.
10. **HUD PS-retry**: PS exists at first check ⇒ HUD created synchronously inside BeginPlay at the same point in the same order.
11. **Scatter**: authority ⇒ `GenerateScatter` runs the identical sequence (the two int writes added); `RunScatterPasses(…, true)` is a byte-identical extraction (same statements, same order, same logs).
12. **Client-only additions** (`PerformLocalMatchReset`, OnRep bodies, client scatter mode, TryInitHUD retry path, `ServerRequestPlayAgain`): unreachable in standalone; `RequestPlayAgain` authority branch = the direct `PlayAgain()` call the widget made.

## Flagged decisions (enumerated for QA)

- **F1 — `OnRep_ClockBase` handler name** vs the strict `OnRep_<Property>` law (`ClockBaseSeconds`): follows the SIGNED doc's own D8/§3.3 table verbatim (the doc wins on conflict per the board amendment). Rename to `OnRep_ClockBaseSeconds` on QA's word — one-line.
- **F2 — Hero `OnRep_Team` is re-stamp, not log-only**: the addendum-§1 resolution of the doc's reserved §7.1 seam (the doc itself reserved it "for exactly this"). Idempotent, cosmetic-free, closes a live gate-truth hazard.
- **F3 — No-GameState fallback in `OnCastleDestroyedHandler`**: the doc retires the direct push outright; I kept it as the defensive branch of a null GameState (mis-config) so standalone can never lose its end screen. Null-safety law vs design purity — QA's call; deleting the branch is three lines.
- **F4 — `PerformLocalMatchReset` = `HandleMatchReset()` alone**: the doc words it "HandleMatchReset() + ResetDeck()", but HandleMatchReset ALREADY owns the single controller-side ResetDeck call (TASK-023/030 closure) — a second call would re-open the qa/TASK-024 double-reset WARN. Implemented the intent, not the letter.
- **F5 — PlayAgain step-5 upgrade clearing** now reads each controller's pawn (was: the possibly-stale `TrackedHero` pointer). Identical on every shipped flow; differs only on the defensive pawn-lost path (where the old code could reset upgrades on an unpossessed corpse).
- **F6 — `ClockBaseServerTime` stored as float** (explicit cast from the engine's double): delta math horizon is match-length; precision loss immaterial. Documented in-code.
- **F7 — Observer refusal FText** as a function-local static accessor (not a namespace-scope const): FText must not construct at module static-init. Wording character-for-character as approved.
- **F8 — `TryInitHUD` cap = 120 next-tick retries** (~2 s): an implementation constant (the doc says "bounded", no number); exhaustion degrades to HUD-with-null-safe-binds, logged.
- **F9 — Scatter client-mode `PlaceMines`** spawns client-LOCAL AGoldNode actors at deterministic positions (per the doc §3.5); their STATE (reserve/glow) is static until P2's GoldNode rep — the doc's recorded P1 gap, restated here so TASK-357 doesn't file it.
- **F10 — GameState duplicate-team warn is once-per-instance latched** (`mutable` bool; the audit flag said "defensive log" without cadence) — spam-safe on a 1 Hz-polled resolve.

## What TASK-357's two-client verify must observe (per class)

1. **GameMode/seats (b/g):** host log `seated as Blue`, client login `seated as Red`; `SpawnBot skipped — networked 1v1` in the host log; STANDALONE run still spawns the bot + sandbox still refuses networked (`?Sandbox` forced off when combined).
2. **PlayerState (d):** client HUD gold ticks from server writes only (watch for `refused on a non-authority` warns — there should be ZERO in a clean session; any occurrence is a leak through the D5 lockouts worth filing); gold never visible cross-machine (owner-only); rate text +N/s doubles at 7:00 on both machines.
3. **GameState (c/e + clock):** clock identical ±1 s on both screens including right after join and after Play Again; overtime sting exactly once per machine; end screen on BOTH screens with own-team-relative music (Red client winning must hear Victory music); Play Again from host resets both; from the CLIENT via the rewired button → `ServerRequestPlayAgain … accepted` in the host log (or record the §9.6 fallback).
4. **Castle (c):** damage on the host ⇒ bar + numeric HP identical both screens (OnRep path); crumble stage visually identical; Masons heal tracks on both; on Play Again the CLIENT's castles un-crumble to pristine (the stage-0 → ApplyTeamVisuals branch — new code, watch it specifically); the destroyed castle hides + drops collision on the client (blocker included), restores on reset.
5. **Hero (b + gating):** client hero spawns RED-side (castle-relative fallback — new `RestartPlayer` path); heroes' Team correct on both machines (`Team replicated: …` logs); the CLIENT hero is blocked at the enemy gate and passes its own with NO rubber-band (the OnRep re-stamp evidence); client swing plays montage but deals no damage; client hero death → respawns Red-side after 5 s (per-player map).
6. **Scatter:** identical layouts (grep the `GenerateScatter seed=` + `MinesPass` lines on BOTH instances — same numbers; compare a corridor landmark visually); client log shows `Client regen: replicated seed=…`; Play Again re-rolls identically on both.
7. **PC (D5):** client card/discard press ⇒ the exact refusal text on its HUD; T/E/R/F on the client ⇒ log-only refusals; NO group circles client-side.
8. **Standalone regression (g):** full Play-vs-Bot pass — bot, economy, group orders + stances, camera shake, Play Again, sandbox — plus specifically: hero spawns at the PlayerStart with the same facing (the F7/§10 RestartPlayer route), and the end screen/music unchanged.
9. **Logs (h):** ensure/AccessedNone/Fatal = 0; zero non-authority-mutation warns; the `LogSiegeNet` lines above recorded as evidence.
