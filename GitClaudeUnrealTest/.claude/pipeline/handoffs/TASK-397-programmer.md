# TASK-397 — [FC-3] Miner command SEAM (gameplay-programmer handoff)

**Status:** `ready-for-qa`
**Law:** CONVENTIONS "FOLLOW command + the DEFAULT-STANCE law + the MINER command rework (2026-08-02)" §5 / §6 / §7
**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h`, `Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp` — **and nothing else.**
**Base-class lines touched: ZERO.** See §5 below — this is the headline, because TASK-396 owns `SummonedUnit.{h,cpp}` concurrently.
**No compile, no Git, no editor, no MCP** (per the dispatch).

---

## 1. THE DIAGNOSIS — I verified the trap in the code before designing around it

**Verdict: the manager's reading is CORRECT, and it is worse than "the miner would march" — it is the *only* thing an unsealed miner would do.**

`ASummonedUnit::UpdateState` (`SummonedUnit.cpp:1181`) in the current tree:

| Line | What it does | Miner (`Profile == None`) |
|---|---|---|
| 1186 | freeze / dead / `!bStatsLoaded` early-out | passes |
| 1195 | `TrackChargeMovement()` | no-op (`bCharge` false) |
| **1202–1211** | **profile dispatch — `Siege` → `UpdateStateSiege()` + return; `Support` → `UpdateStateSupport()` + return** | **NEITHER — falls through** |
| 1222–1241 | group dispatch (`CommandGroupId != INDEX_NONE`) | skipped — id is `INDEX_NONE` today |
| **1255** | stance gate, gated on **`Profile == ECardProfile::Standard`** `&& Team == Blue` | **excluded — Profile is None** |
| 1286 | `AcquireTarget()` | returns null (`AggroRadius == 0`) |
| **1292–1296** | `Goal = CurrentTarget;` **`if (!Goal) Goal = FindNearestEnemyCastle();`** | **the enemy castle** |
| 1311 | `EnterAdvance(Goal)` | **marches** |

So `Profile == None` is not merely *allowed* into the legacy body — it is **routed past every gate that could have caught it**: the profile dispatch only names `Siege`/`Support`, and the Shield-Wall stance gate at 1255 explicitly requires `Standard`, so a miner cannot even reach `UpdateStateStandardCommanded`. The one branch that would fire is the castle fallthrough at 1295.

**`AggroRadius = 0` provably does not help.** It gates `AcquireTarget` only (`SummonedUnit.cpp:1286`, and inside `AcquireTarget`'s radius test). The castle goal at 1295 is reached *because* `CurrentTarget` is null — i.e. the miner's acquisition seal is the very thing that guarantees the castle branch is taken. Confirmed by the shipped code's own comment at `MinerUnit.cpp:81-83`, which documents exactly this: the ONE synchronous `UpdateState()` inside `LoadStatsAndStart` already issues "an acquisition-dead Advance toward the enemy castle" today — it is harmless only because `EnsureWalkingToNode` replaces the path request in the same call stack, before any movement tick consumes it. **Un-seal the timer and that same request is re-issued every 0.25 s, with no synchronous replacement.**

**Two further findings the spec did not name, both of which decided the approach:**

- **(D1) EVERY base surface an unsealed miner would need is `private:` on `ASummonedUnit`.** The private block starts at `SummonedUnit.h:745`. Inside it: `UpdateState`, `UpdateStateGrouped`, `UpdateStateStandardCommanded`, `UpdateStateSiege`, `UpdateStateSupport`, `EnterAdvance`, `EnterAdvanceToLocation`, `EnterIdle`, `GetAIController`, `FindOwnCastle` (`:865`), and the state itself — `State`, `CommandGroupId`, `GroupStationOffset`. Approach (A)'s "maximum reuse of `UpdateStateFollow`/`UpdateStateGrouped`" is **not reachable from `MinerUnit.cpp` at any price short of a promote-to-`protected:` edit** to the file this batch assigns exclusively to TASK-396. (`UpdateStateFollow` is pinned `protected` by §7, so *that* one would be reachable — but only that one, and it is a hero-anchor body, not a mining body.)
- **(D2) `CanEverAttack() → false` is a provable no-op on today's miner.** Its four readers are `EnterAttack` (`:2050`), `UpdateStateGrouped` (`:1583`), `PerformAttack` (`:2307`) — all three unreachable on a miner whose state timer never arms and whose attack timer is never set — and `CanReceiveDamageBoost` (`:779-796`), which **already returned false for a miner via `AttackDamage > 0.f`**: `Docs/Data/cards.csv` row `Miner` carries `Damage 0`, and that function's own comment names the Miner as one of its three deliberate exclusions. Nothing else in the module calls it.

---

## 2. THE DECISION — CONVENTIONS §6 approach **(B)**, and why

> **Keep the structural seal. Read commands in the miner's own poll.**
> All three shipped seals (`StateCheckInterval = 0`, `AggroRadius = 0`, `ClearAllTimersForObject`) are **unchanged**. `CanEverAttack() → false` is added **on top** as seal #4. The command decision is made in `AMinerUnit::ResolveMinerOrder()`, called once per `UpdateMining` poll.

Four arguments, in the order I weighted them:

1. **(A) cannot be landed inside this task's file ownership.** By (D1), driving the miner from the base state machine requires promoting ~10 private members to `protected:` in `SummonedUnit.h` — a non-trivial edit to a file that manager ruling 4 assigns to **TASK-396 ONLY**, and which is **modified in the working tree right now**. My `names:` block says `MinerUnit.{h,cpp}` ONLY. (B) needs **zero** base lines. This alone is decisive; the rest is why I would still pick (B) with a free hand.

2. **THE DOUBLE-DRIVE IS STRUCTURAL UNDER (A), NOT A RISK TO BE MITIGATED.** (A) would put **two independent 0.25 s drivers on one `UPathFollowingComponent`**:
   - base: `UpdateState` → `EnterAdvance*` → `AI->MoveToActor/MoveToLocation`, re-pathing when `CurrentMoveGoal != Goal` **or** `GetMoveStatus() == Idle` (`SummonedUnit.cpp:2118-2119`);
   - miner: `UpdateMining` → `EnsureWalkingToNode` → `AI->MoveToActor(Node, …)`, re-pathing when path-following is idle **or** `PathFollow->GetMoveGoal() != Node` (`MinerUnit.cpp:410-413`).

   Each gate reads *the other driver's in-flight request as a hijack and cancels it*. Two same-period timers, arbitrary phase ⇒ a guaranteed cancel/re-request mill at up to 8 Hz — precisely the pathology behind **TASK-280** ("units freeze just past midfield") and **TASK-282** ("halt just short of the castle"). The only way to avoid it under (A) is to disarm one driver, and disarming `UpdateMining` means relocating the arrival test, `TryRegisterArrivedMiner`, the income latches and the walk-healing into the base state machine — which spec item (4) and §5 both forbid ("the bookkeeping is NOT rewritten and NOT moved"). **The miner's bookkeeping is welded to its walk; whoever owns the walk must own the bookkeeping. (B) keeps that one owner.**

3. **(A) trades a structural seal for a behavioral one; (B) keeps both.** The 20×/s trap is real (`Cadence 0` + `MinAttackCadence 0.05` clamp in `LoadStatsAndStart`). Under (A) the *only* thing standing between a miner and 20 hits/s is that the three `CanEverAttack()` guards stay complete forever. Under (B), Attack is unreachable **because the decision loop that could reach it never runs**, *and* the guards are there anyway. Strictly stronger, for the project's single most economically load-bearing unit.

4. **(A) makes "provably zero behavior change" much harder to argue, for zero gain.** Restoring `StateCheckInterval` and dropping the `ClearAllTimersForObject` sweep changes the miner's spawn instant and adds a permanent second timer running base code (`TrackChargeMovement`, the freeze gates, whatever routing is added) on every miner, every 0.25 s, forever. The miner needs none of it — it has a loop.

**The honest cost of (B), stated as the spec demands:** a ~15-line duplicate of the base's owning-team-controller + group resolve (`SummonedUnit.cpp:1231-1241`), and TASK-398 will have to source its own station offset and castle pointer rather than reusing the base's. *That second cost is not actually differential* — by (D1), `GroupStationOffset` and `FindOwnCastle()` are private and would have been just as unreachable from `MinerUnit.cpp` under (A). Both are flagged for TASK-398 in §7 below and in code comments at the insertion points.

**`Profile == None` fall-through disposition (the spec asks for this explicitly):** **it is never reached, because the state timer that would reach it is never armed.** (B) does not route `Profile == None` anywhere — it leaves `ASummonedUnit::UpdateState` byte-identical and simply never lets a miner run it more than the one pre-existing synchronous call inside `LoadStatsAndStart`, which behaves today exactly as it behaved yesterday (see §4, row 1).

---

## 3. WHAT I ADDED

**`MinerUnit.h`** (pure addition — the diff removes **no** line):

| Addition | Access | Notes |
|---|---|---|
| `enum class EMinerOrderMode : uint8 { Mine, MineInDisc, GoToPoint }` | file scope | **Plain C++, deliberately not a `UENUM`** — internal token between two private members of one class; never serialized, never edited, never Blueprint-visible. `MineInDisc`/`GoToPoint` are **declared, not implemented** — they are the shape TASK-398 fills. |
| `struct FMinerOrder { Mode; Point; Radius; }` | file scope | Plain POD, by value, re-derived every poll, never cached. |
| `virtual bool CanEverAttack() const override { return false; }` | `public` | **MANDATORY per §6.** Matches the §7 pin character-for-character. |
| `virtual bool CanFollowHero() const override { return true; }` | `public` | §3 table + §7 pin. |
| `virtual bool CanTakeZoneOrders() const override { return true; }` | `public` | §3 table + §7 pin. |
| `FMinerOrder ResolveMinerOrder();` | `private` | **THE SEAM.** Non-const (it self-heals a dead group id). |
| class-doc rewrite | — | seal #4 added to the NO-ATTACK-PATH RULE list; a new **COMMAND SEAM** block records the (B)-over-(A) decision and its evidence in the file itself. |

**`MinerUnit.cpp`** — the complete executable delta is **2 includes + a 5-line guard + one new function**:

```
+#include "Siegebound/SiegePlayerController.h"   // FindControllerForTeam / FindUnitGroup / HasIssuedCommand
+#include "Siegebound/UnitCommand.h"             // FSiegeUnitGroup complete type at the FindUnitGroup call
```
in `UpdateMining`, after the alive-registration retry and before the retarget gate:
```
+	const FMinerOrder Order = ResolveMinerOrder();
+	if (Order.Mode != EMinerOrderMode::Mine)
+	{
+		return;                       // ⚠️ TASK-398 BODY SLOT — unreachable in TASK-397
+	}
```
plus `AMinerUnit::ResolveMinerOrder()` itself: Blue-only early-out → `FindControllerForTeam` → group branch (`GetCommandGroupId` → `FindUnitGroup`, dead id ⇒ `ClearCommandGroup()` self-heal + fall through in the same poll) → stance branch (`HasIssuedCommand`) → **every path returns `EMinerOrderMode::Mine`.**

The seam obeys the **degrade rule** it documents: no world, no controller, or a dead group id all answer `Mine`. *A miner that cannot read its orders must fall back to EARNING, never to stalling.*

`GetFirstPlayerController()` is **not used** — the resolve is `ASiegePlayerController::FindControllerForTeam(World, Team)` (M8 TEAM LAW).

---

## 4. THE ZERO-BEHAVIOR-CHANGE PROOF — what I verified, and how

**Mechanical proof first, because it is the strongest form available without a compile.** `git diff -U0` on both files, filtered:

- **`MinerUnit.h`: the diff removes ZERO lines.** Every change is an addition.
- **`MinerUnit.cpp`: the diff removes exactly ONE line, and it is a comment** — `// (Seal #3 — the post-Super timer sweep — lives in BeginPlay.)`, replaced by a longer comment.
- **Added non-comment lines in `.cpp`: 2 includes, 4 lines of guard, and the new function body.** Nothing else. Reproduce with:
  `git diff -U0 -- Source/GitClaudeUnrealTest/Siegebound/MinerUnit.cpp | grep -E "^-[^-]"`

