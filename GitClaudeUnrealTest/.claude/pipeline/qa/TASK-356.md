# QA Report — TASK-356 (M8-rep1: core-state replication pass 1)
Verdict: **PASS** (loop-2 re-review, 2026-07-29 — 0 BLOCKER / 0 WARN / 1 NIT) — the B5 fix replaces a rotted constant with a **derivation from live geometry**, the arithmetic lands exactly 300 uu clear of the measured colliding face, the self-caught UE5 double/float cast is correct and complete, all four respawn paths inherit the fix, the spawn fallback is genuine defense-in-depth (pure pass-through on every success), and **standalone byte-equivalence is verified by reading — Blue's PlayerStart branch is untouched**. Loop-1 and loop-0 records preserved below.

---

# Loop-2 re-review (2026-07-29) — BLOCKER 5: Red hero spawned inside its own castle

Scope per dispatch: verify the derivation + arithmetic + anti-rot claim, the `bOnlyCollidingComponents` rationale against the actual components, the double/float cast, respawn-path coverage (incl. the teleport observation), rule the spawn fallback, verify standalone equivalence by reading, and sweep. Evidence base: `handoffs/TASK-356-programmer.md` loop-2 · `handoffs/TASK-357-buildmaster.md` §7 · the code.

**Context accepted:** the re-run empirically CONFIRMED the loop-1 pass — B1 closed (client regenerates a byte-identical battlefield: same seed/mineStream/layers, 6 gold nodes where it had 0) and B2 closed (far castle at 488 m now 500/500, crumble + destroyed crossing). The NET RELEVANCY LAW is proven in play. B5 is one distance, and the loop-1 reorder is correctly NOT reverted.

## Mandate 1 — the derivation + arithmetic: VERIFIED EXACT

Code (SiegeGameMode.cpp:705-723, castle-relative branch):
```
OwnCastle->GetActorBounds(/*bOnlyCollidingComponents=*/ true, CastleBoundsOrigin, CastleBoxExtent);
const float AuthoredFloorX       = static_cast<float>(HeroSpawnCastleOffset.X);
const float DerivedSpawnDistance = static_cast<float>(CastleBoxExtent.X) + HeroSpawnCastleClearance;
const float SpawnDistance        = FMath::Max(AuthoredFloorX, DerivedSpawnDistance);
```
Tunables verified in the header: `HeroSpawnCastleOffset = FVector(1500.0f, 0.0f, 100.0f)` (SiegeGameMode.h:304 — raised 600→1500, **floor only**), `HeroSpawnCastleClearance = 300.0f` (h:316, EditDefaultsOnly, `ClampMin = "0"`).

**Arithmetic against the measured geometry — every step checks out:**

| Step | Value |
|---|---|
| Castle_Red centre X | 25,000 |
| Measured colliding half-extent X | 1,219 ⇒ span **23,781 … 26,219** |
| `DerivedSpawnDistance` | 1,219 + 300 = **1,519** |
| `AuthoredFloorX` | 1,500 |
| `SpawnDistance = max(1500, 1519)` | **1,519** (derived governs) |
| `TowardCenterline` (Castle.X 25,000 > 0) | **−1.0** |
| `OutLocation.X = 25,000 + (1,519 × −1)` | **23,481** ✔ |
| Clearance from the near colliding face | 23,781 − 23,481 = **exactly 300 uu** ✔ |
| `OutRotation` (TowardCenterline not > 0) | **Yaw 180** ✔ (faces the enemy half) |

Y/Z unchanged (`HeroSpawnCastleOffset.Y = 0`, `.Z = 100`). Blue symmetry checked for completeness: Castle_Blue at −25,000 ⇒ TowardCenterline +1 ⇒ X = −23,481, Yaw 0 — same 300 uu clearance, mirror-correct.

**Anti-rot claim — VERIFIED, and safe in both directions.** On a castle that GROWS, `GetActorBounds` returns the larger half-extent, the derived term dominates the floor, and the spawn moves outward with the geometry — no constant to remember. On a castle that SHRINKS (e.g. back toward the M1 ~405 half-extent ⇒ derived 705), the 1,500 floor governs — still comfortably outside, merely farther than strictly needed. **Degenerate/zero bounds** (mesh unloaded, or a castle whose actor collision is currently disabled — `ApplyDestroyedState`) yield derived = 300 ⇒ the floor governs at 1,500 ⇒ still outside the 1,219 extent. The floor does exactly the job its doc claims. *(Ordering also checked: PlayAgain resets castles — re-enabling collision — at step 3, before restoring heroes at step 5; and `OnCastleDestroyedHandler` cancels all respawn timers, so no respawn resolves against a destroyed castle. The degenerate case is defended but unreachable on shipped flows.)*

