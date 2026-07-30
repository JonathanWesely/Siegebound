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

---

## Loop-1 fixes (TASK-357 two-client gate: 3 blockers + finding 4)

Sources: `handoffs/TASK-357-buildmaster.md` + `qa/TASK-356.md` (measured evidence), the board's **"Manager ruling (2026-07-29) — NET RELEVANCY POLICY"**, and CONVENTIONS **NET RELEVANCY LAW**. Policy was ruled; this loop implements it. Every engine claim below was read from the installed UE 5.8 source this pass, not recalled.

### BLOCKERS 1 + 2 — one cause: default 150 m relevancy inside a 500 m arena

**Mechanism (confirmed against engine source + the measurements):** UE's default distance relevancy is ~150 m. The scatter is a POINT actor at the world origin while both players fight ~250 m away ⇒ permanently irrelevant ⇒ `OnRep_GenerationIndex` never fired ⇒ client ran with 0 obstacles / 0 gold nodes (host 6). The FAR castle sat 488 m from the client ⇒ HP/crumble/destroyed never arrived (host 500 / client 2000), while the NEAR castle (12 m) was perfect. **Corroboration that this is relevancy and not code:** GameState and PlayerState replicated correctly across the same 500 m in the same run — and they are the two classes the engine already marks always-relevant (verified below). My OnRep code is untouched by this fix.

**Fix — the ruled tiered policy, implemented exactly:**
- **NEW `Source/GitClaudeUnrealTest/Siegebound/SiegeNetLimits.h`** (one-concept header, the `TeamId.h` precedent; no UCLASS, no module dep): `namespace SiegeNet` with `ArenaRelevancyDistance = 60000.f` (the ruled arena-diagonal × 1.1) and `ArenaRelevancyDistanceSquared = ArenaRelevancyDistance * ArenaRelevancyDistance` — **derived by multiplication, never a typed 3.6e9**, so the square can never drift from the distance. Carries the three-tier definition, the measured defect it closes, and the P2 wave duty (unit fleet = Tier B, bandwidth-measured; levers are update-frequency/dormancy, never shrinking the band).
- **`ACastle`** (Castle.cpp:57) `bAlwaysRelevant = true` — blocker 2.
- **`ASiegeBattlefieldScatter`** (BattlefieldScatter.cpp:155) `bAlwaysRelevant = true` — blocker 1, with the D9 correction written into the code comment.
- **`AHeroCharacter`** (HeroCharacter.cpp:69) `SetNetCullDistanceSquared(SiegeNet::ArenaRelevancyDistanceSquared)` — Tier B from the constant.
- **⚠️ UE 5.5+ API trap avoided:** the raw `NetCullDistanceSquared` field is `UE_DEPRECATED(5.5, "Public access … Use SetNetCullDistanceSquared()")` (Actor.h:898-900). A direct assignment would have been a deprecation break at TASK-357's compile; the setter (Actor.h:4648) is used. `bAlwaysRelevant` remains a plain public non-deprecated UPROPERTY (Actor.h:332-333) — direct ctor assignment is correct there.