**Therefore, by inspection of the diff rather than by argument, not one statement of the following was edited, moved, or reordered:** `BeginPlay` · `EndPlay` · `FreezeAI` · `StartMiningClink` / `StopMiningClink` · the whole retarget gate · the arrival test and `TryRegisterArrivedMiner` block · wait mode · `EnsureWalkingToNode` · `TryRegisterWithOwnerState` · `ResolveOwningPlayerState` · `SeekBestMine` · `EndMineTenure` · `NotifyMineDepleted` · every latch (`bArrivedAtNode`, `bIncomeActive`, `bRegisteredAlive`, all six one-shot log guards) · the constructor's three seal writes.

**Behavioral proof, per acceptance bullet:**

| Acceptance item | Why it is unchanged |
|---|---|
| **spawn / first tick** | `BeginPlay` untouched; the constructor's `StateCheckInterval = 0` / `AggroRadius = 0` untouched; the `ClearAllTimersForObject` sweep untouched. The one synchronous `UpdateState()` inside `LoadStatsAndStart` still runs, still acquires nothing, still issues the castle-bound `EnterAdvance`, and is still replaced by `EnsureWalkingToNode` in the same call stack. `CanEverAttack() → false` cannot alter that path — it is read only by `EnterAttack`/`UpdateStateGrouped`/`PerformAttack`, none of which that path enters (no target ⇒ line 1305's `if (CurrentTarget && …)` is false ⇒ `EnterAdvance`, not `EnterAttack`). |
| **walk → register at the ring → +1 gold/s once per tenure** | The entire block is byte-identical (diff proof). The seam runs *before* it and returns `Mine`, so control reaches it on every poll exactly as before. |
| **wait mode on an enemy-claimed mine · eviction re-seek · all-depleted idle** | Same — untouched code, unconditionally reached. |
| **§3.3 miner cap** | Enforced in `ASiegePlayerController` (`CanAddMiner`, TASK-030). Not read, not touched. |
| **income latches / death bookkeeping** | `EndPlay(Destroyed)`, `EndMineTenure`, `NotifyMineDepleted` untouched. |
| **`FreezeAI` still kills every miner timer** | `FreezeAI` untouched; it still clears `MiningPollTimerHandle`, so the seam cannot run on a frozen miner. Belt: `UpdateMining`'s `IsUnitDead() || IsAIFrozen()` gate is **above** the seam call, so a stray timer fire never even resolves a controller. |
| **a miner can NEVER enter Attack** | Four independent reasons, three of them shipped: (1) `StateCheckInterval = 0` ⇒ `SetTimer` **clears** instead of scheduling ⇒ `UpdateState` never repeats; (2) `AggroRadius = 0` ⇒ `AcquireTarget` returns null ⇒ the `EnterAttack` branch at `SummonedUnit.cpp:1305` is never taken; (3) `ClearAllTimersForObject(this)` after `Super::BeginPlay`; **(4) NEW — `CanEverAttack() → false`, honoured at the three shipped guard points: `EnterAttack` `SummonedUnit.cpp:2050` (stands down to `EnterIdle`), `UpdateStateGrouped` `:1583` (forces `CurrentTarget = nullptr`), `PerformAttack` `:2307` (joins the `bDead/bAIFrozen/bSpellFrozen` refusal gate).** |
| **cost of the seam** | Red bot miners: **one enum compare** (the `Team != Blue` early-out) — no world query, no iteration. Blue miners: one `FindControllerForTeam` (silent, allocation-free, iterates the PC list — 1 entry in standalone; verified at `SiegePlayerController.cpp:1506-1534`, **zero logging on every path**) plus at most one `FindUnitGroup`, four times a second, for ≤6 miners. **No new log line is emitted by this change on any path.** |

