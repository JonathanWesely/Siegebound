# TASK-344 [CMD-3zone] — programmer handoff (gameplay-programmer, 2026-07-27)

3-zone HOLD rework + AMBUSH, full C++ file-only per the plan (`C:\Users\wesel\.claude\plans\groovy-bouncing-manatee.md` Lane B) + CONVENTIONS "Group orders — 3-zone HOLD + AMBUSH". Five files touched, nothing else. NOT compiled (file-only task — build-master compiles at TASK-346). Line refs below are POST-EDIT.

## Per-file delta map

### Source/GitClaudeUnrealTest/Siegebound/UnitCommand.h (rewritten, 118 lines)
- File doc updated to record the supersession (R now opens the group pick; the Hold stance member survives for WBP_HUD's pins).
- `ESiegeUnitCommand : uint8 { Attack, Hold, Defend }` — **BYTE-IDENTICAL** (same members, same order, same underlying type; only comment text changed).
- NEW `ESiegeGroupCommandType : uint8 { Hold, Ambush }` (≈h:56-61).
- NEW `USTRUCT() FSiegeUnitGroup` (≈h:70-118): `GroupId` (INDEX_NONE default) · `Type` · `PositionCenter`/`PositionRadius` · `AttackCenter`/`AttackRadius` · `Members` (`TArray<TWeakObjectPtr<ASummonedUnit>>` — the group never owns unit lifetimes) · `PositionMarkerDecal`/`AttackMarkerDecal` (`TObjectPtr<ADecalActor>`, Transient). Forward decls `ADecalActor`/`ASummonedUnit` keep the header pure-data.
- File converted to CRLF to match the repo's working-copy endings (the rewrite initially landed LF).

### Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h
- NEW `FOnCommandPromptChanged` delegate (const FString& — the FOnCardRefused pattern; ≈h:55-66) + `UPROPERTY(BlueprintAssignable) OnCommandPromptChanged` (≈h:209-211). Empty string = pick over/groups released ⇒ HUD falls back to the stance display (the TASK-345 additive-bind contract).
- DELETED: `GetHoldLocation()` / `GetHoldRadius()` getters, `HoldRadius` tunable, `HoldLocation`, `bInHoldTargetMode`, `bHoldSurfaceValid`, `HoldPickLocation`, `HoldHero`, and the four hold-pick function decls.
- NEW public `const FSiegeUnitGroup* FindUnitGroup(int32) const` (≈h:217-225) — the unit-side live-group resolve; doc pins "never cache the pointer".
- NEW handler `OnCmdAmbushPressed()` (≈h:435-436); `CmdAmbushAction` slot (≈h:581-583) + `CmdAmbushActionAsset` soft path `/Game/Input/Actions/IA_CmdAmbush` (≈h:629-631) — null-safe, F inert until TASK-345.
- NEW six tunables replacing `HoldRadius` (EditDefaultsOnly, Category "Siegebound|Commands", ≈h:645-682): `GroupRadiusWheelStep 100` · `GroupRadiusMin 200` · `GroupRadiusMax 5000` · `GroupSelectRadiusDefault 1200` · `GroupPositionRadiusDefault 700` · `GroupAttackRadiusDefault 1500`. All flagged for Jonathan's feel-pass.
- NEW private `enum class EGroupPickStage : uint8 { None, Select, Position, AttackZone }` (right after EPlacementInvalidReason).
- NEW private function decls (h:852-926): `BeginGroupPick` h:852 · `UpdateGroupPickReticle` h:860 · `ApplyGroupPickWheel` h:869 · `ConfirmGroupPickStage` h:884 · `CancelGroupPick` h:896 · `SpawnGroupCircleDecal` h:907 · `PruneUnitGroups` h:915 · `ClearAllUnitGroups` h:923 · `BroadcastCommandPrompt` h:926.
- NEW state block replacing the hold scratch (h:1152-1218): `UnitGroups` (UPROPERTY(Transient) TArray<FSiegeUnitGroup> — keeps the marker TObjectPtrs GC-visible) · `NextUnitGroupId` (never reused → a stale unit id can never alias a new group) · `UnitGroupPruneTimerHandle` · `GroupPickStage/Type/Radius` · `bGroupPickSurfaceValid`/`GroupPickLocation` · `GroupPickSelectedMembers` (weak) · `GroupPickPositionCenter/Radius` · `GroupPickActiveDecal`/`GroupPickSelectDecal`/`GroupPickPositionDecal` · `GroupPickHero` (its OWN melee-suppression record).

### Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp
- Includes: `TimerManager.h` added (cpp:40). Anonymous namespace: `UnitGroupPruneInterval = 1.f` + `GoldenAngleRadians = 2.399963f` impl constants (cpp:56-70 — deliberately NOT tunables; the six feel tunables are the UPROPERTYs).
- Constructor: `CmdAmbushActionAsset` soft path (cpp:97).
- `BeginPlay`: arms the 1 s prune timer for the controller's lifetime (cpp:205-210).
- `SetupInputComponent`: resolves `CmdAmbushAction` (cpp:252, creating task TASK-345) + binds `OnCmdAmbushPressed` (cpp:316-323) — both skipped null-safe while the asset is missing.
- `PlayerTick` pick branch (cpp:330-366): polled RMB/Esc → `CancelGroupPick`; **polled wheel** `ApplyGroupPickWheel()` (the only call site — wheel INERT outside the branch); `UpdateGroupPickReticle()`; polled LMB → `ConfirmGroupPickStage()`. Placement + targeting branches byte-identical below it.
- Handlers: `OnCmdHoldPressed` → `BeginGroupPick(Hold)` (cpp:820); NEW `OnCmdAmbushPressed` → `BeginGroupPick(Ambush)` (cpp:827); `OnCmdAttackPressed` (cpp:804) / `OnCmdDefendPressed` (cpp:834) now do `CancelGroupPick(); ClearAllUnitGroups();` BEFORE `SetUnitCommand` (the release law; cpp:815-816 / 842-843).
- Mutual-ignore guards renamed from the hold flag to `GroupPickStage != None`: `EnterPlacementMode` (≈cpp:906-914), `EnterTargetingMode` (≈cpp:1606-1614), `ApplyCursorInputState` (cpp:3318 body — cursor-owner OR). Both directions preserved: `BeginGroupPick` still silently ignores while placement/targeting is live (cpp:2049-2075).
- `HandleMatchReset`: the stance-reset block drops the deleted `HoldLocation` zeroing and calls `ClearAllUnitGroups()` (cpp:1195-1203) — Play-Again releases every group + markers.
- NEW implementation block (cpp:2040-2640): `BeginGroupPick` 2040 · `UpdateGroupPickReticle` 2118 · `ApplyGroupPickWheel` 2144 (step/clamp per the tunables; resizes the active decal in place) · `ConfirmGroupPickStage` 2179 (see behavior notes) · `CancelGroupPick` 2378 · `SpawnGroupCircleDecal` 2437 (the SpawnSpellReticle recipe cloned: null-safe M_SpellReticle with the SHARED `bWarnedNoReticleMaterial` warn-once latch; IDENTITY spawn → ABSOLUTE −90 pitch — the TASK-100 composition lesson; `DecalSize=(500,R,R)`; optional "StageTint" MID) · `PruneUnitGroups` 2525 · `ClearAllUnitGroups` 2563 · `FindUnitGroup` 2605 · `BroadcastCommandPrompt` 2623 (every prompt also UE_LOGs — ships without the WBP bind).
- `SetUnitCommand` (cpp:786) — **byte-identical** (WBP_HUD stance contract).

### Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h
- Forward decl `struct FSiegeUnitGroup;` (h:27).
- NEW public API after `GetUnitState()` (≈h:312-353): `AssignCommandGroup(int32, const FVector&)` · `ClearCommandGroup()` · `IsGroupCommandEligible() const` (the ONE public eligibility surface — `Profile` is private) · `GetCommandGroupId() const` (debug/PIE hook).
- `UpdateStateStandardCommanded` doc: HOLD bullet replaced with the supersession note; NEW `UpdateStateGrouped(const FSiegeUnitGroup&)` decl with the full ladder doc (≈h:633-656).
- NEW private members `CommandGroupId` (int32, INDEX_NONE) + `GroupStationOffset` (FVector) — both `UPROPERTY(VisibleInstanceOnly, Transient)` per the law (≈h:975-982).
- `DefendRadius` doc trimmed of its dead `HoldRadius` cross-reference (no code change).

### Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp
- Include comments updated + explicit `#include "Siegebound/UnitCommand.h"` for the FSiegeUnitGroup complete type (cpp:38-39).
- `UpdateState` group dispatch (cpp:1105-1127): sits AFTER the Siege/Support dispatch, ABOVE the stance gate. Resolves `FindUnitGroup(CommandGroupId)` on the first player controller; live group → `UpdateStateGrouped(*Group)`; **dead id → `ClearCommandGroup()` and fall through to the stance gate THIS same tick (self-heal — never a stall)**.
- `UpdateStateStandardCommanded` (cpp:1296-1397): the HOLD branch (the GetHoldLocation/GetHoldRadius consumer) DELETED; cases reordered Defend-first so `case Hold:` sits adjacent to `case Attack: default:` and **falls through with a defensive comment** (cpp:1338-1344). The Defend and Attack case BODIES are byte-identical — only the case order moved and the Hold body vanished.
- NEW `UpdateStateGrouped` (cpp:1399-1489) — the priority ladder (see below).
- NEW `AssignCommandGroup` (cpp:1491-1503) / `ClearCommandGroup` (cpp:1505-1509) / `IsGroupCommandEligible` (cpp:1511-1523: `Standard && Blue && !bDead && !bAIFrozen` — exactly the law's four conditions).

## The 8 teardown-site swaps (CancelHoldTarget → CancelGroupPick), enumerated
1. `EndPlay` — cpp:225
2. `OnUnPossess` — cpp:447
3. `OnCancelPlacePressed` — cpp:782 (guarded branch, same shape as before)
4. `OnCmdAttackPressed` — cpp:815 (now unconditional — no-op-safe — followed by the release at 816)
5. `OnCmdDefendPressed` — cpp:842 (+ release at 843)
6. `HandleHeroDied` — cpp:879 (log-if-active preserved at 872-878)
7. `HandleMatchEnd` — cpp:1063 (formed groups deliberately SURVIVE match end — units are frozen; reset clears them)
8. `HandleMatchReset` — cpp:1163 (+ `ClearAllUnitGroups()` at 1203)

Additional internal callers (not teardown sites): the PlayerTick RMB/Esc poll (cpp:344), the stage-3 all-dead refusal (cpp:2297), and the stage-3 completion teardown (cpp:2363).

**Melee-release law:** `CancelGroupPick` releases `GroupPickHero`'s suppression BEFORE its early-out (cpp:2387-2391) — the exact ExitPlacementMode/ExitTargetingMode shape; `GroupPickHero` is its own record so no defensive cross-call can strand it.

## Stickiness / no-thrash argument (the TASK-280/282 design-against)
Per-tick decision table in `UpdateStateGrouped`:

| CurrentTarget | HOLD | AMBUSH |
|---|---|---|
| none | acquire tier-1 (attack disc) → tier-2 (position disc) → tier-3 station | same |
| dead | drop → ladder | drop → ladder |
| alive, in attack zone | KEEP (sticky) | KEEP |
| alive, in position zone only | KEEP — unless an attack-zone enemy exists ⇒ single MONOTONE upgrade | KEEP (zone test skipped) |
| alive, outside BOTH zones | DROP (the leash) → ladder → station return | KEEP (chase-to-the-kill) |

Why it cannot thrash: **acquisition runs ONLY when target-less** — there is no per-tick nearest-enemy re-pick, which is precisely what froze TASK-280/282 (the box-first goal flipped every 0.25 s and re-pathed every tick). A held target's move goal is stable, and `EnterAdvance` re-paths only on goal change / path idle (untouched). The only target switch with a live target held is the HOLD position→attack upgrade, which moves strictly UP the tier ladder (an attack-tier target is never downgraded), so tier ping-pong is impossible. The tier-3 return leg goes through `EnterAdvanceToLocation`, whose TASK-275 kite-fix (`bWasActorMove` forced re-path) is untouched and covers chase→return exactly as it did for the old hold point. Spread: golden-angle sunflower stations (radius R·√((i+0.5)/N), angle i·2.399963 rad) computed + nav-projected ONCE at the stage-3 confirm by the controller and pushed as per-unit scalars — no per-tick recompute, no point-milling.

## Byte-identical claims (QA checklist)
- `AcquireTarget`, `AcquireEnemyNearPoint`, `FindNearestEnemyCastle`, `FindOwnCastle` — untouched.
- `EnterAttack` / `EnterAdvance` / `EnterAdvanceToLocation` (**TASK-275 kite-fix**, cpp:≈1990-2043) / `EnterIdle` / `PerformAttack` — untouched.
- `UpdateStateSiege` / `UpdateStateSupport` / every miner (AMinerUnit) path / bot + all Red units (no new API is ever read for them; the group id can only be set through the controller's eligibility-gated sweep) — untouched.
- The legacy post-gate Standard body in `UpdateState` (cpp:1156-1195) — untouched.
- `UpdateStateStandardCommanded`: Defend + Attack case bodies byte-identical (Defend moved above Hold textually; behavior of a switch is order-independent).
- `SetUnitCommand` / `FOnUnitCommandChanged` / the HUD stance display / `ESiegeUnitCommand` byte layout — untouched.
- Placement + spell targeting: bodies untouched; the ONLY edits are the third-mode guard expression rename (hold flag → pick-stage test) in `EnterPlacementMode` / `EnterTargetingMode` / `ApplyCursorInputState` — same silent-ignore semantics, both directions.
- `SpawnSpellReticle`/`UpdateSpellReticle`/`DestroySpellReticle` and the shared `SpellReticleActor` — untouched; the group pick no longer shares that actor (it owns its own circles), so targeting and the pick can never cross-contaminate the reticle.
- All combat/economy/match-flow — untouched.

## Flagged decisions (deviations / choices the plan left open)
1. **`AssignCommandGroup` clears `CurrentTarget`** (SummonedUnit.cpp:1502). Not explicit in the plan; without it an AMBUSH group inheriting a stale far-away chase target would pursue something the player never circled (the leash-exemption would honor it to the kill). A fresh order re-targets from the new zones within one 0.25 s tick.
2. **`HoldPickLocation` deleted** alongside the named deletions — it was the hold-pick's private scratch, dead once the four functions went; keeping it would be an unused-member warning risk.
3. **`BroadcastCommandPrompt` private helper added** (not in the name law's list — an implementation detail so every prompt logs identically; the law's names are all present verbatim).
4. **StageTint contract for TASK-345:** the optional per-stage tint is pushed via MID at vector parameter **"StageTint"** (Select white, Position green 0.2/1/0.3, Attack red 1/0.35/0.2 — SpawnGroupCircleDecal cpp:2494-2508). Silent no-op today. If the artist does the optional M_SpellReticle param, the name must be exactly `StageTint` (stock nodes only).
5. **Stage-3 all-members-dead edge:** if every selected unit died during the flow, the final confirm refuses ("Selected units are gone"), and the WHOLE pick cancels (no group, no markers — cpp:2288-2299). The plan didn't cover this; forming an empty group would have the prune destroy the just-transferred markers within a second anyway.
6. **Completion prompt ordering:** the stage-3 confirm funnels through `CancelGroupPick` (which broadcasts empty) and then broadcasts "HOLD/AMBUSH set: N unit(s)" — the "set" line is what remains on the HUD until the next pick/release clears it.
7. **Groups survive match end** (only the in-flight PICK is torn down at HandleMatchEnd) — members are match-end frozen so nothing acts; `HandleMatchReset` does the actual release. Reading of "Play-Again resets".
8. **Spell-frozen units ARE select-eligible** — the law's eligibility is exactly `Standard + Blue + !bDead + !bAIFrozen`; a FrostNova'd unit can be circled and obeys once it thaws (its UpdateState early-out covers the frozen window).
9. **Prune cadence** is a cpp impl constant (`UnitGroupPruneInterval = 1.f`), armed once at BeginPlay — not one of the six law tunables. The stage-3 steal ALSO calls `PruneUnitGroups()` synchronously so a steal-emptied group dies at the confirm, not up to 1 s later.
10. **T/E handlers call `CancelGroupPick` unconditionally** (previously guarded on the hold flag) — it is no-op-safe by the melee-release-before-early-out construction, and unconditional matches the other teardown callers.
11. **UnitCommand.h converted to CRLF** after the rewrite (repo working-copy convention; content unaffected).

## What QA should scrutinize hardest
- The melee-release-before-early-out on EVERY cancel path (cpp:2378-2396) + all 8 swap sites above.
- The stickiness table vs the plan's ladder semantics (especially the HOLD upgrade edge: target alive, in attack zone → NO upgrade runs; only a position-tier target upgrades).
- Mutual exclusion both directions (BeginGroupPick guards at cpp:2049-2075; Enter* guards; wheel poll reachable ONLY inside the PlayerTick pick branch).
- Marker ownership/lifecycle: transfer-null-then-cancel at cpp:2350-2363 (completed flow keeps Position+Attack, kills Select); cancel-at-any-stage kills all live circles; prune/release kill group markers; `UnitGroups` is UPROPERTY so the TObjectPtrs are GC-visible.
- Kite-fix byte-identity (`EnterAdvanceToLocation`) and the untouched legacy/bot/Siege/Support/miner paths.
- Compile traps: no literal close-comment sequences inside doc comments (grep-verified clean); every `FString::Printf` format string is a TEXT literal (TCheckedFormatString-safe); no shadowing introduced (Group*, Ring*, Station* names are all fresh); complete types included (`Engine/DecalActor.h` + `Components/DecalComponent.h` + `NavigationSystem.h` pre-existing in the controller cpp; `UnitCommand.h` explicit in SummonedUnit.cpp; `TimerManager.h` added).

## Assets referenced (parallel-task contracts)
- `/Game/Input/Actions/IA_CmdAmbush` (TASK-345 creates; soft, null-safe — F inert until then).
- `/Game/Materials/M_SpellReticle` (existing; shared by the pick circles; optional `StageTint` param = TASK-345 optional).
- No new content paths otherwise; feature is fully functional for HOLD via R with zero editor assets.