**Tier A verify-only (the law's "VERIFY and document, do NOT blind-set" clause) — discharged with evidence, nothing set:**
- `AGameStateBase::AGameStateBase` sets `bReplicates = true` **and** `bAlwaysRelevant = true` — Engine/Private/**GameStateBase.cpp:25-26**.
- `APlayerState::APlayerState` sets `bReplicates = true` **and** `bAlwaysRelevant = true` (+ `SetNetUpdateFrequency(1)`) — Engine/Private/**PlayerState.cpp:25-26**.
Both classes are therefore already Tier A; a redundant assignment in our subclasses would have hidden that fact. **Relevancy ≠ condition:** `Gold`'s `COND_OwnerOnly` is untouched and still restricts the property to its owner.

**D9 CORRECTION (recorded in code — BattlefieldScatter.cpp ctor — and here):** the signed doc's "client obstacles are a SUPERSET of the server's ⇒ never rubber-bands" argument is **VOID unless the seed arrives**. Under default relevancy the client received a strict **SUBSET (zero)** — the exact inversion the design promised. **Tier-A membership is that argument's precondition**; the rest of §3.5's reasoning stands.

### BLOCKER 3 — hero spawn: the team branch now governs

**Mechanism:** `GetHeroStartTransform` ran `FindPlayerStart(Player)` and returned on ANY `APlayerStart` **before consulting `HeroTeam`**. L_Arena has exactly one PlayerStart (Blue side, ≈-23800), so the castle-relative branch was unreachable dead code and both heroes stacked there (Red at -23800, y=84 — the engine nudging a second pawn off an occupied spawn).

**Fix (SiegeGameMode.cpp, `GetHeroStartTransform`):** reordered so the team governs — (1) resolve the hero's **own-team castle** first (it defines that team's side of the centerline); (2) accept a PlayerStart **only when it lies on the same side** (X-sign comparison against that castle) — or when the level has no castle at all (the pre-M8 behavior preserved for defensive/test maps); (3) else the castle-relative offset toward the centerline (now REACHABLE — this is the Red client's spawn); (4) else the arena origin. The side test is **data-driven from castle X sign — not a Blue/Red hardcode** (the M8 team law retires "Blue = local"), so a future Red-side PlayerStart is picked up automatically. Header doc rewritten to match.

**Standalone byte-identity (load-bearing — gate g passed and must keep passing):** the single player is Blue; L_Arena's PlayerStart (≈-23800) and Castle_Blue (-25000) are both X < 0 ⇒ same side ⇒ the PlayerStart is accepted exactly as before, same location, same yaw-only rotation ⇒ the measured `(-23800, 0, 98)` rot `(0,0,0)` reproduces.

### FINDING 4 — verdict: **the manager's hypothesised mechanism is NOT what happened; the real cause is a TOOLING ARTIFACT of the python-invoke trigger.** (The law stands regardless.)

Investigated in engine source; three findings, each checkable:

1. **`bReplicates = False` on the client's own PC is NORMAL ENGINE BEHAVIOR, not a defect and not evidence of anything.** `APlayerController`'s constructor never sets it (verified: the only `bReplicates = true` in PlayerController.cpp is **ANoPawnPlayerController's at :6813**; `AController`'s ctor sets `bOnlyRelevantToOwner` at Controller.cpp:67 and no replication flag). The SERVER enables it per instance at login — `UWorld::SpawnPlayActor` → `SetReplicates(true)` + `SetAutonomousProxy(true)` (**World.cpp:4937-4938**). `bReplicates` is **not itself a replicated property**, so a client's locally-constructed copy keeps the CDO's `false` while the actor channel writes the real roles. That is why every OTHER replicated actor read `true` on that client: their **class constructors** set it (our `ACastle`/`ASiegeBattlefieldScatter`; engine `APawn` Pawn.cpp:86, `APlayerState`, `AGameStateBase`). Nothing was wrong with the PC.
2. **The hypothesised inversion did not occur, and could not have via this path.** `AActor::HasAuthority()` measured **False** on that PC (role `ROLE_AUTONOMOUS_PROXY`) — the guards behaved correctly, which is precisely why `RequestPlayAgain` took the client-relay branch and logged it. Moreover **`AActor::GetFunctionCallspace` never reads `bReplicates`** (read in full, Actor.cpp:5467-5665): for a client calling a `FUNC_NetServer` function it returns **Remote**, or **Absorbed** if `RemoteRole == ROLE_None` — and Absorbed *does not run the body* and *does* log `LogNet Warning: Client is absorbing remote function`. Neither was observed, so the callspace path cannot explain "body ran, no warning, host silent".
3. **What DOES explain all three observations exactly:** `GetFunctionCallspace`'s **very first branch** returns `FunctionCallspace::Local` when the global `GAllowActorScriptExecutionInEditor` is true (**Actor.cpp:5469-5474**, comment: *"Call local, this global is only true when we know it's being called on an editor-placed object"*), and **`FEditorScriptExecutionGuard`'s constructor sets exactly that global** (`GAllowActorScriptExecutionInEditor = true`, **ScriptCore.cpp:451-455**; declared Script.h:554-562). Editor/python remote-exec invocation runs inside that guard — so **any RPC triggered from a python invoke resolves Local**: the body runs on the calling machine, nothing is sent, and no LogNet warning is emitted. Body ran client-side ✓, no engine warning ✓, host never saw it ✓. This also corrects one line in the QA report: the routing decision is *not* caller-agnostic — a python invoke differs from a Blueprint/C++ call by exactly this global, which is why QA rightly flagged the item "not asserted-proven" pending the real button.

**Fixes shipped anyway (belt-and-braces — the law's instruction, and they make the re-run self-diagnosing):**
- **`ASiegePlayerController` ctor: `bReplicates = true`** — the CONVENTIONS COROLLARY's explicit "assert/verify `bReplicates` on any class whose authority branch matters"; this class is dense with authority branches (4 D5 lockouts + the RPC routing). Same pattern APawn and ANoPawnPlayerController use. Safe + inert: the engine's login-time `SetReplicates(true)` now early-outs on the same value/RemoteRole and `SetAutonomousProxy` is unchanged; standalone has no connections, so nothing changes (byte-identity holds).
- **`ServerRequestPlayAgain_Implementation` authority guard:** if the body ever executes without authority it now logs a precise `LogSiegeNet` **Error** naming the callspace-resolved-Local defect and **refuses** — instead of falling through to the misleading "no ASiegeGameMode on the server (mis-config?)" line QA saw. A client can never locally reset a match, and TASK-357's re-run gets an unambiguous signal either way.
- Routing itself (`RequestPlayAgain`: authority → direct `PlayAgain()`; client → the RPC) is unchanged — it is correct per D6/§4.2.

### Tier declarations table (declaration duty — all six replicated classes)

| Class | Tier | Mechanism | Declared at | Set at |
|---|---|---|---|---|
| `ASiegeGameState` | A | engine default — **verified, not set** (GameStateBase.cpp:25-26) | SiegeGameState.h (above `GetLifetimeReplicatedProps`) | — |
| `ASiegePlayerState` | A | engine default — **verified, not set** (PlayerState.cpp:25-26); `Gold` `COND_OwnerOnly` unaffected | SiegePlayerState.h | — |
| `ACastle` | A | `bAlwaysRelevant = true` | Castle.h | Castle.cpp:57 |
| `ASiegeBattlefieldScatter` | A | `bAlwaysRelevant = true` (+ D9 precondition note) | BattlefieldScatter.h | BattlefieldScatter.cpp:155 |
| `AHeroCharacter` | B | `SetNetCullDistanceSquared(SiegeNet::ArenaRelevancyDistanceSquared)` — from the constant, no literal | HeroCharacter.h | HeroCharacter.cpp:69 |
| `ASiegePlayerController` | A | engine-owned, OWNER-SCOPED (`bOnlyRelevantToOwner`, Controller.cpp:67); ctor `bReplicates = true` is the finding-4 hardening, not a tier change | SiegePlayerController.h | SiegePlayerController.cpp:109 |

### Loop-1 files touched
NEW `SiegeNetLimits.h`; `Castle.{h,cpp}`; `BattlefieldScatter.{h,cpp}`; `HeroCharacter.{h,cpp}`; `SiegeGameState.h`; `SiegePlayerState.h`; `SiegeGameMode.{h,cpp}`; `SiegePlayerController.{h,cpp}`; this handoff. **No `DefaultEngine.ini` edit** (D13 holds). No OnRep/replication-registration logic changed — the loop-1 delta is relevancy + spawn ordering + two hardening lines.

### What TASK-357's re-run must observe
1. **Blocker 1 dead:** client log shows `Client regen: replicated seed=<N>` and the client's `GenerateScatter seed=` matches the host's **exactly**; client gold-node count == host's (6, not 0); a corridor landmark matches on both screens.
2. **Blocker 2 dead:** damage the FAR castle (the one ~488 m from the client) — HP + crumble stage identical on both screens; the near castle still correct; destroyed/reset states cross too.
3. **Blocker 3 dead:** Red client hero spawns **castle-relative on the RED side** (≈ +25,000 X band, facing the centerline), NOT at -23800; heroes are not stacked; client hero death → respawns Red-side after 5 s.
4. **Standalone regression (gate g) still exact:** single-player hero spawns at `(-23800, 0, 98)` rot `(0,0,0)` — the same numbers the passing run recorded.
5. **Finding 4 decided by the real button** (owed once TASK-355 lands): press Play Again on the CLIENT's WBP_VictoryScreen. Expected: host log `ServerRequestPlayAgain — client-initiated Play Again accepted`. If instead the CLIENT logs the new `executed WITHOUT authority — the RPC resolved LOCAL` error, the routing defect is real and reproducible outside tooling — file it with that line as the evidence. A python-invoke trigger is NOT a valid test for this item (it forces Local by construction — see the verdict above).
6. **Tier-A bandwidth sanity (cheap):** note net throughput at the gate; two castles + one scatter + the engine's own always-relevant actors should be unmeasurable. The P2 unit fleet is Tier B and must be measured then, per the law.

---

## Loop-2 fix (BLOCKER 5 — Red spawns inside its own castle ⇒ no pawn)

Loop-1 landed: B1/B2 closed (client regenerates a byte-identical battlefield, 6 gold nodes where it had 0; far castle 500/500 with crumble+destroyed crossing), compile green, the UE 5.5 deprecation avoided, standalone FULL PASS. B5 is the one remaining defect, and it is **one distance** — the loop-1 reorder itself is correct and is NOT reverted.

### Measurement (from `qa/TASK-356.md` B5 + `handoffs/TASK-357-buildmaster.md` §7)

| quantity | value |
|---|---|
| `HeroSpawnCastleOffset` (authored) | `(600, 0, 100)` — sized for the **M1** castle's ~810-uu footprint |
| Castle_Red centre | X = 25,000 |
| Castle colliding-bounds X half-extent (measured live) | **1,219** ⇒ span 23,781 … 26,219 |
| Red spawn produced by the 600 offset | X = **24,400** — inside the span (619 uu past the 23,781 near face; the report's table quotes ~819 — either arithmetic lands *inside*) |
| Result | `SpawnActor failed because of collision` ⇒ `SiegePlayerController_1 pawn=None` |
| Empirical clean-spawn reference | the level's own Blue PlayerStart at **1,200** uu out spawns cleanly every time |

**Root cause class:** a hardcoded extent that ROTTED. 600 was correct for the M1 castle and silently wrong the moment the 3× remaster tripled the footprint — exactly the lesson CASTLE-3X already taught. So the fix is not "type a bigger number", it is "stop hardcoding the extent".

### Chosen value + derivation (SiegeGameMode.cpp `GetHeroStartTransform`, castle-relative branch)

The distance is now **DERIVED from the castle's live geometry**, with the authored constant demoted to a floor:

```
GetActorBounds(bOnlyCollidingComponents=true) -> CastleBoxExtent
SpawnDistance = max( HeroSpawnCastleOffset.X , CastleBoxExtent.X + HeroSpawnCastleClearance )
```

- **`HeroSpawnCastleClearance` = 300** (new EditDefaultsOnly UPROPERTY) — the margin past the *measured* half-extent. Against the live castle: **1,219 + 300 = 1,519**.
- **`HeroSpawnCastleOffset.X` 600 → 1,500** — now only the FLOOR (used if bounds are ever degenerate/unresolvable). 1,500 = the validated 1,200 clean-spawn reference with margin, and it matches the recommended ~1,500 band on its own.
- **Resolved Red spawn: X = 25,000 − 1,519 = 23,481 — exactly 300 uu clear of the 23,781 colliding face**, Yaw 180 (branch logic unchanged). Comfortably past the 1,200 empirical floor; comfortably clear of the hero capsule (r≈42) and of the real 22-hull UCX (tighter than the box bound).
- **Anti-rot property (the point of the exercise):** if the castle is ever resized again, `GetActorBounds` follows it and the spawn moves with it — no constant to remember. `bOnlyCollidingComponents=true` deliberately: only what can BLOCK a spawn counts, so the HP-bar widget component (3,150 uu up, NoCollision) cannot inflate it, and the gate blocker (X span ±266 about the castle) sits well inside and changes nothing.
- **Standalone byte-equivalence (load-bearing):** Blue takes the **PlayerStart** branch (step 2), which this change does not touch — single-player still lands `(-23800, 0, 98)` rot `(0,0,0)`. The castle-relative branch is not on the standalone path at all in L_Arena.
- **Compile-trap caught while re-reading my own edit:** `FVector` components are **double** in UE5 and `FMath::Max` is a single-type template — `FMath::Max(HeroSpawnCastleOffset.X, DerivedSpawnDistance)` (double vs float) would have failed template deduction. Both operands are now explicitly `static_cast<float>`.
- A one-line `LogGitClaudeUnrealTest` line prints castle X, measured half-extent, clearance, resolved distance, floor and final location — so TASK-357 can read the derivation instead of inferring it.

### Respawn-path coverage (verified by reading every caller — the scope warning was right)

All four paths resolve through the same `GetHeroStartTransform`, so **all four** get the corrected distance:

| # | Path | Route | Benefits how |
|---|---|---|---|
| 1 | Initial spawn / client login | `RestartPlayer` (:250) → resolver (:275) → `RestartPlayerAtTransform` → `SpawnDefaultPawnAtTransform` | derived distance **+ the new spawn fallback** |
| 2 | 5 s hero respawn | `HandleHeroRespawnTimer` (:577) → `RestoreHeroAtStart` → resolver (:611) → `SetActorLocationAndRotation` | derived distance (a **teleport**, not a spawn — see note) |
| 3 | PlayAgain step 5 | PlayAgain (:923) → `RestoreHeroAtStart` → resolver | same as #2 |
| 4 | Defensive pawn-lost | `RestoreHeroAtStart` (:605) → `RestartPlayer` | folds into #1 |

**Note on #2/#3 (worth QA's attention):** those paths TELEPORT an existing pawn with `bSweep=false, TeleportPhysics` — they would NOT have logged a spawn failure; a bad location silently plants the hero *inside* the castle. They were only masked in the failed run because Red never had a pawn to teleport. The derived clearance is what makes the existing in-code comment ("No sweep: the start point is clear by design") true for Red as well.

### Fallback decision: **YES, implemented** — `ASiegeGameMode::SpawnDefaultPawnAtTransform_Implementation` override

**Why:** the engine's implementation (`AGameModeBase::SpawnDefaultPawnAtTransform_Implementation`, GameModeBase.cpp:1225-1237 — read this pass) spawns with a **bare `FActorSpawnParameters`**, so the pawn class's own `SpawnCollisionHandlingMethod` governs — and BP_HeroCharacter's refuses a colliding spawn. That is precisely how a mis-sized offset became `pawn=None` instead of a nudged hero. A pawnless seat is unplayable and silent-ish; that outcome should not be reachable by any future geometry change.

**Shape (deliberately minimal + byte-identity-preserving):** call `Super` FIRST and return immediately on success — so every succeeding spawn (all standalone spawns; the regression measured zero failures) is a pure pass-through with **no behavior change**. Only on a NULL result retry the SAME transform with `ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn` (verified to exist, EngineTypes.h:4418) mirroring the engine's `SpawnInfo` (Instigator + `RF_Transient`), and log a `LogGitClaudeUnrealTest` **Error** naming `HeroSpawnCastleClearance` as the thing to re-check. Defense in depth, not the fix — with the derivation in place this path should never execute, and if it ever does the error line is the diagnosis.

### Loop-2 files touched
`SiegeGameMode.h` (offset doc + value 600→1500, new `HeroSpawnCastleClearance`, the `SpawnDefaultPawnAtTransform_Implementation` declaration), `SiegeGameMode.cpp` (derived distance + log in the castle-relative branch, the spawn-failure override), this handoff. Nothing else — no relevancy, replication, OnRep, or reorder logic touched.

### What TASK-357's re-run must observe
1. **B5 dead:** client login logs the castle-relative derivation line with `spawn distance 1519` and location ≈ `(23481, 0, 100)`; **`SiegePlayerController_1 pawn=<BP_HeroCharacter_C>`** (not `None`); TWO `BP_HeroCharacter_C` actors in the world; **zero** `SpawnActor failed because of collision` and zero `Couldn't spawn Pawn` lines.
2. **The Red hero is where it should be:** ≈ +23,481 X, Yaw 180, standing OUTSIDE its castle (the loop-1 branch intent, now spawnable) — and it can move/be gate-blocked as the loop-1 gating checks expect.
3. **Respawn paths:** kill the client hero → respawns Red-side after 5 s at the same derived spot (path #2); Play Again → both heroes restored to their own sides (path #3). Neither should place a hero inside geometry.
4. **The fallback stayed asleep:** **no** `was refused for collision — retried with AdjustIfPossibleButAlwaysSpawn` line anywhere. If it DOES appear, the spawn still succeeded (that is the point) but the geometry moved — re-check `HeroSpawnCastleClearance`.
5. **Standalone regression unchanged:** hero at `(-23800, 0, 98)` rot `(0,0,0)`, zero spawn failures — the same numbers as the passing run.
6. Everything closed in loop 1 (B1/B2, seats, gold, clock, match end, D5 lockouts) stays closed — this loop touched none of it.
