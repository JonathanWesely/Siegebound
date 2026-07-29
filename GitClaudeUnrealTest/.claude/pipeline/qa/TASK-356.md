# QA Report — TASK-356 (M8-rep1: core-state replication pass 1)
Verdict: **PASS** — 0 BLOCKER / 0 WARN / 3 NIT (all notes; no code change owed before TASK-357)

Scope: pre-compile review per the AMENDED TASK-356-QA board mandate (addendum-first), against the SIGNED `handoffs/TASK-353-architecture.md` (doc wins on conflict), the STEP-0 addendum, `handoffs/TASK-352-audit.md`, and CONVENTIONS "Networked 1v1 (M8)". All ten files read post-edit; engine-behavior claims cross-checked where load-bearing.

---

## STEP 0 — the addendum (the amended mandate's opening check): VERIFIED
- `handoffs/TASK-353-architecture-addendum.md` EXISTS, is authored as STEP 0, and is **evidence-based**: every claim is file:line against the shipped `949c252` tree (stamping sites, blocker config, crumble members), with engine claims cited to installed 5.8 source (Pawn.cpp bReplicates, spawn/rep ordering). It resolves all three §7 items + residuals and pins the §3.1 member names (`CrumbleStage` / `ApplyCrumbleStage` absolute / stage-0 = `ApplyTeamVisuals`).
- **356's 349-lane edits COMPLY with its fences — verified in code:**
  - The castle gating-config path is authority-UNgated: `HasAuthority()` appears in Castle.cpp at exactly THREE sites — `TakeDamage` (:302), `ResetCastle` (:568), `HealOverTime` (:665). `BeginPlay`/`ConfigureTeamGating`/`PostInitializeComponents` carry no gate (grep-proven) — client-side gate truth preserved.
  - The hero `OnRep_Team` is implemented as RESOLVED (re-stamp, not log-only): HeroCharacter.cpp:183–198 re-stamps the capsule channel from the replicated truth; `PossessedBy` (:149–181) assigns Team from the owning PS and re-stamps server-side (the SpawnDefaultPawnFor-begins-play-before-Possess quirk the addendum names). BeginPlay stamp retained (:95–98) — whichever of {proxy BeginPlay, Team rep} lands second, the channel ends correct.
  - `OnRep_CrumbleStage` branches exactly as pinned: `> 0 → ApplyCrumbleStage(CrumbleStage)`, `== 0 → ApplyTeamVisuals()` (Castle.cpp:461–474).