## Mandate 2 — `bOnlyCollidingComponents = true`: BOTH claims verified against the actual components
- **HP-bar widget excluded — correct.** `HPBarWidget` is created at relative Z **+3,150** and set `SetCollisionEnabled(ECollisionEnabled::NoCollision)` in the castle ctor. With `bOnlyCollidingComponents=false` it would have inflated the bound with a component that cannot block a spawn — precisely the wrong input for a spawn-clearance question. Flag is right.
- **Gate blocker sits inside — correct.** `GateBlockerVolume` is armed `QueryAndPhysics` at BeginPlay with extent (260, 135, 226) at relative location (6, −525, 284) ⇒ X span [−254, +266] about the castle, deep inside the ±1,219 hull. It is *included* in the colliding bounds (correctly — it does block) and changes the X half-extent by nothing. Both statements hold, so the bounds flag cannot silently reproduce the bug class.
- Ordering sanity: the castle's BeginPlay (which arms the blocker) runs at world init, before any player login/`RestartPlayer`, so the bound queried is the armed one either way.

## Mandate 3 — the self-caught compile trap: cast CORRECT and COMPLETE
`FVector` is `FVector3d` in UE5 ⇒ `.X` is `double`; `FMath::Max` is a single-type template ⇒ `Max(double, float)` fails deduction. Both operands are now explicitly `float`: `AuthoredFloorX = static_cast<float>(HeroSpawnCastleOffset.X)` (:716) and `DerivedSpawnDistance = static_cast<float>(CastleBoxExtent.X) + HeroSpawnCastleClearance` (:717, float + float). `FMath::Max(float, float)` deduces cleanly. Downstream is safe: `SpawnDistance * TowardCenterline` is float→double widening into the `FVector` ctor, and the `%.0f` log args are all float/double (variadic promotion) — the codebase's standard pattern. **This would have broken TASK-357's compile; it is now correct.** Also verified `GetActorBounds(bool, FVector&, FVector&, bool = false) **const**` (Actor.h:1611) — const-correct against the `const ACastle* OwnCastle`, which is a second latent compile break avoided.

## Mandate 4 — respawn-path coverage: all four corrected, and the teleport observation is REAL
All four routes resolve through the same `GetHeroStartTransform`, so all four inherit the derived distance:

| # | Path | Route | Mechanism |
|---|---|---|---|
| 1 | Login / initial spawn | `RestartPlayer` → resolver → `RestartPlayerAtTransform` → `SpawnDefaultPawnAtTransform` | spawn (+ the new fallback) |
| 2 | 5 s hero respawn | `HandleHeroRespawnTimer` → `RestoreHeroAtStart` → resolver (:611) | **teleport** (:615) |
| 3 | PlayAgain step 5 | PlayAgain → `RestoreHeroAtStart` → resolver | **teleport** (:615) |
| 4 | Defensive pawn-lost | `RestoreHeroAtStart` (:605) → `RestartPlayer` | folds into #1 |

**The sharp observation is CONFIRMED and it matters.** Paths #2/#3 call `Hero->SetActorLocationAndRotation(..., /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics)` (:615) — a **teleport, not a spawn**. `SpawnActor`'s collision handling never runs, so these paths would have emitted **no** `SpawnActor failed because of collision`, no `Couldn't spawn Pawn`, and no warning of any kind: they would have **silently planted the hero inside the castle's collision**. The failed run only masked this because Red never obtained a pawn to teleport in the first place. So B5's visible symptom (`pawn=None`) was the login path's version of a defect that had a *silent* version on both respawn paths — and the derived clearance is what finally makes the in-code comment at :614 ("No sweep: the start point is clear by design") true for Red as well. Good catch by the programmer; verified in code.

