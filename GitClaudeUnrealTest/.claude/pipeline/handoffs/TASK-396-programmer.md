# TASK-396 — [FC-2] Follow command: the unit-side body, the eligibility split, the spawn auto-enroll — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-08-02 · **Status on exit:** `ready-for-qa`
**Law implemented:** CONVENTIONS **"FOLLOW command + the DEFAULT-STANCE law + the MINER command rework (2026-08-02)"** §2 (unit half), §3, §4 (unit half), §7 (pinned registry).
**Files touched: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` + `SummonedUnit.cpp` — AND NOTHING ELSE.**
**No compile, no Git, no editor, no MCP.** Code only, as instructed.

---

## ⚖️ M8 DECLARATION DUTY — STATED EXPLICITLY

**This task adds NO replicated property, NO new replicated class and NO new `UPROPERTY` of any kind, so NO new relevancy tier is declared.** Mechanically verifiable:
`git diff -U0 -- Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h | grep -E "^\+.*(UPROPERTY|Replicated|DOREPLIFETIME)"` → **zero hits.**

The two new members (`LastFollowGoalLocation`, `bHasFollowGoalLocation`) are plain non-reflected C++ scalars — deliberately NOT `UPROPERTY`, because they are per-tick pathing scratch that nothing outside `UpdateStateFollow` reads. `ASummonedUnit`'s replication posture is byte-identical to before. Units are server-only in M8 P1, so the whole feature is **host / single-player verifiable only** until P2.

**`GetFirstPlayerController()` is not called anywhere in this change.** `grep -n GetFirstPlayerController SummonedUnit.{h,cpp}` returns two hits and **both are comments naming it as BANNED**. Every controller resolve goes through `ASiegePlayerController::FindControllerForTeam(World, Team)` (M8 TEAM LAW).

---

## ✅ THE LINK-BREAKER — VERIFIED, AS THE DISPATCH DEMANDED

`SummonedUnit.h` has **exactly three access specifiers**, before and after my edits:

| | before | after |
|---|---|---|
| `public:` | 121 | **121** |
| `protected:` | 484 | **574** |
| `private:` | 745 | **862** |

**I introduced NO new access specifier anywhere, and none above `:482`.** TASK-379's two getters moved from `:467`/`:482` to **`:557`/`:572`** — still inside the SAME, ORIGINAL `public:` block that starts at `:121`, because everything I added above them is declarations and comments, not a specifier. Every outside call site links exactly as it did.

Reproduce: `grep -n "^public:\|^protected:\|^private:" Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` → `121 / 574 / 862`, then check `GetPermanentDamageBonusPerStack` (557) and `GetMaxPermanentDamageStacks` (572) both fall in `[121, 574)`.

My new public virtuals sit at `:404`/`:413`/`:423`/`:453` — all inside that same block. `UpdateStateFollow` sits at `:860`, inside `protected:` **as §7 pins it**.

---

## §7 PINNED REGISTRY — CONSUMED CHARACTER-FOR-CHARACTER

| §7 pin | Shipped at | Access |
|---|---|---|
| `virtual bool CanFollowHero() const;` | `SummonedUnit.h:404` | public ✅ |
| `virtual bool CanTakeZoneOrders() const;` | `SummonedUnit.h:413` | public ✅ |
| `bool IsFollowCommandEligible() const;` | `SummonedUnit.h:423` | public ✅ |
| `bool IsGroupCommandEligible() const;` | `SummonedUnit.h:453` | public ✅ (**name + signature KEPT**) |
| `void UpdateStateFollow(const FSiegeUnitGroup& Group);` | `SummonedUnit.h:860` | **protected** ✅ |

I consumed (never re-declared, never "improved") `ASiegePlayerController::{GetFollowAnchor, EnrollInDefaultFollowGroup, FindControllerForTeam, FindUnitGroup, FollowRepathTolerance}` and `ESiegeGroupCommandType::Follow`. Declarations and definitions are 7-for-7 in parity.

---

## 1. THE ELIGIBILITY SPLIT (spec 1, §3 table)

```cpp
bool ASummonedUnit::CanFollowHero()     const { return Profile == Standard || Profile == Support; }
bool ASummonedUnit::CanTakeZoneOrders() const { return Profile == Standard; }
bool ASummonedUnit::IsFollowCommandEligible() const { return CanFollowHero()     && Blue && !bDead && !bAIFrozen; }
bool ASummonedUnit::IsGroupCommandEligible()  const { return CanTakeZoneOrders() && Blue && !bDead && !bAIFrozen; }
```

`IsGroupCommandEligible()`'s **only executable change** is `Profile == ECardProfile::Standard` → `CanTakeZoneOrders()`. For every base-class unit those are the same expression, so the R/F sweep is unchanged for the entire shipped fleet; the one behavioural delta is the Miner, which is TASK-397's override doing exactly what §3 asks. **Its doc comment is rewritten** to say ZONE ORDERS ONLY and to point at `IsFollowCommandEligible()` for C — the spec's explicit requirement, since the name no longer covers Follow.

The §3 table is reproduced as a comment block above the four functions so the next reader does not have to open CONVENTIONS.

---

## 2. THE DISPATCH HOIST (spec 2) — the one structural change

`UpdateState` (`SummonedUnit.cpp:1259`), inserted **after `TrackChargeMovement()` and before the profile dispatch**:

```cpp
if (CommandGroupId != INDEX_NONE)
{
    ... FindControllerForTeam → FindUnitGroup ...
    if (FollowGroup)
    {
        if (FollowGroup->Type == ESiegeGroupCommandType::Follow) { UpdateStateFollow(*FollowGroup); return; }
        // a live HOLD/AMBUSH group: leave it entirely to the shipped dispatch below
    }
    else { ClearCommandGroup(); }
}
```

- **Why hoisted:** Support and Siege `return` out of the profile dispatch, so the shipped group dispatch is unreachable for them. Without the hoist a following **Cleric** would never dispatch. This is the reason §4 calls the dispatch point load-bearing.
- **Hold/Ambush dispatch stays exactly where it is.** A zone-ordered unit falls straight through this block and is run by the shipped dispatch, unchanged.
- **The dead-id self-heal is duplicated deliberately, not accidentally.** A Support unit can now hold a group id, and the shipped block below is unreachable for it — so this is the only self-heal such a unit ever gets. For a Standard unit the net effect is identical (clear here ⇒ the block below skips ⇒ fall through to the stance gate in the same tick). **Strict improvement, zero regression:** today a Siege/Support unit with a dead id would leak it forever.
- **Cost:** one extra `FindControllerForTeam` + `FindUnitGroup` per tick **only** for a unit in a live zone group. `FindControllerForTeam` is allocation-free, logs on no path, and iterates the PC list (1 entry in standalone).

---

## 3. `UpdateStateFollow` (spec 3) — `SummonedUnit.cpp:1758`

Order of operations: **type tripwire → force `CurrentTarget = nullptr` → (Support only) heal targeting → LIVE anchor resolve → hero-death hold → station → arrival test → anti-repath band → `EnterAdvanceToLocation`.**

### THE NEVER-ATTACK GUARANTEE — cite the code path (ACCEPTANCE)

A following unit cannot enter Attack, for three independent reasons:

1. **`CurrentTarget = nullptr` is forced on every tick** (`:1789`), before anything else can run.
2. **The function calls NONE of `AcquireTarget` / `AcquireEnemyNearPoint` / `EnterAttack`.** The only two calls that can leave it are `EnterAdvanceToLocation` and `EnterIdle`, and **both clear `AttackTimerHandle` + `StopAttackLunge()` on the way out of Attack** (`EnterAdvanceToLocation` `SummonedUnit.cpp:2411`, `EnterIdle` `:2466`). A unit that was mid-fight when it was circled therefore stands down on its FIRST follow tick.
3. **Belt for the ≤0.25 s enroll window:** `PerformAttack` (`:2484`) re-validates `CurrentTarget` and returns on a dead/null target — and `AssignCommandGroup` already nulled it at enroll. So even a timer still in flight lands no hit.

**This is a per-BODY seal and deliberately NOT `CanEverAttack()`** (§4): a following Footman must become a normal attacker the instant its group is released, which a per-CLASS seal cannot express.

### The tripwire

The `Group` parameter carries no zones, so the only thing read off it is `Group.Type != Follow ⇒ Error log + return`. Unreachable by construction (the one caller tests the type); it exists so a future refactor cannot silently route a zone group into the never-attack body. It also means the pinned parameter is genuinely used rather than warning-suppressed.

---

## 4. ⚠️ ANTI-REPATH (spec 4, ruling 10) — the highest-risk item in the batch, and how I gated it

```cpp
const float RepathTolerance = PC->FollowRepathTolerance;          // READ, never re-declared
const AAIController* const AI = GetAIController();
const bool bStillWalking = AI && AI->GetMoveStatus() != EPathFollowingStatus::Idle;
if (bHasFollowGoalLocation && bStillWalking
    && FVector::DistSquared2D(Station, LastFollowGoalLocation) <= FMath::Square(RepathTolerance))
{
    return;   // inside the band and still walking — let the in-flight path run
}
LastFollowGoalLocation = Station;
bHasFollowGoalLocation = true;
EnterAdvanceToLocation(Station);
```

- **I consume `ASiegePlayerController::FollowRepathTolerance` off the resolved controller. I did not re-declare 250 locally** — TASK-395's handoff §6 named this as the single thing QA should check, and it is honoured.
- **The comparison is against the LAST ISSUED GOAL** (`LastFollowGoalLocation`), not against the hero's last position — exactly as TASK-395's contract text specifies.
- **I added a SEPARATE latch rather than reusing `CurrentMoveGoalLocation`.** That shipped field is also written by the HOLD/AMBUSH tier-3 body and by `UpdateStateStandardCommanded`, so reusing it would compare against another order's point. The latch is reset by `AssignCommandGroup`, by `ClearCommandGroup`, and every time the body idles — so a new order, a re-station and a hero respawn all always re-path once.

**Measured cadence at the shipped numbers** (hero walk 500 uu/s, sprint 750 uu/s, tick 0.25 s):

| Hero state | Station drift / tick | Ticks between re-paths | Wall clock |
|---|---|---|---|
| stationary | 0 | never (arrival branch idles the unit) | — |
| walking | 125 uu | 3 (skip at 125, skip at 250, fire at 375) | 0.75 s |
| sprinting | 187.5 uu | 3 (skip at 187.5, fire at 375) | 0.75 s |

Every issued move gets **at least two full ticks** of uninterrupted path-following. That is the TASK-280/282 mill closed by construction, not by tuning.

### 🔍 THE `bStillWalking` TERM — QA, SCRUTINISE THIS ONE

This is my one addition beyond the letter of the spec, and it is load-bearing. Without it, a follower that **completes or fails** its leg while the hero has drifted less than the band would be stuck: the drift test says "don't re-path", the arrival test says "not arrived", and nothing moves — a permanent stall up to one full band (250 uu) short of station, on a stationary hero. Adding "…and path following is not Idle" closes it.

**It cannot re-open the mill.** The mill is *restarting an in-flight request*; this term only ever fires when there is no in-flight request. The tick after it fires, the unit is moving and inside the band, so that request runs to completion. Worst case it costs one re-issue per completed leg — the same discipline the shipped `EnterAdvance`/`EnterAdvanceToLocation` already use (`|| GetMoveStatus() == Idle`).

---

## 5. HERO DEATH (spec 5, ruling 8)

Anchor resolved **LIVE every tick, never cached** — `FindControllerForTeam` → `GetFollowAnchor()`, which itself re-reads `GetPawn()` and returns nullptr on a missing/pending-kill/`IsDead()` hero. On nullptr: **drop the goal latch, `EnterIdle()`, return.** No target, no march, no attack. Resume is automatic on the next tick that resolves a live pawn — **including a brand-new post-respawn pawn actor**, precisely because nothing is cached — and the dropped latch guarantees that resume tick issues its own fresh move.

A following **Cleric keeps healing while the hero is dead** (the heal-targeting call sits above the anchor resolve). That is correct: it holds position and mends.

---

## 6. SPAWN AUTO-ENROLL (spec 6) — placement, and the deviation I am flagging

**One call, on the UNIT: `TryAutoEnrollInFollowGroup()` (`SummonedUnit.cpp:1211`), invoked from `LoadStatsAndStart()` (`:1194`) — after `bStatsLoaded = true`, before the synchronous `UpdateState()`.**

**🔍 DELIBERATE, DOCUMENTED DEVIATION from the spec's literal "at `BeginPlay`":** it is in `LoadStatsAndStart`, which is the tail of `BeginPlay`. Four measured reasons, all written at the call site:

1. **`Profile` is bound inside `LoadStatsAndStart`, ~50 lines above the insertion point.** `ASummonedUnit::Profile`'s constructor default is **`Standard`** (`SummonedUnit.h:1365`) — so at the tail of `BeginPlay` on the plain-SpawnActor path an **Ogre would enroll before its row said Siege.** This is not a hypothetical; it is why the literal placement is wrong.
2. `bStatsLoaded` is true here, so eligibility is answered from real card data.
3. It covers **both** spawn shapes from ONE insertion point — `BeginPlay` (the deferred path every shipped spawner uses) and the late `InitUnit` bind.
4. It runs before the synchronous `UpdateState()`, so a follower's **first** decision is already the follow body — no wasted castle-bound Advance on the spawn frame.

Gates, in order: `bDead || !bStatsLoaded || bAIFrozen` → `IsFollowCommandEligible()` → `FindControllerForTeam` non-null → `PC->EnrollInDefaultFollowGroup(this)`. **Every refusal is silent and degrades to today's behaviour.** The unit creates no group, computes no station and never touches `DefaultFollowGroupId` — exactly the seam TASK-395 §4 defined.

---

## 7. 🚩 THE MINER SPAWN-DEFAULT — WHAT I DID, FOR TASK-398'S AUTHOR TO VERIFY

**The dispatch told me my auto-enroll must not enroll miners at spawn. It does not — and I am NOT the one who stops it, on purpose.**

**TASK-398 had already landed in the working tree when I got to this point** (`MinerUnit.{h,cpp}`, `Castle.{h,cpp}`, `GoldNode.{h,cpp}` all dirty). It delivers the ruling entirely inside its own file:

- `AMinerUnit::bFollowOnSpawn` — `EditDefaultsOnly`, default `false`. **This is the one line/BP toggle Jonathan flips at TASK-402.**
- `AMinerUnit::bSpawnFollowEnrollWindowClosed` — set `true` on the statement immediately after `Super::BeginPlay()` returns.
- `AMinerUnit::CanFollowHero() { return bFollowOnSpawn || bSpawnFollowEnrollWindowClosed; }`

So a miner is **not follow-eligible for exactly the duration of `Super::BeginPlay()`** — which is where my enroll lives — and my `IsFollowCommandEligible()` gate refuses it. The miner joins no group **and no `Members` array**, which is the part that matters (a stale `Members` entry would make `EnrollInDefaultFollowGroup`'s `Contains()` early-out swallow every future C press and make Follow unreachable for miners forever).

### ⚠️ I DRAFTED A BASE-SIDE CARVE-OUT AND THEN DELETED IT — this is the most important thing in this handoff

Before I found TASK-398's tree state I had written a second, base-side lever: `virtual bool ShouldSpawnFollowing() const { return Profile != ECardProfile::None; }`, ANDed into the enroll gate. It worked, and it was wrong:

> **With `bFollowOnSpawn = true`, my gate would still have refused. Jonathan's designated flip would have silently done nothing.**

Two levers for one decision, one of which silently wins, is exactly the class of defect the pinned registry exists to prevent. It is fully removed (`grep -rn ShouldSpawnFollowing Source/` → zero hits) and replaced by a comment block at the site it would have occupied, stating the contract TASK-398 depends on: **`TryAutoEnrollInFollowGroup()` gates on `IsFollowCommandEligible()` and NOTHING ELSE, and runs inside `ASummonedUnit::BeginPlay`'s synchronous call stack.**

### KNOWN RESIDUAL, stated rather than papered over

On the **late-`InitUnit`** path (plain `SpawnActor` then `InitUnit`), `LoadStatsAndStart` runs after `AMinerUnit::BeginPlay` already closed the window — so a miner spawned that way *would* auto-enroll. **Unreachable today:** `grep -rn "SpawnActor<ASummonedUnit\|SpawnActor<AMinerUnit\|SpawnActor<ASorcererUnit" Source/` returns **zero** — every shipped spawner (`SpawnUnitSwarm`, `ABarracks`, `SummonTestUnit`) uses `SpawnActorDeferred` + `InitUnit` + `FinishSpawning`. If such a spawner is ever added, **the fix belongs in `AMinerUnit` (widen the window), not in a second base gate** — and TASK-398's `AMinerUnit::BeginPlay` Warning tripwire is the detector, though note it runs *before* a late `InitUnit` and so would not fire on that specific path.

---

## 8. A FOLLOWING CLERIC STILL HEALS (spec 7, ruling 9) — mechanism and cost

**Mechanism:** the heal-**targeting** half of `UpdateStateSupport` is extracted **verbatim** into `ASummonedUnit::UpdateSupportHealTargeting()` (private, `:2147`) and called from `UpdateStateFollow` when `Profile == Support`. The heal itself is untouched — it runs on its own `HealTimerHandle` via `PerformHeal`, so a following Cleric mends at **exactly** the shipped rate, with the shipped range re-validation and the shipped no-overheal clamp.

**`UpdateStateSupport` is behaviourally byte-identical.** The four moved statements are in the same order; the caller now reads `ASummonedUnit* HealTarget = UpdateSupportHealTargeting();`. `git diff -U0` on the .cpp removes exactly those statements and nothing else executable outside the two functions I own.

**The ONE thing that deliberately did NOT move: `FaceTarget(HealTarget)`.** It stayed at the `UpdateStateSupport` call site and is **not** called in the follow body. Reason: a Support Cleric walks *at* its patient, so facing it agrees with movement; a *following* Cleric walks to its station and its patient may be behind it, so snapping the yaw every 0.25 s would fight the movement component's orientation. **Cost of the whole mechanism:** one `FindNearestDamagedFriendly()` per state tick per following Cleric — the same query at the same cadence a non-following Cleric already runs. **Zero** additional cost for any non-Support unit (the call is behind a `Profile == Support` branch).

---

## 9. What I added for TASK-398 as the file's last owner — and what I reverted

- ✅ **KEPT: `FVector GetGroupStationOffset() const { return GroupStationOffset; }` (public, `:372`).** This is the "one base one-liner" TASK-397 deferred (`MinerUnit.cpp` insertion point #1) and it is quoted **character-for-character** in `MinerUnit.h`'s own comment as the recorded fix for `AMinerUnit::ResolveStationOffset`'s flagged deviation. **It is currently UNUSED, on purpose** — TASK-398 re-derives its own slot because the getter did not exist when it was written. `SummonedUnit.h` is the only file it could live in and this is the last task that owns it, so it lands now; consuming it is a one-line `MinerUnit.cpp` follow-up that retires that flag.
- ❌ **REVERTED: promoting `FindOwnCastle()` to public.** I made it public for the miner's DEFEND body, then reverted when I read that TASK-398 shipped `ACastle::FindNearestCastleForTeam` instead. No outside caller needs it, and unused public API is worse than a later edit. It is documented in place as still-private with the reason.
- ❌ **NOT TAKEN, deliberately: the `Castle.h`-flagged consolidation** ("`ASummonedUnit::FindOwnCastle` could delegate here once that file is free"). The file *is* free now, and I still refused: the two use different metrics (bounds-aware `GetDistanceToTarget` vs squared 2D), and `FindOwnCastle`'s only caller is `UpdateStateStandardCommanded`, which is in **my byte-identical regression set**. Not a bugfix to slip into a Follow task. Recorded in the function's doc comment for whoever takes that flag.

**📌 FOR THE MANAGER — two names to pin in CONVENTIONS §7** (I did not edit CONVENTIONS; naming law is the manager's): `FVector ASummonedUnit::GetGroupStationOffset() const` (public) — the retirement path for `MinerUnit.h`'s flagged station-offset deviation.

---

## 10. REGRESSION LAW — the byte-identical set, checked

`git diff -U0 -- SummonedUnit.cpp | grep -E "^-[^-]"` removes **13 lines total**, and every one is accounted for: 6 comment lines + 1 predicate line in `IsGroupCommandEligible` (the intended narrowing), and 6 lines of `UpdateStateSupport`'s heal-targeting (moved verbatim into the helper). **Nothing else executable was removed, moved or reordered.**

| Protected item | Status |
|---|---|
| the legacy Standard body | ✅ untouched — nothing between the stance gate and the end of `UpdateState` changed |
| `UpdateStateStandardCommanded` | ✅ zero lines |
| `UpdateStateGrouped` (Hold/Ambush) | ✅ zero lines |
| `UpdateStateSiege` / all Siege paths | ✅ zero lines; Siege is excluded by `CanFollowHero()`, so Ogre/Sapper never enroll and **still auto-march** |
| `AcquireTarget` | ✅ zero lines |
| all bot/Red behaviour | ✅ every new path is gated on `Team == Blue` or on a group id only a Blue unit can hold |
| the 3 ANCIENT-GROUNDS attack-seal guard points | ✅ zero lines (`EnterAttack`, `UpdateStateGrouped`, `PerformAttack` all untouched) |
| boost / health-bar surface | ✅ zero lines; TASK-379's getters untouched and still public |
| combat, economy, match flow | ✅ no timer, damage, gold or match-state line touched |

**With no C press and no enrollment, `CommandGroupId` is `INDEX_NONE` and the hoisted block is a single integer compare that falls through.**

---

## 🔍 WHAT QA SHOULD SCRUTINISE HARDEST

1. **The `bStillWalking` term in the anti-repath gate (§4).** My one addition beyond the spec's letter. I argue it is required (it closes a stall) and cannot re-open the mill (it only fires when nothing is in flight). If QA disagrees, deleting the term is a one-line change — but read the stall analysis first.
2. **The enroll's placement in `LoadStatsAndStart` rather than `BeginPlay`'s tail (§6).** Deliberate deviation; reason (1) — the `Profile` constructor default is `Standard` — is the one that makes the literal placement a real bug, not a style preference.
3. **The deleted base-side miner carve-out (§7).** Please confirm my reading of TASK-398's window is right: that `AMinerUnit::CanFollowHero()` returns false for the whole of `Super::BeginPlay()`, and that my enroll is inside that stack. If it is not, miners auto-enroll and the economy silently dies — this is the single highest-consequence interaction in the batch.
4. **The duplicated dead-id self-heal in `UpdateState` (§2).** I argue it is a strict improvement (it closes a leak for Support units that can now hold ids) and a no-op for Standard units. Worth a second pair of eyes on the "no-op for Standard" claim.
5. **`UpdateStateSupport`'s extraction (§8)** — specifically that leaving `FaceTarget` at the call site preserves statement order exactly.
6. **`GetGroupStationOffset()` ships with no caller (§9).** Justified as an assigned cross-task deliverable, not speculation, but it is dead code today.
7. **The hoist's extra per-tick resolve for zone-ordered units (§2).** Cheap, but it is a real cost I chose over restructuring the shipped dispatch.

## 🚩 FLAGGED (recorded, not decided by me)

- **The `Castle.h` consolidation of `FindOwnCastle`** — the file is free now, I still refused (byte-identical set). Needs a deliberate decision, not an assignee's initiative.
- **The late-`InitUnit` miner hole (§7)** — unreachable today; the fix belongs in `AMinerUnit` if a plain-`SpawnActor` unit spawner is ever added.
- **A following Cleric is the only unit that runs two searches per tick** (heal targeting + its own movement). Negligible at fleet sizes, recorded for the perf capstone.

## Compile traps checked

No literal `*/` inside any doc comment I wrote · every `UE_LOG` format string is a string literal (`TCheckedFormatString`, the TASK-268 C7595 lesson) — the one new log takes `%s` + `%d` with `static_cast<int32>` · no shadowing of inherited reflected members (`LastFollowGoalLocation`, `bHasFollowGoalLocation` grepped, both new) · complete-type include law satisfied by the **existing** includes (`SiegePlayerController.h`, `UnitCommand.h`, `AIController.h`, `Navigation/PathFollowingComponent.h`) — **I added no include** · `AAIController::GetMoveStatus()` verified `const` in UE 5.8 (`AIController.h:246`), so the `const AAIController*` call compiles · 7 new declarations, 7 definitions, parity verified.