## Mandate 2 — THE LOAD-BEARING CLAIM: standalone/practice-mode byte-equivalence HOLDS, per site
Standalone facts (doc §10): authority true everywhere, NM_Standalone, no connections ⇒ no rep/no OnReps, exactly one local PC. Traced per class:
- **GameMode:** `bNetworkedMatch` = no `?listen` + NM_Standalone ⇒ false (both latch halves verified, InitGame :84 + BeginPlay :131 — the belt ORs in BEFORE SpawnBot :138). Seat latch: one login ⇒ Blue (:211–239), the same tag the old unconditional line wrote. SpawnBot gate (:935) false ⇒ bot spawns as today; sandbox refusal only fires when both latches true. **Push-loop retirement:** `SetMatchResult` → server-side `NotifyLocalControllersMatchEnd` resolves the SAME single local PC and makes the SAME `HandleMatchEnd(Winner)` call the retired direct loop made — identical observable, compliant route. **F3 fallback** (no ASiegeGameState): unreachable in any configured world (GameStateClass = ASiegeGameState in the ctor), logged, and its non-local HandleMatchEnd calls are defused by the PC's IsLocalController UI-guard — ruled ACCEPT under the null-safety house law (NIT-3 records the doc delta). **D10 hero map:** standalone = one controller → FindOrAdd single entry, SetTimer-replaces (same no-stack semantics as the old single handle); match-end cancel iterates the one-entry map; PlayAgain step 5 iterates the one PC → `ResetUpgrades` + `RestoreHeroAtStart` — the identical restore the TrackedHero path made (F5's delta is defensive-path-only). `RestartPlayer` override: Blue resolves the pre-existing team-keyed `GetHeroStartTransform` → PlayerStart location + yaw — the ONE non-literal route; L_Arena's start has zero pitch/roll ⇒ identical transform; **assigned to TASK-357 gate (g) as the empirical check (hero spawn facing)**. EndPlay clears the whole map (:156–160).
- **PlayerState accrual-fork kill:** `BeginPlay` wraps ONLY `ResetGold()` in `HasAuthority()` (:66–71) — standalone/server: true ⇒ seed + income timer exactly as before; **the server copies of a remote client's PS also pass the guard and accrue server-side — the intended authoritative drive**. The overtime BIND and `CachedGoldRate` seed stay on all copies (read-only display). All **13 guards verified present** (12 public mutators + private `StartIncomeTimer` belt); `SetGold` correctly unguarded (private choke; OnRep must not be refused by it). `OnRep_Gold` broadcasts `OnGoldChanged` DIRECTLY — the doc's subtlest trap (routing through the guarded choke would refuse or double-write on clients) correctly dodged.
- **GameState:** the non-authority Tick fork (:88–100) is unreachable in standalone; the authority path is the pre-356 body byte-for-byte (accumulate → whole-second broadcast → overtime latch + sting). `PublishClockBasis` at exactly the 3 state changes (BeginPlay/StopClock/ResetClock), two float writes nothing reads standalone. `StopClock`/`ResetClock` guards pass on authority.
- **PC:** all four D5 lockouts are `!HasAuthority()` early-outs ⇒ unreached in standalone; `RequestPlayAgain` authority branch = the direct `GameMode->PlayAgain()` call the widget made; `PerformLocalMatchReset` (F4) delegates to `HandleMatchReset()` alone — **verified**: HandleMatchReset owns the single controller-side `ResetDeck()` (the TASK-023/030 closure); a literal doc reading ("+ ResetDeck") would have re-opened qa/TASK-024-report.md WARN-1. Standalone never calls it (OnRep-only). `TryInitHUD`: PS resolves on the first check ⇒ HUD created synchronously at the same BeginPlay point, same order (deck first, widgets after); the `IsLocalController` guards in HandleMatchEnd/Reset are no-ops for the one local PC (state halves — latch, mode exits, deck/stance resets — run on all copies exactly as before).
- **SummonedUnit (the two resolves):** site 1 (:1177–1180) — `CommandGroupId` is only ever assigned to Blue units (controller-side eligibility), so the team resolve returns exactly the one Blue PC the old first-controller call returned; a null resolve hits the same self-heal path. Site 2 (:1208–1214) — upstream `Team == Blue` gate STAYS (P2 scope per doc §2.4), same single-PC equivalence. Group orders/stances work standalone unchanged.
- **Scatter:** authority ⇒ `GenerateScatter` runs the identical sequence + two int writes nothing reads; `RunScatterPasses(..., true)` is the extracted old tail with the nav-settle poll + validation on the authority path only (:348 client return verified). Client mode unreachable standalone.
- **FeedbackLibrary:** the local-viewer loop resolves the one local PC = old controller-index-0 — identical shake; `ClientStartCameraShake` is the same pre-existing engine call (no new RPC surface).
- **Hero:** `PossessedBy` in standalone reads the Blue PS ⇒ Team stays Blue; the re-stamp re-applies the identical channel (idempotent). The melee gate (:338) passes on authority ⇒ the sweep is byte-identical; montage/whoosh order unchanged. Rally/War Banner unguarded — correct: client-side they iterate friendlies and find none (no client units in P1), vacuous no-ops.

**Byte-equivalence verdict: HOLDS.** No unguarded behavior change in standalone was found at any site. The single non-literal route (RestartPlayer transform resolve) is pre-flagged by the doc and pinned to TASK-357's standalone regression.