## Mandate 5 — the spawn fallback: **DEFENSE-IN-DEPTH, not masking** (ruled)
`ASiegeGameMode::SpawnDefaultPawnAtTransform_Implementation` (cpp:280-323, decl h:223):
- **Super FIRST, return on success** (:285-288) ⇒ every succeeding spawn — all standalone, and the re-run measured zero failures — is a **pure pass-through with no behavior change**. Byte-identity preserved structurally, not by argument.
- Only on NULL does it retry the SAME transform with `SpawnCollisionHandlingOverride = AdjustIfPossibleButAlwaysSpawn` (:312), mirroring the engine's `SpawnInfo` exactly (`Instigator` + `RF_Transient`, :310-311) plus a loud `Error` naming `HeroSpawnCastleClearance` as the thing to re-check (:316-320).
- **Why it is not masking:** it changes nothing on any path that already works; it cannot hide the real defect because the derivation fixes the cause upstream and this path logs at **Error** severity naming the tunable when it runs. The alternative outcome it replaces — a silently pawnless, unplayable seat — is strictly worse than a nudged spawn plus a diagnostic. The right posture is exactly the handoff's: *this should never execute*, and **TASK-357's re-run must confirm it stayed asleep** (zero occurrences of that Error line).
- **Engine claims verified:** `AGameModeBase::SpawnDefaultPawnAtTransform_Implementation` (GameModeBase.cpp:**1225-1237**) does spawn with a bare `FActorSpawnParameters` (:1227-1231) — so the pawn class's own `SpawnCollisionHandlingMethod` governs, exactly as the rationale states — and logs `"SpawnDefaultPawnAtTransform: Couldn't spawn Pawn"` at :1234, matching the observed failure line. Override signature matches the virtual exactly. `AdjustIfPossibleButAlwaysSpawn` confirmed at EngineTypes.h:**4418** (and already compiling elsewhere in this module: Barracks.cpp:143, SiegePlayerController.cpp:3315).

## Mandate 6 — standalone byte-equivalence: **HOLDS (verified by reading, not trusting)**
The PlayerStart branch (:670-683) is **byte-unchanged from loop-1**: same `FindPlayerStart` → `IsA<APlayerStart>` guard → same-side test → `OutLocation = Start->GetActorLocation()`, `OutRotation = FRotator(0, Start yaw, 0)` → **`return`**. In standalone the single player is Blue; L_Arena's only PlayerStart (−23,800) and Castle_Blue (−25,000) are both X ≤ 0 ⇒ signs equal ⇒ the branch accepts and **returns before the derivation branch is ever reached**. Single-player still lands **`(-23800, 0, 98)` rot `(0,0,0)`** — the measured gate-(g) pass reproduces. The castle-relative branch is not on the standalone path in L_Arena at all, and the spawn override is a pass-through whenever Super succeeds (which it did, zero failures, in the standalone regression). **Byte-equivalence verdict: intact.**

## Mandate 7 — standing sweep
- **Nothing else moved — evidenced, not assumed.** A module-wide count of the loop-1 surfaces (`bAlwaysRelevant` · `SetNetCullDistanceSquared` · `DOREPLIFETIME` · `UFUNCTION(Server` · `GetFirstPlayerController`) returns **33 hits across 14 files, identical to loop-1's distribution** — Castle.cpp 6 · BattlefieldScatter.cpp 4 · HeroCharacter.cpp 2 · SiegeGameState.cpp 6 · SiegePlayerState.cpp 2 · SiegePlayerController.h 2 (the one Server RPC + one doc mention) · headers 1 each. **`SiegeGameMode.{h,cpp}` appear NOWHERE in that set**, which is the cleanest possible proof that the loop-2 delta touched no relevancy, replication, OnRep, or RPC surface — it is confined to spawn geometry. The loop-1 reorder itself is intact (:654-683 read).
- **ONE-RPC law still exact** (single `UFUNCTION(Server, Reliable, WithValidation)`); **no new `GetFirstPlayerController`** (zero call sites; the two hits are doc/comment).
- **Compile traps:** the double/float cast (above) and the `const` bounds call (above) are the two real ones and both are correct; no literal `*/`; the new `Error`/`Log` Printf format specifiers match their argument lists; no shadowing; `HeroSpawnCastleClearance` carries `EditDefaultsOnly` + `ClampMin` (tunable hygiene).

## NIT (note, no action owed)
1. **Semantic overload on `HeroSpawnCastleOffset`:** its `.X` is now a *floor* while `.Y`/`.Z` remain literal offsets, all inside one `FVector`. The header doc says so explicitly, so nothing is ambiguous today; a future touch may prefer splitting it (`HeroSpawnCastleMinDistance` + a 2-component lateral offset). Cosmetic only.

