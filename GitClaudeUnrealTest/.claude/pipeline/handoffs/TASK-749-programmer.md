# TASK-749 — `ASiegeGhostPawn` — programmer handoff

**Status:** ready-for-qa · **Law:** `GHOST-§1`/`§3`/`§4`/`§5`/`§6`, `SC-§35`, `TOWER-§9.2` (inverted), `ACC-§8`, `SHIP-§9c`
**Compile:** ⛔ not attempted — TASK-754 owns the one compile, serialized behind the tower wave.

## Files — NEW ONLY (3), ⛔ zero existing files edited

| file | note |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGhostPawn.h` | NEW, sole |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeGhostPawn.cpp` | NEW, sole |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeGhostPawnTest.cpp` | NEW, sole (checked first — no ghost/pawn test existed) |

✅ **Fence verified by `git status`:** the only paths I created are the three above. `SummonedUnit.*`, `Tower.*`, `ClimbableTower.*`, `HeroCharacter.*`, `SiegeGameMode.*`, `SiegePlayerController.*`, `WarMapWidget.*`, `SiegeAssistantSnapshot.*` all show as modified **by other agents**, ⛔ not by me. ⛔ No editor, ⛔ no MCP, ⛔ no Git, ⛔ no `Capture()`/`EnsureSnapshot()`, ⛔ Zone A untouched, ⛔ no token figure.

## ⭐⭐ THE SAFETY ARGUMENT — verified independently, and it is WIDER than the ruling claimed

`GHOST-§1` cited **two** acquisition sites. I verified those two and then swept for every other damage lane, because a safety argument that only covers the cited paths is worthless. **All eight enumerations gather through `GetAllActorsWithInterface(UTeamAgent::StaticClass())`:**

| # | site | lane |
|---|---|---|
| 1 | `SummonedUnit.cpp:1717` `AcquireTarget` | unit fleet ✅ (cited) |
| 2 | `SummonedUnit.cpp:2262` `AcquireEnemyNearPoint` | attack-zone orders ⭐ **not cited** |
| 3 | `Tower.cpp:220` `AcquireTarget` | all tower cards ✅ (cited) |
| 4 | `Tower.cpp:378` chain-zap selection | Crystal bounces ✅ (cited) |
| 5 | `SiegeCombatStatics.cpp:36` `ApplyRadialDamage` | ⭐⭐ **EVERY AoE** — Sapper suicide, Bomb Tower, Fireball. **Not cited, and the most important one**: AoE catches bystanders who were never "acquired" at all |
| 6 | `SpellLibrary.cpp:64` | spell resolvers ⭐ not cited |
| 7 | `SpellLineSweep.cpp:133` | line-sweep spells ⭐ not cited |
| 8 | `HeroCharacter.cpp:404` | the enemy hero's own melee ⭐ not cited |

**And the two paths that are ⛔ NOT enumerations — read, because they are where an omission-based argument would actually break:**
- `Projectile.cpp:352` applies damage to `Target.Get()`, an explicit pointer captured **at fire time** from an acquisition above ⇒ **target-locked, never a collision sweep.** A ghost in the flight path is neither victim nor shield.
- `Projectile.cpp:434` `FindTerrainHit` line-traces `ECC_WorldStatic`/`ECC_WorldDynamic` **object types only** (its own comment: *"Pawn/Vehicle object types are NOT queried"*) and then requires a `Terrain`/`Obstacle` tag ⇒ the ghost is excluded **twice**.
  ⚠️⚠️ **THIS PRODUCED A REAL DESIGN CONSTRAINT QA SHOULD CHECK:** `G-2`'s *"blocks WorldStatic"* is a **RESPONSE** ruling, ⛔ **not an object-type ruling.** Re-typing the capsule's *object type* to `ECC_WorldStatic` — a plausible misreading — would insert the ghost into that trace and hand a dead player a **projectile shield**. The capsule stays `ECC_Pawn`; **test 3(c) asserts it and names the reason.**

⇒ **Conclusion: omitting `ITeamAgent` genuinely removes the ghost from every acquisition and every damage lane in the project.** ⛔ Zero suppression code written: no `bInvulnerable`, no damage guard, no targeting filter, no edit to any unit/tower file.

## ⭐ "CANNOT ATTACK" IS ALSO STRUCTURAL — a second free win

Measured at `HeroCharacter.cpp:282-321`: the hero's offensive verbs (`IA_Attack`→`DoMeleeAttack`, `IA_Sprint`, `IA_Rally`) are bound in **`AHeroCharacter::SetupPlayerInputComponent`** — on the **hero pawn**. `ASiegeGhostPawn::SetupPlayerInputComponent` binds **only Move and Look** ⇒ while the ghost is possessed **there is no attack binding in existence to press.** No suppression branch.

## ⚠️⚠️ THE FINDING THAT MOST AFFECTS TASK-750 — the input-dead trap is REAL and I closed my half

**`ASiegePlayerController` does NOT add the input mapping context.** The **hero pawn** does, in `NotifyControllerChanged` (`HeroCharacter.cpp:228-280`, whose own comment says *"ASiegePlayerController (TASK-007) does not add contexts the way the template controllers do"*).

⇒ **The instant the controller leaves the hero, nothing re-adds `IMC_Hero`.** A ghost that did not do this itself would leave the player unable to move, order, or open the map **for the full 180 s** — exactly `GHOST-§4`'s named disaster, now 36× more expensive than at 5 s.

✅ **`ASiegeGhostPawn::NotifyControllerChanged` mirrors the hero's guard chain**, including the `KBD-§5`/`KBD-§6` positional-layout resolve at the **innermost scope** (hoisting it would probe an OS keyboard layout for a machine that is not there). Without that resolve a Dvorak player's ghost would be WASD-broken while their living hero was fine — invisible to every QWERTY reviewer.
⛔ **I add a context; I never touch input MODE.** No `SetInputMode`, no `bShowMouseCursor`, no `EnableInput`/`DisableInput` anywhere in my files — `HELP-§5` binds that to `ApplyCursorInputState()` and the ordering is TASK-750's.

## ⭐ WHAT ALREADY WORKS FOR THE GHOST FOR FREE (measured — TASK-750 should not rebuild these)

- **`G-3` powers are controller-bound** (`SiegePlayerController.cpp:464-626`): unit orders, assistant console, war map, controls overlay, card slots. They survive the possession swap and work the moment the context is added.
- **`GetFollowAnchor()` (`:3522`)** returns `GetPawn()` as a plain `APawn*`; the `AHeroCharacter` cast is only the dead-hero gate ⇒ **the ghost becomes the follow anchor automatically** — `G-8`'s intent, at zero cost.
- **`IsHeroInCommanderRange()` (`:4819`)** reads a plain `const APawn*` ⇒ **the ghost reaches the AI commander** with no special case.
- **The player's team lives on `ASiegePlayerState::GetTeam()`**, ⛔ not on the pawn (`:4815`) ⇒ order routing and commander lookup are unaffected by the swap. ⭐ **This is the deep reason the ghost needs no team and no `ITeamAgent` to command anything.**

## The API TASK-750 consumes — three calls, precisely

```cpp
void      InitializeGhost(ETeamId InTeam);  // ① after SpawnActor, BEFORE Possess
ETeamId   GetGhostTeam() const;             // ② plain accessor, ⛔ NOT ITeamAgent::GetTeamId()
void      RetireGhost();                    // ③ teardown — IDEMPOTENT
```
- **`RetireGhost()` is idempotent by contract** so TASK-750 needs no "already retired?" flag — and because **two** legitimate callers exist (respawn at `HeroRespawnDelay`, and match end per `GHOST-§2`'s inherited rule) which can both fire when a match ends near the respawn boundary.
- ⚠️ **ORDERING TASK-750 MUST HONOUR:** re-possess the hero **FIRST**, retire the ghost **SECOND**. Retiring a still-possessed pawn leaves the controller pawnless — the input-dead failure in another costume. `RetireGhost()` deliberately does **not** un-possess (possession ordering is `GHOST-§4`'s, not this pawn's) and **warns** if called while possessed.
- ⛔ **This class owns no timer**, so `PlayAgain()`'s clear-only-what-you-own policy has nothing to leak through it.
- Assign on `BP_SiegeGhostPawn`: `GhostMappingContext` = **`/Game/Input/IMC_Hero`** (⚠️ a ghost-only context would silently drop every `G-3` power), `MoveAction`/`LookAction`/`MouseLookAction`, and `GhostMesh`.

## ⚖️ The three rulings I was asked to make and declare

1. **Movement + vision (`G-1`)** — the hero's own, and **DERIVED, ⛔ not duplicated**: `BeginPlay` reads `GetDefault<AHeroCharacter>()->GetEffectiveWalkSpeed()`, so the number cannot drift into a second source of truth. The **CDO** is read deliberately (`SwiftBootsStacks = 0`) ⇒ the ghost is **not** buffed by the items the corpse carried. Camera rig mirrors the hero exactly (arm 400, `bUsePawnControlRotation`) ⇒ vision identical, ⛔ not extended. ⛔ **No sprint** — a hero ability bound on the hero pawn, and `G-3`'s list is closed. ⛔ No flight (`bCanFly=false`, gravity 1.0, `DefaultLandMovementMode=MOVE_Walking`).
2. **Interaction (`G-3`)** — **orders only.** ⛔ No capture, interaction, pickups, or card play (`G-5` default — flagged, ⛔ built neither way). Enforced by omission: no such binding exists on this pawn.
3. **Collision (`G-2`)** — **blocks `WorldStatic`, ignores `Pawn`**, object type `ECC_Pawn`, `QueryAndPhysics`, no overlap events, no navigation effect, `bEnablePhysicsInteraction=false`. ⭐ The asymmetry is the point: it needs a floor, and a pawn-blocking ghost is a **free body-block wall handed to a dead player**.

## ⚠️ DECLARED DEVIATION — base class is `ACharacter`, not a direct `APawn`

`GHOST-§1` writes `ASiegeGhostPawn : public APawn`. **I derived from `ACharacter`, which IS-A `APawn`** — every statement the law makes about the type stays literally true, including `GHOST-§4`'s *"is an `APawn`, so `TryGetPawnOwner()` resolves"*.

**Reason — `G-1` and `G-2` themselves:** `UCharacterMovementComponent` (gravity, ground-walking, step-up, slopes) **requires an `ACharacter` owner** and does not function on a bare `APawn`. Alternatives weighed and refused: `UFloatingPawnMovement` **flies** (⛔ `G-1`); hand-rolling gravity onto a bare `APawn` re-implements an engine component for a worse result. `ACharacter` also supplies the `USkeletalMeshComponent` that `GHOST-§4` explicitly anticipates and the capsule `G-2` configures.
✅ **The law's binding content — a pawn that does not implement `ITeamAgent` — is honoured exactly**, and **test 4 asserts the `APawn` claim at the type** so the deviation can never quietly become a violation. ⚖️ QA's call; if ruled against, the cost is a hand-rolled gravity implementation, which I consider strictly worse.

## ⚠️ TWO CONSEQUENCES DECLARED RATHER THAN SILENTLY SHIPPED

- **(a) The ghost carries no team collision channel.** `AHeroCharacter` re-stamps its capsule to `SiegeTeamObjectChannel(GetTeamId())` (`HeroCharacter.cpp:113`) because those are the **combatant body** channels. A ghost is not one.
- **(b) ⇒ It is not stopped at castle gates.** `ACastle::GateBlockerVolume` ignores all channels and blocks only the enemy **team** channel (`Castle.cpp:606-608`) ⇒ it does not stop an `ECC_Pawn` ghost. **Judged acceptable, FLAGGED for TASK-755:** `G-3` makes the intrusion mechanically inert, `G-4` makes it observable, and the castle's actual **walls are `WorldStatic` and DO block it** — so this is ⛔ not the wall-pass `G-1` forbids. One line to change if Jonathan rules otherwise.

## 📌 M8 declaration (`GHOST-§6`) — "nothing to declare" would be FALSE here

**⚖️ NET RELEVANCY TIER B** — arena-scaled `SetNetCullDistanceSquared(SiegeNet::ArenaRelevancyDistanceSquared)`, ⛔ never a hand-typed literal. Same tier as `AHeroCharacter` deliberately: the ghost stands in for the hero, and `G-4` requires the **enemy** to see it across the 500 m arena. ⛔ `bReplicates` is **not** set here — `APawn`'s constructor already does it (mirrored from `HeroCharacter.cpp:61`). ⛔ **No RPC and no replicated property authored** (`ACC-§8`); the reserved shape is named: `GhostTeam` is the one property needing `Replicated` + `GetLifetimeReplicatedProps` when M8 lands. **Test 7 asserts the tier** — a declaration in a comment cannot fail; that test can.

## Tests — 8 new, `SiegeGhostPawnTest.cpp`

Every assertion can FAIL, and **every negative claim ships with a self-check proving the instrument can still see a positive** (the house anti-vacuity discipline from `SiegeClimbableTowerTest.cpp`).

1. ⭐⭐ **`…DoesNotImplementITeamAgentAndIsThereforeUnacquirable`** — the headline, asserted in **two independent lanes**: the **reflection** lane (`ImplementsInterface` — the exact predicate `GetAllActorsWithInterface` filters on) and the **cast** lane (`Cast<ITeamAgent>` — what every acquisition performs after gathering). Self-checks: hero, unit **and** tower all return TRUE. Failure text names the consequence and says *"⛔ do NOT fix this by adding a flag; REMOVE THE INTERFACE."*
2. `…DoesNotImplementIHealthBarProvider` — self-check: the hero DOES.
3. `…BlocksWorldStaticAndNeverBlocksPawn` — (a) blocks WorldStatic (b) never blocks Pawn (c) ⭐ object type is `ECC_Pawn` (the projectile-shield guard) (d) neither team channel (e) QueryAndPhysics. Self-check: the two responses **differ**, proving a real per-channel container.
4. `…IsAPawnAndIsNeverAHeroOrAUnit` — machine-checks the law's own `APawn` claim; also not-a-hero, not-a-unit.
5. `…DeclaresNoAttackDamageOrInvulnerabilityMember` — reflection walk (`ExcludeSuper`) for `Attack`/`Damage`/`Invulnerab`/`Immune`/`Health`. **Two self-checks**: the walk finds `GhostTeam` + `GhostMappingContext`, and ⭐ the same scan **finds those tokens on `AHeroCharacter`** — otherwise a clean result is vacuous. ⚠️ Limitation stated in-file: the walk sees only **reflected** members.
6. `…IsVisibleToEveryoneNotJustItsOwner` — `G-4`; states the deliberate contrast with `MARK-§ M-3`.
7. `…NetCullDistanceIsArenaScaledTierB` — the M8 tier, plus parity with the hero's.
8. `…WalksAndNeverFliesAndDerivesTheHeroSpeed` — no flight, walking mode, normal gravity, no physics shove, and ⭐ a self-check that **the derivation source is live** (goes red if the hero's speed API is removed/zeroed).

**⚠️ Not covered, stated so the gap is honest:** possession/cursor ordering, the 180 s timer and the runtime context re-add are TASK-750's, and `GHOST-§4` rules their instrument is **PIE with a message-log read** — never waivable on a clean compile.

### Suite total for TASK-754
**+8 from TASK-749.** ⚠️ ⛔ **Do not take an absolute from me:** I measured the folder at **196** at task start and **206** at task end, but only 8 are mine — `SiegeLadderClimbTest.cpp` moved 11→13 under me (the live ladder wave). **Recompute the absolute at compile time.**

## What QA should scrutinise

1. ⭐⭐ **The base-class deviation** (`ACharacter` vs the law's literal `APawn`) — reasoned above; test 4 machine-checks the law's actual claim. Rule it.
2. ⭐ **The `ECC_Pawn` object-type constraint** — confirm nobody "fixes" it to `WorldStatic` to satisfy `G-2`. That would be a projectile shield.
3. **Cross-task compile coupling I accepted deliberately:** `SiegeGhostPawn.cpp` includes `Siegebound/HeroCharacter.h` (**read-only**, ⛔ no edit) for `GetEffectiveWalkSpeed()`. `HeroCharacter.{h,cpp}` is **TASK-748's** live surface. The API is public, `const`, and in an untouched region — but if TASK-748 renames it, my file is the second casualty. Preferred over duplicating `500.f`, per `G-1`'s "the hero's OWN".
4. **`GhostTeam` currently has no C++ consumer** — stated honestly in-code, not oversold. Reserved for the `G-4` tint and M8.
5. **An art dependency is OWED and unboarded:** ⛔ no task in this batch produces a ghost **mesh** or a team-tinted **material**. `BeginPlay` **warns loudly** when `GhostMesh` is unset because an invisible ghost breaks `G-4` outright. Interim: assign the hero's own mesh on `BP_SiegeGhostPawn` — also the most faithful reading of *"similar to their original body."*
6. **`BP_SiegeGhostPawn` does not exist yet** — TASK-750's `GhostPawnClassAsset` falls back to the raw C++ class by its own spec, so a missing BP is a **degraded-but-alive** ghost (no mesh, no input context ⇒ warns). Worth an editor/art follow-up.
