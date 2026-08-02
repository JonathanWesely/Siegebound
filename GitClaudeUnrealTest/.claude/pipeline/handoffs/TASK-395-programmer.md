# TASK-395 — [FC-1] Follow command: controller plumbing — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-08-02 · **Status on exit:** `ready-for-qa`
**Law implemented:** CONVENTIONS **"FOLLOW command + the DEFAULT-STANCE law + the MINER command rework (2026-08-02)"** §1, §2, the controller half of §4, §7 (pinned registry), §8 (tunables).
**No compile, no Git, no editor, no MCP.** Code only, as instructed.

---

## ⚖️ M8 DECLARATION DUTY — STATED EXPLICITLY

**This task adds NO replicated property and NO new replicated class, so NO new relevancy tier is declared.**
All Follow state (`DefaultFollowGroupId`, `NextFollowStationIndex`, `FollowFormationRadius`, `FollowRepathTolerance`, the `UnitGroups` entry itself) lives on `ASiegePlayerController`, which is already **TIER A — engine-owned, OWNER-SCOPED** (`bOnlyRelevantToOwner = true`), exactly like the shipped `UnitGroups`. It is one player's private command surface, never world state. In M8 P1 units are server-only, so the whole feature is **host / single-player verifiable only** until P2. `EnsureDefaultFollowGroup()` and `EnrollInDefaultFollowGroup()` both carry the D5 client-observer refusal (`!HasAuthority()` ⇒ skip, logged to `LogSiegeNet` at Verbose), matching the shipped `BeginGroupPick` / `SetUnitCommand` posture.

**`GetFirstPlayerController()` is not used anywhere in this change** — `GetFollowAnchor()` resolves off `this` and its documented reach-path is `ASiegePlayerController::FindControllerForTeam(World, Team)` (M8 TEAM LAW). Verify with `grep -n GetFirstPlayerController Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.{h,cpp}` → zero hits.

---