## What TASK-357's re-run must observe (loop-2)
1. **B5 dead:** client login logs the derivation line with `spawn distance 1519` and location ≈ `(23481, 0, 100)`; **`SiegePlayerController_1 pawn=<BP_HeroCharacter_C>`** (not `None`); two `BP_HeroCharacter_C` actors in the world; **zero** `SpawnActor failed because of collision` / `Couldn't spawn Pawn`.
2. **The fallback stayed asleep:** **zero** occurrences of the new `Default pawn spawn … was refused for collision` Error. (If it fires, the derivation is wrong for the current geometry — read the value it prints.)
3. **Respawn paths (the silent class):** kill the client hero → respawns Red-side after 5 s at the derived spot; Play Again → both heroes restored to their own sides. Confirm neither ends up inside geometry (these paths teleport and will not self-report).
4. **Standalone regression:** hero still `(-23800, 0, 98)` rot `(0,0,0)`, bot spawns, full Play-vs-Bot pass, zero spawn failures.
5. Loop-1 items stay green (B1 byte-identical battlefield + 6 gold nodes; B2 far castle 500/500 with crumble/destroyed crossing; Tier-A control group).
6. **Still NOT closable in this lane (carried):** real-button `ServerRequestPlayAgain` routing needs genuine OS input on an unlocked desktop — a scripted invoke proves nothing (the callspace-Local law). Report as attempted/blocked, never as a scripted pass.

**Status: qa-passed (loop 2)** → TASK-357 re-runs the two-client gate.

---
---

# Loop-1 re-review (2026-07-29) — NET RELEVANCY LAW + blocker 3 + finding 4 (PASS: 0 BLOCKER / 0 WARN / 2 NIT)

## Mandate 1 — policy compliance table (all rows PASS)

| Requirement (ruled law) | Verified | Evidence |
|---|---|---|
| Constant in the one-concept header | PASS | `SiegeNetLimits.h` — `namespace SiegeNet`, no UCLASS, no module dep; carries tier definitions, the measured defect, and the P2 wave duty |
| **Squared value DERIVED by multiplication, never a typed 3.6e9** | PASS | `ArenaRelevancyDistanceSquared = ArenaRelevancyDistance * ArenaRelevancyDistance` (:65), `inline constexpr` both |
| Tier A `bAlwaysRelevant` on exactly `ACastle` + `ASiegeBattlefieldScatter` | PASS | Castle.cpp:57, BattlefieldScatter.cpp:155 — module grep: the only two assignments |
| Tier B hero uses the SETTER from the constant, **no hand-typed distance** | PASS | HeroCharacter.cpp:69; the only `SetNetCullDistanceSquared` call site and **no numeric relevancy literal anywhere** |
| GameState/PlayerState **verified-and-documented, NOT blind-set** | PASS | Nothing set in either subclass. Citations exact: GameStateBase.cpp:**25**/:**26**, PlayerState.cpp:**25**/:**26** — the control group that proves the relevancy diagnosis |
| **All six replicated classes declare their tier in-header** | PASS | Castle.h:74 · Scatter.h:89 · Hero.h:283 · GameState.h:81-83 · PS.h:87-89 · PC.h:187-195 |
| Relevancy ≠ condition (Gold owner-only intact) | PASS | `DOREPLIFETIME_CONDITION(..., Gold, COND_OwnerOnly)` untouched |
| D9 correction recorded in code | PASS | BattlefieldScatter.cpp:150-155 |

## Mandate 2 — UE 5.5 deprecation claim: VERIFIED
Actor.h:**898-900** `UE_DEPRECATED(5.5, ...)` on the raw `NetCullDistanceSquared`; setter at :**4648**; `bAlwaysRelevant` a plain non-deprecated UPROPERTY (:331-333). Setter is ctor-safe (Actor.cpp:7022-7027, bare field assign). A direct assignment would have broken the compile.

## Mandate 3 — blocker 3: correct; standalone equivalence held
Team governs; PlayerStart accepted only on the own-team side by castle X-sign (data-driven, no Blue/Red hardcode); castle-relative branch reachable; origin fallback null-safe. Standalone: Blue PlayerStart (−23,800) and Castle_Blue (−25,000) both X ≤ 0 ⇒ accepted ⇒ `(-23800, 0, 98)` rot `(0,0,0)`.
*(Loop-2 note: the reorder was right; only the castle-relative DISTANCE was rotted — B5.)*

## Mandate 4 — FINDING 4: the refutation is **SOUND** (all three claims)
(a) `APlayerController`'s ctor never sets `bReplicates` (only `ANoPawnPlayerController` :6813); the server enables it at login (World.cpp:**4936-4938**) and it is not itself replicated. (b) `AActor::GetFunctionCallspace` read in full (Actor.cpp:**5467-5666**) — **`bReplicates` is never read**; a client's only non-Remote outcome is Absorbed + `LogNet Warning` (:5652-5661), which does not run the body. (c) The first branch returns `Local` on `GAllowActorScriptExecutionInEditor` (Actor.cpp:**5469-5474**), set by `FEditorScriptExecutionGuard` (ScriptCore.cpp:**450-455**) — **and QA closed the asserted link: `PyUtil::InvokeFunctionCall` declares that guard at PyUtil.cpp:644**, so every python-invoked RPC resolves Local. **The defect was the test lane, not the code.**