**Guard points cited by file:line as requested** — `SummonedUnit.cpp:2050` / `:1583` / `:2307`, measured against the current tree (`SummonedUnit.cpp` is unmodified in the working tree as of this handoff). **TASK-396 will edit that file and shift these numbers; the function names are the stable anchor.**

---

## 5. BASE-CLASS DELTA — **NONE.** Named, as the dispatch requires

**I touched zero lines of `SummonedUnit.h`, `SummonedUnit.cpp`, `SiegePlayerController.*`, `UnitCommand.h`, `GoldNode.*` or `Castle.*`.** `git diff --name-only` shows `MinerUnit.h` + `MinerUnit.cpp` as my only source files. (Other files are dirty in the tree from concurrent tasks — `SummonedUnit.h` + `SiegeCheatManager.cpp` + `DeckBuilderWidget.cpp` = TASK-379/380; `SiegePlayerController.{h,cpp}` + `UnitCommand.h` = TASK-395 in flight. **None of those edits are mine.**)

Everything the seam needs was already public on the shipped base:

| Surface used | Where | Access |
|---|---|---|
| `ASummonedUnit::CanEverAttack()` | `SummonedUnit.h:390` | `public virtual` (TASK-360) — override only |
| `ASummonedUnit::GetCommandGroupId()` | `SummonedUnit.h:363` | `public` |
| `ASummonedUnit::ClearCommandGroup()` | `SummonedUnit.h:350` | `public` |
| `ASummonedUnit::Team` | `SummonedUnit.h:532` | `protected` |
| `ASiegePlayerController::FindControllerForTeam` / `FindUnitGroup` / `HasIssuedCommand` | controller `public` block | `public` |