## Files touched (2 of the 3 I own; `MinerUnit.*` / `SummonedUnit.*` untouched)

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/UnitCommand.h` | `Follow` **APPENDED** to `ESiegeGroupCommandType`; enum + `FSiegeUnitGroup` doc comments extended. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | Public follow API + 2 tunables + 2 BP getters; `CmdFollowAction`/`CmdFollowActionAsset`; private `OnCmdFollowPressed` / `ConfirmFollowPick` / `ComputeFollowStationOffset` / `FindUnitGroupMutable`; `DefaultFollowGroupId` / `NextFollowStationIndex`. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | Impl of all of the above + the one-stage fork in `ConfirmGroupPickStage` + lifecycle resets in `PruneUnitGroups` / `ClearAllUnitGroups`. |

**NOT touched:** `SummonedUnit.{h,cpp}` (TASK-396), `MinerUnit.{h,cpp}` (TASK-397/398), `GoldNode.*`, `Castle.*`, `SiegeCheatManager.*`, `SiegeBotController.*`, any `/Game/` asset, `L_Arena`.

---

## ⚠️ HARD LINK DEPENDENCY ON TASK-396 — READ THIS FIRST

`SiegePlayerController.cpp` calls **`ASummonedUnit::IsFollowCommandEligible()`** in exactly two places (`ConfirmGroupPickStage` :2603 · `EnrollInDefaultFollowGroup` :3044). That symbol is **TASK-396's deliverable** and does **not exist yet** — I verified by grep. This is the designed state: §7 pins it as `public bool IsFollowCommandEligible() const;` on `ASummonedUnit`, and UBT compiles the module as one unit, so **TASK-395 + TASK-396 must be compiled together**. Compiling TASK-395 alone will fail with an unresolved member — *that is not a defect in this task*, and QA should not open a loop over it.

(`AMinerUnit::CanFollowHero()` has already landed from TASK-397, so the parallel lane is moving as designed.)

---

## 1. THE ENUM (spec 1)

```cpp
enum class ESiegeGroupCommandType : uint8 { Hold, Ambush, Follow };   // Follow APPENDED
```
`Hold == 0` / `Ambush == 1` **byte-preserved**. `ESiegeUnitCommand` (the STANCE enum whose byte layout `WBP_HUD`'s switch pins depend on) is **not touched** — Follow is a group order, never a stance. `FSiegeUnitGroup` is **structurally unchanged**: a Follow group is a normal group whose defaults already *are* zero radii / zero centers / null marker decals, so nothing needed adding.

## 2. THE INPUT (spec 2)

`CmdFollowActionAsset` → `/Game/Input/Actions/IA_CmdFollow.IA_CmdFollow` (ctor) → `ResolveInputAction(...TEXT("TASK-399"))` → `ETriggerEvent::Started` → `OnCmdFollowPressed()`, through the **existing** null-safe pattern, byte-parallel to `IA_CmdAmbush`. **A missing asset makes C fully inert with one log line and no crash** — and it *will* be missing at compile-review time, because the asset + the `IMC_Hero` mapping are TASK-399's editor deliverable. I did not create, reference-by-hard-pointer, or touch any asset.

**C-key freedom (for TASK-399, code-side evidence):** `grep -rn "EKeys::C\b|Crouch" Source/GitClaudeUnrealTest/Siegebound/` → **zero hits**. C is free from the C++ side. `IMC_Hero` is a binary uasset — TASK-399 must still **confirm in-editor and FLAG, never stomp**, any conflict.

## 3. THE ONE-STAGE PICK (spec 3) — built as a short path, not a crippled long one

`OnCmdFollowPressed()` → `BeginGroupPick(ESiegeGroupCommandType::Follow)`. That reuses **all** the shipped pick machinery (cursor posture, `GroupPickHero` melee suppression, mutual exclusion with placement + targeting, the match-ended guard, the D5 client lockout, the polled wheel, the polled RMB/Esc, the trace) and enters at `EGroupPickStage::Select`.

**The fork is one branch inside `ConfirmGroupPickStage`'s `Select` case** (`SiegePlayerController.cpp:2621-2629`): after the (shared) sweep and the (shared) empty-circle refuse-and-stay, Follow calls `ConfirmFollowPick()` and **returns** instead of dropping the circle and opening stage 2. There is **no second/third stage to disable** and no cloned flow.

- **Wheel:** unchanged `ApplyGroupPickWheel` — `GroupRadiusWheelStep` 100, clamp [200, 5000], opens at `GroupSelectRadiusDefault` **1200**. Still **polled**, still **no new InputAction**, still inert outside a pick.
- **Cancel:** unchanged — the polled RMB/Esc in `PlayerTick` and the `IA_CancelPlace` binding both land on `CancelGroupPick()`.
- **The select circle is DESTROYED at confirm.** `ConfirmFollowPick` funnels through `CancelGroupPick()` and — unlike the stage-3 confirm — **nulls nothing first**, so the transient decal dies. A Follow group owns no ground, so `PositionMarkerDecal` / `AttackMarkerDecal` stay null and no stale marker is left lying in the world.
- **Structural tripwire:** `ConfirmGroupPickStage` :2568 logs `Error` + tears down if a Follow pick is ever seen in a zone stage. It is unreachable by construction; it exists so a future refactor cannot silently build a follow group carrying a bogus zone.
- **Eligibility splits at the sweep** (:2603): zone types keep `IsGroupCommandEligible()`, Follow uses `IsFollowCommandEligible()`. Hold/Ambush behaviour through that loop is unchanged (I only hoisted the `IsValid` check into a `continue`).

### `CancelGroupPick()` covers Follow at all existing teardown sites — with **zero** new sites
`CancelGroupPick` is **stage-agnostic**: it resets `GroupPickStage` to `None` whatever it held and destroys every circle the flow still owns. So the nine shipped teardown callers already cover a Follow pick and I added none:
`EndPlay` :303 · `PlayerTick` polled RMB/Esc :422 · `OnUnPossess` :525 · `OnCancelPlacePressed` :886 · `OnCmdAttackPressed` :931 · `OnCmdDefendPressed` :958 · `HandleHeroDied` :995 · `HandleMatchEnd` :1179 · `HandleMatchReset` :1316. (Line numbers are pre-edit; the melee-release-before-early-out law is untouched.)

## 4. THE DEFAULT FOLLOW GROUP (spec 4) — and the seam TASK-396 plugs into

**THE SEAM, STATED EXPLICITLY (the orchestrator asked for this):**
> `void ASiegePlayerController::EnrollInDefaultFollowGroup(ASummonedUnit* Unit)` is the **single** entry point by which a unit joins Follow. **TASK-396 owns exactly one call to it** — unconditionally, from `ASummonedUnit::BeginPlay`, on the controller resolved via `FindControllerForTeam(GetWorld(), Team)`, null-controller ⇒ skip silently. It owns **nothing else** on the controller side: it does not create groups, does not compute stations, does not touch `DefaultFollowGroupId`. Everything downstream of that one call is mine and is already written.

Inside, in order: `IsValid` → `IsFollowCommandEligible()` gate → `EnsureDefaultFollowGroup()` (lazy create, returns `INDEX_NONE` on non-authority) → **idempotence check** (already a member ⇒ return, keeping its existing station) → **STEAL** out of every other group (the shipped re-selection law) → append → `ComputeFollowStationOffset(NextFollowStationIndex++)` → `Unit->AssignCommandGroup(...)` (which also drops the unit's `CurrentTarget` — exactly right on the way into a never-attack body) → `PruneUnitGroups()` to reap any group the steal emptied.

**Every refusal is silent and degrades to today's behaviour** — never a crash, never a stall. That is load-bearing: this runs on *every* unit spawn.

**EXACTLY ONE FOLLOW GROUP PER CONTROLLER.** Pressing C never creates a second one — `ConfirmFollowPick` just enrols the circled units into the same group. That is what makes "the spawn default" and "the C command" the same object.

**The station formation.** Golden-angle sunflower inside `FollowFormationRadius` (**900** uu, `EditDefaultsOnly`, flagged), computed **once** at enroll, pushed as a per-unit scalar `FVector` offset (no arrays on units):
`radius = R·sqrt(((i mod 12)+0.5)/12)`, `angle = i·GoldenAngleRadians`.
The **radius wraps** through 12 nominal slots so any squad size stays inside the ring; the **angle never wraps**, so every live follower keeps its own bearing.
**Deliberately NOT nav-projected** — the one deviation from the stage-3 station recipe, and it is correct: this is an offset from a point that *moves*, so a projection taken at enroll against a stale hero position would be meaningless. The unit's `EnterAdvanceToLocation` already passes `bProjectDestinationToNavigation = true`.

**🔍 QA, SCRUTINISE THIS ONE — a deliberate, documented deviation from the spec's literal wording.** The spec says stations are assigned "by member index". I used a **monotonic per-group ordinal** (`NextFollowStationIndex`, reset to 0 whenever the group is re-created) instead of the raw `Members` array index. Reason: `PruneUnitGroups` **compacts** `Members`, so array indices are **recycled** — and because Follow is the *spawn default*, this group churns constantly (every unit spawns in, fights, dies). Using the array index would make **two living followers sharing one station the common case, not an edge**. The ordinal is still deterministic, still O(1), still one `int32`, and preserves the property the spec actually wants (a stable, distinct, once-computed station per unit). If QA rules the literal reading must ship, it is a one-line change at `SiegePlayerController.cpp:3097`.

## 5. THE ANCHOR (spec 5)

```cpp
AActor* ASiegePlayerController::GetFollowAnchor() const;   // SiegePlayerController.cpp:3108
```
Returns `GetPawn()` — **re-read on every call, NEVER cached** — or `nullptr` when the pawn is missing/pending-kill **or** `AHeroCharacter::IsDead()`. Live resolution is precisely what makes a **replacement pawn work for free** after a respawn: the game mode's respawn path may hand back a different actor, and a cached pointer would escort a corpse forever.

**Hero-death ruling (manager, ruling 8) is honoured at this seam:** dead/absent hero ⇒ `nullptr`, and `nullptr`'s documented contract (written into the function's doc comment so TASK-396 cannot miss it) is **hold position** — `EnterIdle()`, no target, no march, no attack — resuming the instant a live pawn resolves. Rejected alternatives are recorded in the comment so nobody re-derives them: marching to the corpse; falling back to Defend (that would make followers fight, breaking ruling (iii)).

**`GetFirstPlayerController()` is BANNED and unused.** Callers reach this controller through `FindControllerForTeam`.

---

## ⚠️ 6. THE ANTI-REPATH REQUIREMENT — the threshold, and why it is that number

**`ASiegePlayerController::FollowRepathTolerance = 250.f` uu** (`EditDefaultsOnly`, `BlueprintReadOnly`, flagged for Jonathan's feel pass).

**The rule (written into the tunable's doc comment, which is TASK-396's binding contract):** the follow body re-issues `EnterAdvanceToLocation` **only when the recomputed station has drifted more than 250 uu from the goal it last issued** — the comparison is against the **last issued goal**, *not* against the hero's last position (comparing hero positions would reset on every re-path and can oscillate).

**Why this is the trap and not polish — the measurement, not a guess:**
`EnterAdvanceToLocation`'s own internal guard is `CurrentMoveGoalLocation.Equals(Point, 1.f)` (`SummonedUnit.cpp:2261`) — a **1 uu** tolerance. A station recomputed from a *walking* hero clears that every single 0.25 s state tick, so an ungated follow body re-paths **unconditionally, every tick**. That is exactly the re-path mill that produced **TASK-280** ("units freeze just past midfield") and **TASK-282** ("halt just short of the castle"): each `MoveToLocation` restarts path-following before the previous request produced meaningful motion, and the unit stutters in place. **An unconditional re-path is a QA FAIL and I did not want this discovered at PIE.**

**Why 250 specifically — four independent constraints, all satisfied:**

| Constraint | Number | Result at 250 uu |
|---|---|---|
| Hero displacement per state tick | walk 500 uu/s × 0.25 s = **125 uu**; sprint 750 uu/s × 0.25 s = **187.5 uu** (`HeroCharacter.h:406/410`) | Re-path fires after **2 ticks** walking (250 uu) and after **2 ticks** sprinting (375 uu). Worst-case cadence is **0.5 s**, i.e. **half** the tick rate — every move request gets ≥2 ticks of uninterrupted path-following. Standing hero ⇒ **never** re-paths. |
| Must be wider than the arrival disc | `HoldArrivalTolerance` = **150 uu** (`SummonedUnit.cpp:87`) | 250 > 150 (1.67×), so a follower that has *arrived* can never be shaken back into a re-path by hero jitter — the two bands don't fight. |
| Must not visibly wreck the formation | `FollowFormationRadius` = **900 uu** | Worst-case station error is 250 uu inside a 900 uu ring (**0.28×**) — the sunflower spread still reads as a formation. |
| Must not make followers lag absurdly | move acceptance `StructureMoveAcceptanceRadius` = **50 uu** | Steady-state trail ≈ tolerance + arrival + accept + path lag ≈ **~450 uu**, under half the formation radius. Escort feel, not a conga line. |

**Tuning direction if it feels wrong at playtest:** *lower* than ~180 uu re-enters the every-tick regime at sprint and risks the mill returning — that is the floor. *Higher* than ~400 uu starts to read as followers lagging and cutting corners. 250 sits deliberately in the middle and is a flagged tunable, so Jonathan can move it without a code change.

**Ownership note:** the *gate* is unit-side code (`UpdateStateFollow`, TASK-396). What TASK-395 ships is the tunable, its 250 uu value, and the contract text — QA should verify TASK-396 actually reads it and does not re-path unconditionally.

## 7. LIFECYCLE (spec 6) — the anti-leak invariant

`DefaultFollowGroupId` is reset to `INDEX_NONE` at **every** point its group can die, and `EnsureDefaultFollowGroup` re-validates against the live array as a second line of defence:

| Path | What happens |
|---|---|
| **T / E** (`OnCmdAttackPressed` / `OnCmdDefendPressed` → `ClearAllUnitGroups`) | Follow group destroyed with everything else (the release law, now load-bearing: **T is the "everyone attack" button**). Id + station ordinal reset **above** the empty-array early-out, so the invariant holds on every path. Re-creates lazily on the next spawn or C press. |
| **Play Again** (`HandleMatchReset` → `ClearAllUnitGroups`) | Same. |
| **≤1 s prune** (`PruneUnitGroups`) | An **empty** follow group is legitimately reaped (last follower died). The id is reset in the same block, with a distinguishing log line. **This is the leak the spec warned about and it is closed** — ids are never reused, so a stale id would leave the spawn default permanently pointed at nothing. |
| **`EnsureDefaultFollowGroup`** | Validates `DefaultFollowGroupId` against `FindUnitGroup` before returning it — even a *missed* reset self-heals into a fresh group. |
| Units holding a dead id | Self-heal on their next state tick via the shipped null-group path (`FindUnitGroup` ⇒ `nullptr` ⇒ `ClearCommandGroup`). Unchanged. |

**Prompts:** `OnCommandPromptChanged` fires with `"FOLLOW: circle the units to follow you — scroll to resize, LMB confirm, RMB/Esc cancel"` at the pick stage and `"FOLLOW set: N unit(s)"` at confirm (broadcast *after* `CancelGroupPick`'s empty broadcast, the shipped stage-3 ordering, so the completion text is what remains on the HUD). Both also **LOG** via `BroadcastCommandPrompt`, so the feature ships with **no WBP edit** — `WBP_HUD` already binds this delegate additively (TASK-345).

---

## Acceptance criteria — self-check

| Criterion | Status |
|---|---|
| With no C press and no enrollment, every shipped behavior is byte-identical | ✅ Every addition is behind `GroupPickType == Follow`, a new call site, or a lifecycle reset that is a no-op while `DefaultFollowGroupId == INDEX_NONE`. The Hold/Ambush 3-stage path, the wheel, the steal, the sunflower, the prune's destroy semantics and `ClearAllUnitGroups`' member-clearing are unchanged. |
| The pick cannot reach stage 2/3 | ✅ `Select` case returns for Follow (:2625) + an `Error`-logging tripwire at :2568. |
| A missing `IA_CmdFollow` leaves C inert | ✅ Shipped `ResolveInputAction` null-safe pattern; binding skipped, one log line. |
| No `GetFirstPlayerController` | ✅ Zero occurrences in either file. |
| No new replicated property | ✅ **Stated explicitly at the top of this document.** |
| ONE circle only, destroyed at confirm | ✅ `ConfirmFollowPick` nulls nothing before `CancelGroupPick`, so the select decal is destroyed; a Follow group's marker decals stay null. |
| `CancelGroupPick` stays the ONE teardown call, covering Follow at all sites | ✅ Stage-agnostic; 9 shipped sites listed above; **no new teardown site added**. |
| Anti-repath threshold stated with reasoning | ✅ §6 above. |
| Hero anchor resolved LIVE, never cached | ✅ `GetFollowAnchor()` re-reads `GetPawn()` per call. |

**Compile traps checked:** no literal `*/` inside any doc comment I wrote · every `FString::Printf` / `UE_LOG` format string is a string literal (`TCheckedFormatString`, the TASK-268 C7595 lesson) — the only runtime-varying pieces are `%s` *arguments* · no shadowing of inherited reflected members (`FollowRepathTolerance`, `FollowFormationRadius`, `DefaultFollowGroupId`, `NextFollowStationIndex`, `CmdFollowAction`, `CmdFollowActionAsset` are all new names, grepped) · complete-type include law satisfied by the existing includes (`HeroCharacter.h`, `SummonedUnit.h`, `SiegeSessionSubsystem.h` for `LogSiegeNet`) — **I added no include**.

---

## 🔍 What QA should scrutinise hardest

1. **The station-ordinal deviation (§4).** The one place I knowingly departed from the spec's literal wording. Rationale + the one-line revert location are given above.
2. **The two tunables are `public` UPROPERTYs, not `protected`** — a deliberate break with the file's house style. Reason: §7's pinned registry contains **no accessor** for them while §8 names them `ASiegePlayerController::FollowRepathTolerance` / `FollowFormationRadius`, so the *member itself* is the cross-task seam TASK-396/398 must read off a resolved controller. I additionally shipped `GetFollowRepathTolerance()` / `GetFollowFormationRadius()` `BlueprintPure` mirrors so **either access shape links**, because the parallel tasks are being written blind to my choice. If QA prefers protected+getter, the members can move once TASK-396's call shape is known — but not before, or the batch may fail to link.
3. **`OnCmdFollowPressed` and `DefaultFollowGroupId` are `private`**, unlike the `protected` sibling `OnCmd*Pressed` handlers. That is **per §7's pin** ("access levels are part of the pin"), not an oversight. The binding is taken inside `SetupInputComponent`, so private costs nothing.
4. **`FindUnitGroupMutable` uses `const_cast`** on `FindUnitGroup`'s result. Deliberate, to keep exactly one search implementation and one `INDEX_NONE` early-out. The pointee is genuinely non-const (it aliases this controller's own `UnitGroups`), so it is well-defined — but it is a `const_cast`, so it deserves a look.
5. **`PruneUnitGroups()` is called at the end of every enroll**, mirroring the stage-3 steal rule. It can `RemoveAt` on `UnitGroups`, which **dangles the local `FollowGroup` pointer** — I noted this inline and the pointer is never touched afterwards. Worth a second pair of eyes.
6. **`ConfirmFollowPick` reads the pick scratch before `CancelGroupPick` resets it** (an ordering dependency, flagged inline). In particular `GroupPickRadius` is logged *before* the teardown.
7. **The `%s` runtime-ternary in the prune log.** `bWasDefaultFollowGroup ? TEXT("…") : TEXT(".")` — two literals of different lengths; the conditional decays to `const TCHAR*`. This is the same shape the shipped `BeginGroupPick` prompt used, so it is precedented, but format-string checking is exactly where UE 5.8 bites.
8. **Whether TASK-396 actually consumes `FollowRepathTolerance`** rather than re-declaring 250 locally, and whether it re-paths unconditionally. That is the single highest-risk item in the whole batch and it lives in the *other* task.

## 🚩 Flagged for the manager / Jonathan (recorded, not decided by me)

- **The stage-1 select prompt for FOLLOW deliberately reads differently** from Hold/Ambush ("circle the units to follow you") because Follow's circle *is* the whole command and the shared wording promises a second stage that will never open. Wording is Jonathan's to change; it is a string, not a mechanic.
- **`FollowFormationSlots = 12`** is an anonymous-namespace impl constant (the `UnitGroupPruneInterval` / `GoldenAngleRadians` class), **not** a new tunable — §8 pins only two. It changes packing density only, never correctness. If Jonathan wants the escort ring denser/sparser at a fixed 900 uu, this is the dial, and it can be promoted to a UPROPERTY in a one-line change.
- **`ConfirmFollowPick` counts already-following units in its "FOLLOW set: N" readout.** Circling 5 units of which 3 already follow reports **5**, not 2 — "5 units are following you" is the honest readout of the command issued. Flagged in case Jonathan expects the delta instead.