**Law correction routed to the manager:** keep the corollary's general principle (a *locally client-spawned* `bReplicates=false` actor does keep `ROLE_Authority`), correct its application (net-spawned actors take roles from the channel — the PC reading false is normal; nothing in P1 was mis-guarded), and **add the clause: RPC routing must never be validated from python remote-exec.**

**Hardening ruled:** PC ctor `bReplicates = true` — inert-but-honest, masks nothing. `ServerRequestPlayAgain_Implementation` authority guard — strictly diagnostic-improving, blocks no legitimate path.

## Loop-1 sweep + NITs
No new `GetFirstPlayerController`; rep hygiene unchanged (same 15 registrations/conditions, no OnRep touched); ONE-RPC law exact; compile traps clean. NITs: (1) `ArenaRelevancyDistanceSquared` 3.6e9 in float — correct, headroom noted; (2) handoff cites ScriptCore.cpp:451-455, assignment is at :455 — materially correct.

---
---

# Loop-0 review (2026-07-29) — PASS: 0 BLOCKER / 0 WARN / 3 NIT (preserved as the audit record)

## STEP 0 — the addendum: VERIFIED
`handoffs/TASK-353-architecture-addendum.md` exists, is evidence-based (file:line vs `949c252`), resolves all three §7 items, pins the crumble names. **356 complies with both fences:** the castle gating-config path is authority-UNgated (`HasAuthority` at exactly three Castle sites — TakeDamage/ResetCastle/HealOverTime); the hero `OnRep_Team` is the resolved re-stamp + `PossessedBy` server-side re-stamp; `OnRep_CrumbleStage` branches `>0 → ApplyCrumbleStage`, `==0 → ApplyTeamVisuals`.

## Mandate 2 — byte-equivalence: HELD (per site)
GameMode latches/seats/SpawnBot gate authority-inert; `SetMatchResult` → notify resolves the same single local PC the retired push loop called; PS accrual-fork kill leaves server/standalone accrual unchanged (13 guards, `SetGold` correctly unguarded); GameState client Tick fork unreachable; PC lockouts unreached, `RequestPlayAgain` authority branch = the widget's old direct call, `PerformLocalMatchReset` = `HandleMatchReset` alone (F4 — avoids the qa/TASK-024 double-reset); both unit resolves equivalent under one Blue PC; scatter authority path = old sequence + two int writes; feedback local-viewer loop = old index 0; hero `PossessedBy`/melee gate authority-true.

## Mandate 3 — rep hygiene: COMPLETE
15 registrations complete both directions; `Gold` COND_OwnerOnly correct; OnRep naming exact incl. the ratified bool-prefix drop; Castle `Team` COND_InitialOnly right vs PS `Team` plain right; `bReplicates` exactly where designed; P1 scope fence held.
*(Loop-1 correction of record: registrations were right, but **relevancy** — outside P1's design — kept Castle/Scatter state from reaching a distant client. The class of defect a pre-compile review cannot see, which is why the two-client gate exists.)*

## Mandates 4–6
ONE-RPC law held (`ServerRequestPlayAgain` only); the `GetFirstPlayerController` ban resolved (all five audit sites); Scatter D9 correct in code with same-bunch seed-before-regen ordering *(its superset argument was void until Tier A made the seed arrive — the manager's D9 correction)*.

## Mandate 7 — F1–F10 (all ACCEPT)
F1 doc-table handler name · F2 addendum-resolved re-stamp · F3 null-safe fallback over doc purity · F4 verified single ResetDeck · F5 defensive-path-only delta · F6 float horizon immaterial · F7 function-local static FText · F8 bounded 120 retries · F9 doc-letter client mines · F10 `mutable` latch.

## Loop-0 NITs
1. F1 handler name — recorded exception. 2. `ASiegePlayerState::SetTeam` public + BlueprintCallable + unguarded (latent-only; **P2 flag**). 3. F3's retained fallback — deliberate doc deviation.

## Loop-0 sharpest finding (positive)
`OnRep_Gold` broadcasts `OnGoldChanged` DIRECTLY rather than through the guarded `SetGold` choke — the two-writer trap that would have silently refused every client gold display; and the four D5 entry lockouts seal all 14 downstream mutation surfaces.