## Mandate 3 — replication hygiene: COMPLETE
- **Registration completeness (both directions):** every `Replicated`/`ReplicatedUsing` UPROPERTY in the module is registered, and every DOREPLIFETIME has its UPROPERTY — Castle ×4 (CurrentHP/bDestroyed/CrumbleStage/Team-InitialOnly), PS ×2 (Team plain / Gold OwnerOnly), GameState ×6 (basis triple + overtime + ended/winner), Hero ×1 (Team), Scatter ×2 (seed pair). No stragglers (grep-proven).
- **Gold COND_OwnerOnly — CORRECT:** the HUD binds only the LOCAL PS's `OnGoldChanged` (own gold only, per the doc + gate d "host cannot see client gold"); the only cross-player gold consumer (Pickpocket) resolves server-side through the victim's authoritative PS. No UI reads enemy gold.
- **OnRep naming:** `OnRep_CurrentHP`/`OnRep_Gold`/`OnRep_Team`×2/`OnRep_OvertimeActive`/`OnRep_MatchEnded`/`OnRep_GenerationIndex` exact; `OnRep_Destroyed` per the ratified bool law; `OnRep_ClockBase` = F1 (NIT-1, ruled below). All handlers carry `UFUNCTION()` (UHT requirement for ReplicatedUsing — verified on all nine).
- **Castle `Team` COND_InitialOnly — RIGHT, reasoned:** level-placed actors carry the level-authored value on BOTH machines before any replication; no assign-after-first-rep window exists (Team set at load, never at runtime), so the D3 snapshot hazard that forced PS.Team plain does not apply — InitialOnly is a zero-cost belt over an already-identical value. Conversely PS.Team plain is right for exactly the bot's spawn-then-SetTeam shape. Both choices match their hazards.
- **Hero `Team` plain — right** (both machines need the ENEMY hero's team for gate truth; assigned once, OnRep self-corrects any edge).
- **`bReplicates`** set exactly where designed (Castle ctor :47, Scatter ctor); NOT added to units/buildings/GoldNode/CaptureZone/Projectile (doc §6 exclusions verified untouched — the P1 scope fence held; no unit/combat/spell replication crept in).

## Mandate 4 — the ONE-RPC law: HOLDS
Module-wide grep: exactly ONE `UFUNCTION(Server, Reliable, WithValidation)` — `ServerRequestPlayAgain` on the PC (h:368); `_Validate` (return true — no payload) + `_Implementation` (GameMode resolve + `HasMatchEnded()` intent gate) both present with correct WithValidation signatures. Zero Client/Multicast RPCs added anywhere. The D5 lockouts use the approved string **character-for-character** (`"Not available yet in online matches"`, :57) on the two card entries; stance/group entries log-only on `LogSiegeNet`.

## Mandate 5 — the GetFirstPlayerController ban: RESOLVED
Module grep: **zero remaining call sites in gameplay code.** The audit's five: SummonedUnit :1145+:1118 → `FindControllerForTeam` (both verified, doc-specced treatment); SiegeFeedbackLibrary :156 → local-viewer loop; GameMode `FindLocalSiegeController` → deleted (retirement note :921); `TrackedHero`/single timer → deleted (per-player map). Remaining textual hits are comments/docs plus TASK-354's session files (its own QA's lane; the session subsystem's local-PC travel resolve is the doc §5-noted session-plumbing exception). `FindControllerForTeam` itself is null-safe and ban-compliant (team match on PS, never "first = the player").

## Mandate 6 — Scatter D9: CORRECT, ordering sound
Authority-gated BeginPlay generate (:175) + `GenerateScatter` guard (:202); client regen ONLY via `OnRep_GenerationIndex` → `ClearScatter()` + `RunScatterPasses(ChosenSeed, false)`; the nav-settle poll + reachability/widen-band/mine-radius culls run on the authority path only (:348 client return). The superset-never-rubber-bands argument is preserved in code comments AND structure (client never culls ⇒ client obstacles ⊇ server obstacles ⇒ CMC corrections structurally impossible). **Seed-before-regen ordering:** `ChosenSeed` + `GenerationIndex` are written in the same server frame on the same actor — they ride one property bunch, and the engine applies a bunch's property values before RepNotifies fire, so the OnRep always reads the fresh seed; join-in-progress fires off the initial bunch (index ≥ 1 vs CDO 0). Client determinism inputs verified identical (level-placed castle transforms + the same config asset + FRandomStream(Seed)).

## Mandate 7 — F1–F10 rulings
- **F1 — ACCEPT (NIT-1).** `OnRep_ClockBase` vs the strict `OnRep_<Property>` law: the SIGNED doc's own §3.3/D8 table names the handler `OnRep_ClockBase`, and the board amendment says the doc wins. Recorded as the naming law's second ratified exception (after the bool-prefix drop). The one-line rename to `OnRep_ClockBaseSeconds` is the manager's optional call — not owed.
- **F2 — ACCEPT.** The re-stamp OnRep is the addendum's evidence-based resolution of the doc's own reserved §7.1 seam; verified idempotent, cosmetic-free, and paired with the PossessedBy server-side re-stamp.
- **F3 — ACCEPT (NIT-3 records the delta).** The no-GameState fallback keeps the retired direct push as an unreachable-in-configured-worlds defensive branch: null-safety house law over design purity; logged; its non-local HandleMatchEnd calls are UI-half-guarded. Deleting it buys nothing but risk.
- **F4 — ACCEPT.** Verified: `HandleMatchReset` owns the single controller-side `ResetDeck()`; the doc's literal "+ ResetDeck" wording would re-open qa/TASK-024-report.md WARN-1 (double reset). Intent over letter — correctly.
- **F5 — ACCEPT.** Upgrade clearing via each controller's pawn = identical on every shipped flow; differs only where the old code was wrong anyway (stale-pointer resets on an unpossessed corpse).
- **F6 — ACCEPT.** Float storage of the server-time basis: clients only compute DELTAS over match horizons; even at ~10⁶ s uptime float precision (~0.06 s) is far inside the 1-s display granularity. Documented in-code.
- **F7 — ACCEPT.** Function-local static FText accessor (:55–59) — the correct UE pattern (FText must not construct at module static-init); wording character-for-character as approved.
- **F8 — ACCEPT.** 120 next-tick retries ≈ 2 s: bounded per the doc, logged exhaustion, degrades to the null-safe-bind HUD; standalone first-check-passes.
- **F9 — ACCEPT.** Client-local deterministic AGoldNode spawns are the doc's §3.5 letter; mines are NoCollision so no collision divergence; static reserve/glow is the recorded P1 gap — TASK-357 briefed not to file it.
- **F10 — ACCEPT.** Once-per-instance latch; `mutable bool` verified (GameState.h:237 — compiles under the const method).

**Compile traps:** no literal `*/` in doc comments (pattern-swept all touched files; hits are legitimate inline `/*param=*/`); every new Printf has matched specifiers/args (read at each site); no shadowing (the GameState loop var rename note preserved; new locals collide with no members); `Net/UnrealNetwork.h` present in all five reps' cpps; `SiegeSessionSubsystem.h` (LogSiegeNet) included in all six consumers — the cross-lane coupling is the sign-off §9.9 accepted shape (both lanes compile together at TASK-357); `_Validate`/`_Implementation` signatures correct; OnRep handlers all `UFUNCTION()`; by-name widget calls use the existing ParmsSize-checked ProcessEvent pattern (ETeamId and bool both 1 byte — matches the SetWinner precedent).

## NITs (notes, no loop owed)
1. **F1 handler name** — recorded exception; optional rename is the manager's call at any future touch.
2. **`ASiegePlayerState::SetTeam` (h:116) is public + BlueprintCallable + unguarded while writing replicated identity.** All existing callers are server-only (InitNewPlayer, SpawnBot) and the signed doc's guard table deliberately scopes to the economy surface — compliant with the doc. Latent-only hazard (a future client-side BP call would silently fork the local proxy). **P2 flag:** guard it or annotate server-only when the team surface is next touched.
3. **F3's retained fallback** is a deliberate doc deviation — on the record here so the doc/code delta is never rediscovered as drift.

## What TASK-357's two-client run must observe (per class — endorsing the handoff's list, with QA emphases)
1. **Seats/GameMode (b/g):** host `seated as Blue` / client `seated as Red`; `SpawnBot skipped — networked 1v1`; STANDALONE still spawns the bot; `?Sandbox` forced off when combined with `?listen`.
2. **PlayerState (d):** client HUD gold ticks from server writes only; **ZERO `refused on a non-authority` warns in a clean session — any occurrence is a D5-lockout leak, file it**; gold never visible cross-machine; +N/s doubles at 7:00 on both.
3. **GameState (c/e/clock):** clock identical ±1 s on both screens incl. post-join and post-Play-Again; overtime sting exactly once per machine; end screen on BOTH with own-team-relative music (a winning Red client must hear Victory); client Play-Again via `ServerRequestPlayAgain … accepted` in the host log (or record the §9.6 fallback ruling).
4. **Castle (c):** HP bar + numeric identical both screens; crumble stage identical; **the client-side stage-0 un-crumble on Play-Again (OnRep→ApplyTeamVisuals — new code, watch it specifically)**; destroyed castle hides + drops collision on the client (gate blocker included) and restores on reset.
5. **Hero (b/gating):** client hero spawns Red-side castle-relative (`RestartPlayer` override); `Team replicated` logs on both machines; **the CLIENT hero blocked at the enemy gate / passing its own with NO rubber-band (the OnRep re-stamp's live evidence)**; client swing = montage, zero damage; client death → Red-side respawn after 5 s.
6. **Scatter:** identical layouts (compare `GenerateScatter seed=` + mines lines + a corridor landmark on both instances); `Client regen: replicated seed=` in the client log; Play-Again re-rolls identically on both. Don't file the D9 superset residual or static client mine state (F9).
7. **PC (D5):** client card/discard press shows the exact refusal text; T/E/R/F log-only refusals; no group circles client-side.
8. **Standalone regression (g):** full Play-vs-Bot pass — bot, economy, group orders + stances, shake, Play Again, sandbox — plus **hero spawns at the PlayerStart with the same facing** (the RestartPlayer yaw-flatten, the one non-literal route).
9. **Logs (h):** ensure/AccessedNone/Fatal = 0; net warnings verbatim; `Failed to compile Material` grep.

**Status: qa-passed** → TASK-357 unblocks on the 356 side (still gated on TASK-354-QA per the board).

---

# TASK-357 INTEGRATION RESULT — **qa-failed** (build-master, 2026-07-29) — QA loop 1 of 3

**Compile: GREEN** (joint 356 + 354, `Result: Succeeded`, clean DLL link — the pre-compile review's compile-trap sweep held; zero compile defects).
**Two-client P1 gate: FAILED — 3 defects + 1 flagged, ALL in TASK-356's lane.** No code committed.

Method (repeatable, no menu needed — TASK-355 is blocked): twin `-game` processes, host `…uproject "/Game/Maps/L_Arena?listen" -game -log -LOG=M8_host.log`, client `…uproject 127.0.0.1:7777 -game -log -LOG=M8_client.log` — the command-line equivalents of `HostListenMatch()`/`JoinMatch()`. Both processes driven over UE python remote execution (per-process multicast port via `-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRemoteExecution=True,…:RemoteExecutionMulticastGroupEndpoint=239.0.0.1:<port>`), because the desktop was LOCKED all session (blocks SendInput ⇒ no console/keyboard lane). Full recipe: `handoffs/TASK-357-buildmaster.md`.

## BLOCKER 1 — `ASiegeBattlefieldScatter` never replicates: the client has NO battlefield (D9 defect, the audit's worst hazard, live)

- Client log: `Client copy — waiting for the replicated seed (OnRep_GenerationIndex; M8 doc D9)` — and **`OnRep_GenerationIndex` NEVER fires**. Zero `Client regen: replicated seed=` lines in a full match.
- Consequence measured: host had **6 `AGoldNode`s + 7 scatter layers** (`GenerateScatter seed=512834849`), client had **0 gold nodes** and 14 fewer actors. The two machines play on different battlefields — and the client's obstacle set is a **SUBSET** (empty), i.e. the rubber-band-producing direction, not the "superset residual" the pre-compile review accepted under Mandate 6.
- **Root cause (measured, not inferred):** the actor is a point actor at the world origin with `bAlwaysRelevant = False` and default `NetCullDistanceSquared = 225,000,000` ⇒ **150 m relevancy radius**. In the M7.6 10× arena the castles/PlayerStart sit at **250 m** from the origin (editor readback: scatter (0,0,0); Castle_Blue (-25000,0,0); Castle_Red (25000,0,0); PlayerStart (-23800,0,100)). The client's viewer is *permanently outside the actor's relevancy bubble*, so the channel never opens and the seed pair never sends. The seed-before-regen bunch-ordering reasoning in Mandate 6 is correct but never gets exercised.
- The registration itself is fine — `DOREPLIFETIME(ChosenSeed/GenerationIndex)` verified present; this is a relevancy defect, not a rep-hygiene defect.

## BLOCKER 2 — the ENEMY castle's HP/crumble/destroyed never reach the client (same relevancy cause; §3.1 table effectively dead at range)

Controlled experiment (host applied 1500 world damage to BOTH castles, then read HP on both machines):

| Castle | dist. from client viewer | host HP | client HP |
|---|---|---|---|
| `Castle_Blue` | 1,200 uu (12 m) | 500 | **500 ✓ replicated** |
| `Castle_Red` | 48,800 uu (488 m) | 500 | **2000 ✗ frozen** |

**The `OnRep_CurrentHP` code is PROVEN CORRECT by the near castle** — the far castle simply never opens a channel (150 m cull vs a 500 m arena). At match end the client still read `Castle_Red` intact/undestroyed while showing its defeat screen. In a correctly-seated match this is worse: the enemy castle *is* the win condition, so the HP bar the player watches would be the frozen one. Gate (c) fails.

**Common fix shape for BLOCKER 1+2** (programmer's call — I do not fix code): `bAlwaysRelevant = true` on `ACastle` + `ASiegeBattlefieldScatter` (both are single, low-frequency, match-critical actors), or a `NetCullDistanceSquared` sized to the arena. Re-audit every M8-replicated actor against the 10× arena — the P1 relevancy assumption is the systemic issue, not any one OnRep.

## BLOCKER 3 — the Red (client) hero spawns at the BLUE PlayerStart (D10 / `RestartPlayer` defect)

- Measured both machines: Blue hero `(-23800, 0, 98)`, **Red hero `(-23800, 84, 98)`** — the 84 uu offset is the engine nudging a second pawn off an occupied spawn. Both heroes spawn stacked on the Blue side; the client never appears at its own castle.
- **Root cause in code:** `SiegeGameMode.cpp::GetHeroStartTransform` runs step 1 (`FindPlayerStart(Player)` → accept any `APlayerStart`) **before consulting `HeroTeam`**. L_Arena has exactly one PlayerStart (Blue side), so step 1 always wins and the castle-relative step 2 — the branch `RestartPlayer`'s own comment calls "the Red client resolves the castle-relative fallback… the fallback IS the design" — is **unreachable**. `HeroTeam` is only read in dead code.
- This is exactly the route the pre-compile review flagged as "THE one non-literal route — QA-scrutinize" and deferred to the standalone check. Standalone (Blue-only) is genuinely fine (see below); the defect is networked-only, which is why paper review could not catch it.

## FLAGGED 4 — client-initiated `ServerRequestPlayAgain` executed LOCALLY, never reached the host (D6 / the ONE RPC)

- Client log: `RequestPlayAgain — client relay via ServerRequestPlayAgain (M8 doc §4.2)` immediately followed, **on the client**, by `Warning: ServerRequestPlayAgain — no ASiegeGameMode on the server (mis-config?)` — that warning lives in the RPC `_Implementation`, so the body ran client-side. **Host log contains no `ServerRequestPlayAgain` line at all.**
- Ruled out as a test artifact: the RPC routing decision is made inside `ProcessEvent`/`GetFunctionCallspace` when the generated thunk is called, which is caller-agnostic (Blueprint, C++, or python invoke the *outer* `RequestPlayAgain` identically) — the rewired widget button would take the same path.
- Ruled out as a dead channel: **client→server RPCs demonstrably work this session** — teleporting the client's autonomous-proxy hero locally was corrected by the server (y=1500 → y=0), i.e. `ServerMove`/`ClientAdjustPosition` round-tripped.
- Measured anomaly, specific to the controller: on the client, `SiegePlayerController_0` reads `role=ROLE_AUTONOMOUS_PROXY`, `HasAuthority()=False`, `IsLocalController()=True` but **`bReplicates = False`** — while *every other* replicated actor on that client reads `replicates=True` (Castle ×2, Scatter, GameState, both PlayerStates, both Heroes). No project code sets it (module-wide grep: `bReplicates` is written only in `Castle.cpp:47` and `BattlefieldScatter.cpp:139`). An unreplicated actor makes `GetFunctionCallspace` resolve a `FUNC_NetServer` call to Local silently (no engine warning was logged, consistent with this path).
- **Flagged, not asserted-proven:** the trigger was a direct `RequestPlayAgain()` invoke rather than the (blocked) widget button. Programmer should investigate why the client's own PC is unreplicated; one confirmation through the real button is owed once TASK-355 lands. Play Again is host-only in the interim (already the accepted §9.6 fallback).

## What PASSED (do not re-litigate these; they are verified live)

| # | Must-observe item | Result |
|---|---|---|
| 1 | D2 latch / SpawnBot gate | ✓ host `Networked match latched from the ?listen travel option`; `SpawnBot skipped — networked 1v1`; zero `SiegeBotController` in the networked run |
| 1 | D3 seat latch | ✓ host `seated as Blue`, client login `seated as Red` |
| 2 | Gold `COND_OwnerOnly` | ✓ client sees own gold (34 vs host's truth 33); the host's Blue gold reads a stale **10** on the client vs the true **116** — enemy gold never crosses |
| 2 | non-authority refusal warns | ✓ **ZERO** in both logs (no D5-lockout leak) |
| 3 | Replicated clock (D8) | ✓ 106.70 s (host) vs 107.57 s (client), sequential reads — inside ±1 s |
| 3 | Match end (D7) | ✓ host `Castle 'Castle_1' (Red) destroyed — match over, winner: Blue`; client `match ended — winner Blue` ~70 ms later (GameState is always-relevant, so it crosses the arena fine) |
| 3 | D14 own-team-relative music | ✓ winning Blue host → `S_VictoryMusic`; losing Red client → `S_DefeatMusic` (own-team-relative, not winner-absolute). The unresolved-sound warnings are the known M7 audio gap, not an M8 defect |
| 5 | Hero `Team` rep + OnRep re-stamp (F2) | ✓ client `AHeroCharacter … Team replicated: Red — capsule channel re-stamped`; `SiegePlayerState_0 … Team replicated: Red` |
| 7 | D5 observer lockouts | ✓ client `PlayHandSlot(0) refused — P1 client observer posture` + same for `DiscardHandSlot`; unit count stayed 0 client-side |
| 3 | Host-initiated Play Again | ✓ resets both machines (client's in-range castle restored to 2000) |
| 9 | Logs (h) | ✓ zero ensure / AccessedNone / Fatal / non-authority warns on both; remaining warnings are engine boilerplate (EditorDataStorageUI, WASAPI) |

**Standalone / practice regression (gate g): FULL PASS — the byte-equivalence claim holds empirically.**
Bot spawned (`Spawned bot opponent 'SiegeBotController_0' with a Red ASiegePlayerState`), no networked latch, `NetMode=0`, single login seated Blue, scatter generated locally (`seed=1428466305`), economy live (Blue 195 / Red 37 gold), bot playing cards + miners (28 units live), bot damaged the player castle to 980/2000, match end fired, **Play Again reset everything** (units 28 → 0, castles → 2000, gold → seed 10/10, hero restored). **The §10/F7 `RestartPlayer` yaw-flatten check passes:** hero at `(-23800, 0, 98)` rot `(0.0, 0.0, 0.0)` — the identical PlayerStart transform the pre-M8 engine path produced.
The `mine(s) NOT path-reachable` warnings + `Mine reachability unconfirmed after 5 culls` error are **PRE-EXISTING and seed-dependent** — verbatim in the committed HEAD file (`git show HEAD:…BattlefieldScatter.cpp`), 0 occurrences under the networked run's different seed. Not an M8 regression; not filed.

**Verdict: qa-failed → back to gameplay-programmer (loop 1 of 3).** Fix BLOCKERS 1–3, investigate FLAGGED 4. TASK-354 is NOT implicated in any finding — its own deferred verification passed in full (11/11 parser rejects, see `qa/TASK-354.md`) — but it stays uncommitted because the M8 P1 lane lands as one unit.