---

## 6. ⚠️ THINGS QA SHOULD SCRUTINISE (I am flagging these, not hiding them)

1. **BUILD-ORDER DEPENDENCY — the one thing that can fail a compile.** `CanFollowHero()` / `CanTakeZoneOrders()` are `override`s of virtuals **TASK-396 has not written yet** (`SummonedUnit.h` currently has neither). Until TASK-396 lands them **exactly as §7 pins them**, these two lines are `error C3668`. This is the batch's designed state (one UBT module against the pinned registry — the ANCIENT-GROUNDS precedent), and the board carries the note, but **the build-master must not compile TASK-397 alone**. `CanEverAttack()` has no such dependency. *If TASK-396 is dropped or renames either virtual, delete these two lines and TASK-397 still stands.*
2. **Is a constant-valued seam "dead code"?** I claim no, and this is the judgement call most worth reviewing. The branches are **live** — they read real controller state and perform a real self-heal; only their *answer* is currently constant, which is the acceptance criterion ("the seam returns the legacy decision in every case"). The one genuinely unreachable construct is the 4-line `if (Order.Mode != Mine) return;` guard in `UpdateMining`, kept deliberately as TASK-398's marked body slot and labelled as such in place.
3. **`CanTakeZoneOrders() → true` has ONE player-visible consequence once TASK-396 lands, and it is not a behavior change:** a Blue miner inside an R/F select circle is now **counted and assigned** a group (`AssignCommandGroup` pushes an id + station offset; `SiegePlayerController.cpp:2514/2645`). **It still mines** — the state timer is sealed, so `UpdateStateGrouped` can never run for it, and the seam ignores the group until TASK-398. The change is to the selection **count** and the HUD prompt string, never to what a miner does. Verify me on this.
4. **🚩 THE §5 MINER SPAWN-DEFAULT RULING IS AT RISK FROM TASK-396, AND TASK-397 IS NOT WHERE IT BREAKS.** With `CanFollowHero() → true`, TASK-396's §2 `BeginPlay` auto-enroll will put **every miner in the default follow group at spawn**. Harmless here (the seam answers `Mine` regardless), but the moment TASK-398 wires Follow, **every miner walks to the hero and earns zero gold** — exactly the outcome ruling 7 says must not ship. **The cheapest lever, and it lives in my file:** one `ClearCommandGroup()` call immediately after `Super::BeginPlay()` in `AMinerUnit::BeginPlay` — a provable no-op today (the id is already `INDEX_NONE`, the offset already zero). **I did NOT add it: it is a semantic, and semantics are TASK-398's.** Recorded here and in the `CanFollowHero()` doc comment so it cannot be lost. The alternative fix is base-side in TASK-396 (skip the enroll for miners) and is the manager's call.
5. **Two private-surface walls TASK-398 will hit** (both documented at the insertion points in code): `ASummonedUnit::GroupStationOffset` is private with **no public getter**, and `ASummonedUnit::FindOwnCastle()` is private (`SummonedUnit.h:865`). TASK-398 needs a one-line base getter for each (safe once TASK-396 releases the file) or its own derivation. **Not needed by TASK-397 — do not add them now.**
6. **Line-number drift:** every `SummonedUnit.cpp:NNNN` citation in my code comments and in this handoff is measured against the current tree. TASK-396 edits that file and will shift them. Function names are the durable anchor.

---

## 7. M8 REPLICATION DECLARATION DUTY

**This task adds NO replicated property and NO new replicated class, so no new relevancy tier is declared.** `EMinerOrderMode` / `FMinerOrder` are plain non-reflected C++ types, `ResolveMinerOrder()` is a per-poll read of server-side state, and the three new overrides are pure `const` predicates. `AMinerUnit`'s replication posture is byte-identical to before. (Stated explicitly because "nothing to declare" only counts when it is stated.)

Units remain **server-only in M8 P1**, so this seam — like the rest of the FOLLOW-COMMAND batch — is host/single-player-verifiable only until P2.

---

## 8. WHAT TASK-398 INHERITS

A file where the *only* thing that has to change to add the five behaviors is **which `FMinerOrder` the two marked returns produce**, plus the bodies for `MineInDisc` and `GoToPoint` at the marked slot in `UpdateMining`. The bookkeeping is untouched and stays the single owner of "am I registered and earning". The two ordering rules TASK-398 must not get wrong are written at the slot in code: **release the mine (`AGoldNode::UnregisterArrivedMiner`) before `EndMineTenure()` when leaving a mine**, and **null `TargetGoldNode` on the way OUT of mining, not on the way back in.**
