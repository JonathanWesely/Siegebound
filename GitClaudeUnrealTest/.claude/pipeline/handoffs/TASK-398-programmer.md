# TASK-398 — [FC-4] Miner command SEMANTICS (gameplay-programmer handoff)

**Status:** `ready-for-qa`
**Law:** CONVENTIONS "FOLLOW command + the DEFAULT-STANCE law + the MINER command rework (2026-08-02)" §5 / §7 + "Castle 3× HOLLOW (2026-07-28)" (team-gated interior) + "Mirrored depleting mines"
**Builds on:** `handoffs/TASK-397-programmer.md` (approach **(B)** — the seam lives in the miner's own 0.25 s `UpdateMining` poll; all three structural seals kept, `CanEverAttack() → false` as seal #4)
**Files touched (exactly six, all mine per manager ruling 4):**
`Source/GitClaudeUnrealTest/Siegebound/MinerUnit.{h,cpp}` · `GoldNode.{h,cpp}` · `Castle.{h,cpp}`
**Files NOT touched:** `SummonedUnit.{h,cpp}` (TASK-396) · `SiegePlayerController.{h,cpp}` · `UnitCommand.h` (TASK-395). Verified by `git diff --numstat`.
**No compile, no Git, no editor, no MCP** (per the dispatch).

---

## 0. THE HEADLINE — what QA should check first

Two things carry all the risk in this task, and both are answered in §2 and §3:

1. **A freshly played miner still mines.** Proven below as a call-by-call chain against the *landed* TASK-396 code, not against its spec — and the proof holds at **two independent gates**, so it does not depend on where TASK-396 put its call.
2. **A miner can still be told to Follow.** The obvious implementation of (1) — `ClearCommandGroup()` after the auto-enroll, the lever TASK-397's handoff flagged — would have made **Follow permanently unreachable for miners**. That trap and its fix are §3.

---

## 1. THE FIVE-BRANCH TABLE, AS BUILT

`AMinerUnit::ResolveMinerOrder()` answers one `FMinerOrder` per poll; `UpdateMining` runs the matching body. The five commands collapse onto **four modes**, which is what keeps the surface small enough to reason about:

| Jonathan's words | Reaches the miner as | `EMinerOrderMode` | Body |
|---|---|---|---|
| **"'Attack' means they find the nearest mine and start mining."** | stance `Attack` | `Mine` | **Today's loop, unchanged** — `SeekBestMine()` → `AGoldNode::FindBestMineFor` → walk → `TryRegisterArrivedMiner` → `AddMinerIncome` |
| *(no command issued yet — the §5 spawn default)* | `HasIssuedCommand() == false` | `Mine` | **the same branch, not a copy** — one mode, one body, so they cannot drift |
| **"'Defend' means they come back to the castle and hide inside of it."** | stance `Defend` | `GoToPoint` | `LeaveMining()` → walk to `ACastle::GetInteriorAnchorLocation()` on `ACastle::FindNearestCastleForTeam(World, Team, …)` → stand |
| **"'Hold' … it only goes to the position circle and will mine a mine if there is one in the position circle."** | group `Type == Hold` | `MineInDisc` | rung 1: `AGoldNode::FindBestMineInDisc(centre, radius)` → the normal mining body. rung 2 (no mine in circle): walk to `PositionCenter + GetGroupStationOffset()` |
| **"'Ambush' is the same thing as 'hold'."** | group `Type == Ambush` | `MineInDisc` | **the same `case` label — implemented once, cannot fork.** The leash-exemption that separates them for a fighter is meaningless with no target |
| **"The miner will use the 'follow' command the same way as all other commandable units follow it."** | group `Type == Follow` | `GoToPoint` | `LeaveMining()` → walk to `Anchor->GetActorLocation() + GetGroupStationOffset()`, anti-repath-banded |
| hero dead / unresolvable under Follow (ruling 8) | `GetFollowAnchor() == nullptr` | `Stand` | `StopMovement`, hold position, resume the instant a live pawn resolves |
| no standing own castle under Defend (§5) | finder returns null | `Stand` | idle in place (logged once) |

**`Stand` is a TASK-398 addition to TASK-397's three modes** — it is the only honest encoding of the two *ruled* no-destination cases. It is never a fallback for a read failure: **the DEGRADE RULE is preserved at every new exit** (no world / no controller / dead group id / unrecognised group type all answer `Mine` — a miner that cannot hear its orders keeps EARNING).

### "The attacking portion is SKIPPED ENTIRELY" — how, concretely
`Group->AttackCenter` and `Group->AttackRadius` are **never read anywhere in `MinerUnit.cpp`** (grep-able). There is no tier-1 and no tier-2 acquisition because there is no acquisition code in this class at all — `ResolveMinerOrder` returns a *destination*, nothing else. That plus the four seals is the whole answer to "a miner never enters Attack under ANY command" (§4).

---

## 2. ⚠️ "PROVE A SPAWNED MINER STILL MINES" — the chain, step by step

Traced against the **landed** `SummonedUnit.cpp` (TASK-396 committed to the working tree while I was mid-task), not against its spec.

| # | Code | What happens | Why it holds |
|---|---|---|---|
| 1 | `AMinerUnit::BeginPlay` → `Super::BeginPlay()` | `ASummonedUnit::BeginPlay` reaches its §2 follow auto-enroll (`SummonedUnit.cpp:1238`) | — |
| 2 | `if (!IsFollowCommandEligible()) return;` (`:1238`) | `IsFollowCommandEligible()` = `CanFollowHero() && Blue && !bDead && !bAIFrozen`. `AMinerUnit::CanFollowHero()` = `bFollowOnSpawn \|\| bSpawnFollowEnrollWindowClosed` = **`false \|\| false` = false** | **The enroll returns before `EnrollInDefaultFollowGroup` is ever called.** The miner joins **no group AND no `Members` array** |
| 3 | *(belt)* `EnrollInDefaultFollowGroup` (`SiegePlayerController.cpp:3045`) | gates on `Unit->IsFollowCommandEligible()` **again**, independently | **The refusal holds at BOTH sites.** Even if TASK-396 moved or duplicated its call site, the controller's own gate refuses. This is why the proof does not depend on TASK-396's internals |
| 4 | `bSpawnFollowEnrollWindowClosed = true;` | first statement after `Super::BeginPlay()`, **unconditional and above every early-out** | From this instant the miner is fully follow-eligible; the false window is exactly one synchronous call stack wide and nothing outside it can observe it |
| 5 | tripwire `if (!bFollowOnSpawn && GetCommandGroupId() != INDEX_NONE)` | false today (id is `INDEX_NONE`) — no log, no clear | If a future change ever defers the enroll past this stack, this **logs a Warning and clears**, so the miner *still mines* and the regression is loud instead of silent |
| 6 | `SeekBestMine()` + `EnsureWalkingToNode(Node)` | **shipped lines, untouched** — the walk starts inside `BeginPlay` exactly as today | — |
| 7 | first `UpdateMining` (0.25 s) → `ResolveMinerOrder()` | `GetCommandGroupId() == INDEX_NONE` ⇒ the whole group branch is skipped; `HasIssuedCommand()` false ⇒ falls through ⇒ **`Mine`** | — |
| 8 | dispatch | `Mine` is neither `Stand` nor `GoToPoint` ⇒ falls into the mining body ⇒ retarget gate ⇒ arrival ⇒ `TryRegisterArrivedMiner` ⇒ `AddMinerIncome` | **+1 gold/s, unchanged** |

**Independent corroboration that the two lanes agree:** TASK-396's own `SummonedUnit.h:418-423` and `SummonedUnit.cpp:1221-1237` document this exact mechanism ("delivered ENTIRELY by `AMinerUnit::CanFollowHero()`, which answers its EditDefaultsOnly `bFollowOnSpawn` switch for exactly as long as `ASummonedUnit::BeginPlay` is on the stack") and record that a **second, base-side miner carve-out was drafted and deliberately removed** so `bFollowOnSpawn = true` is not silently defeated. One decision, one owner. I did not coordinate with that task — we converged on the same contract from the same law, which is itself a check.

**What is NOT claimed:** this is a code-path proof, not a PIE observation. `TASK-401 (f)` — "a miner played with no command mines and reaches +1 gold/s" — is the machine verification and stays open.

### 🚩 THE FLIP LINE JONATHAN FLIPS (spec item 3)

```cpp
// Source/GitClaudeUnrealTest/Siegebound/MinerUnit.h   (protected block)
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Miner")
bool bFollowOnSpawn = false;        // ← THIS LINE. false = miner SPAWNS MINING (shipped).
```

- `false` → **shipped default**: a miner spawns mining; Follow applies to it only when explicitly circled with **C**.
- `true` → **the literal reading**: every miner spawns following and earns zero gold until personally ordered.

It is `EditDefaultsOnly`, so **Jonathan can flip it on `BP_Unit_Miner` with no compile at all** — which is the better lever for a playtest gate. The C++ initializer is the other. Read exactly once per miner (inside `Super::BeginPlay`); toggling it mid-match deliberately does nothing to miners already on the field, whose orders are the player's.

---

## 3. ⚠️ THE TRAP I HIT — why `ClearCommandGroup()` after the enroll is the WRONG lever

TASK-397's handoff §6.4 named the cheapest lever as "one `ClearCommandGroup()` call immediately after `Super::BeginPlay()`". **I did not take it, and QA should understand why, because it looks correct.**

`ClearCommandGroup()` clears the **unit's** id and offset. It **cannot remove the unit from the group's `Members` array** — that array lives on `ASiegePlayerController`, and manager ruling 4 forbids me touching that file. A miner left in `Members` with a cleared id then hits this, forever:

```cpp
// ASiegePlayerController::EnrollInDefaultFollowGroup — SiegePlayerController.cpp:3074
if (FollowGroup->Members.Contains(WeakUnit))
{
    return;          // ← idempotence early-out: never reaches AssignCommandGroup
}
```

**Consequence:** pressing **C** over that miner would silently do nothing — no group id, no station — and `ConfirmFollowPick`'s readout (`Unit->GetCommandGroupId() == DefaultFollowGroupId`) would not even count it. **Follow would be permanently unreachable for miners**, breaking the one miner behavior Jonathan spelled out in full ("the same way as all other commandable units"). It would also have looked like a success in every test that only checks "does a spawned miner mine".

**Refusing the enroll instead leaves the controller's state perfectly consistent:** the miner is in no group, in no `Members` array, and a later C press enrolls it fresh with a real, controller-computed station. The cost is that `CanFollowHero()` returns a member rather than a literal `true` — the **signature is still §7-pinned character-for-character** (`virtual bool CanFollowHero() const override`), only the value is data-driven, which is the semantic this task owns. Flagged here rather than buried.

---

## 4. A MINER STILL CANNOT ATTACK UNDER ANY OF THE FIVE — four independent reasons

Unchanged from TASK-397 and **not weakened by adding command bodies**, because the bodies are destination decisions, not state-machine entries:

1. `StateCheckInterval = 0` ⇒ `SetTimer` with rate ≤ 0 **clears** ⇒ `ASummonedUnit::UpdateState` never repeats ⇒ TASK-396's hoisted follow dispatch and the Hold/Ambush group dispatch **can never run for a miner**.
2. `AggroRadius = 0` ⇒ `AcquireTarget` returns null.
3. `ClearAllTimersForObject(this)` after `Super::BeginPlay`.
4. `CanEverAttack() → false`, honoured at the three shipped guard points — `EnterAttack` (stands down to Idle), `UpdateStateGrouped` (acquires nothing), `PerformAttack` (refuses). *Function names are the durable anchor; TASK-396 edited `SummonedUnit.cpp` and shifted TASK-397's line numbers, so I deliberately cite names, not lines.*

Plus, new and specific to this task: **no body added here calls any acquisition or attack surface** (they are all `private` on the base and unreachable from this file regardless), and `AttackCenter`/`AttackRadius` are never read.

**The one synchronous `UpdateState()` inside `LoadStatsAndStart` is unchanged:** the auto-enroll runs *after* it (`SummonedUnit.cpp:1178` notes Profile is bound ~50 lines above the enroll), so at that instant `CommandGroupId` is `INDEX_NONE`, the follow dispatch is skipped, and the legacy body issues the same acquisition-dead castle-bound Advance it has always issued — replaced by `EnsureWalkingToNode` in the same call stack.

---

## 5. THE REGRESSION PROOF — mechanical, from the diff

`git diff --numstat` against HEAD (which includes TASK-397's and my work, both uncommitted):

| File | + | − |
|---|---|---|
| `Castle.h` | 73 | **0** |
| `Castle.cpp` | 55 | **0** |
| `GoldNode.h` | 28 | **0** |
| `GoldNode.cpp` | 79 | **0** |
| `MinerUnit.h` | 429 | **0** |
| `MinerUnit.cpp` | 549 | 12 |

**Four of the six files are PURE ADDITIONS — zero lines removed.** `FindBestMineFor` is untouched (the bot shares it — §5 and TASK-400 criterion 9); the new `FindBestMineInDisc` is a deliberate write-out beside it, documented as such on both sides ("change one, change both").

`MinerUnit.cpp`'s 12 removed lines are **8 comments + these four executable lines** (reproduce with `git diff -U0 -- …/MinerUnit.cpp | grep -E "^-[^-]"`):

```
-	const bool bTargetDead = !Node || Node->IsDepleted() || Node->GetGoldReserve() <= 0;
-		EndMineTenure();
-		Node = SeekBestMine();
-		if (AGoldNode* Upgrade = AGoldNode::FindBestMineFor(GetWorld(), Team, GetActorLocation()))
```

Each, and why it is behaviour-preserving under `Mode == Mine`:

| Removed | Replaced by | Behaviour under `Mine` |
|---|---|---|
| `bTargetDead = …` | `… \|\| bOutOfOrderDisc` | `bOutOfOrderDisc` short-circuits to `false` unless `Mode == MineInDisc` ⇒ **identical expression** |
| `EndMineTenure();` | `LeaveMining(); Node = nullptr;` | `LeaveMining` = `UnregisterArrivedMiner` + `EndMineTenure` + null target. On the pre-existing path the mine is destroyed (weak null) or depleted (`Deplete()` emptied its registry first) ⇒ **the extra call is a provable no-op**; the null target is overwritten by `SeekBestMine()` two lines later |
| `Node = SeekBestMine();` | ternary on `Mode` | picks `SeekBestMine()` when `Mode == Mine` ⇒ **same call**. `SeekBestMine` itself is byte-identical and its `BeginPlay` caller is untouched |
| `if (AGoldNode* Upgrade = …)` | ternary + `if (Upgrade)` | picks `FindBestMineFor` when `Mode == Mine` ⇒ **same call, same guard** |

**Therefore untouched, by diff rather than by argument:** `EndPlay` · `FreezeAI` · `StartMiningClink`/`StopMiningClink` · the arrival test · the whole `TryRegisterArrivedMiner` / `AddMinerIncome` / wait-mode block · `TryRegisterWithOwnerState` · `ResolveOwningPlayerState` · `SeekBestMine` · `EndMineTenure` · `NotifyMineDepleted` · every latch · the constructor's three seals · the §3.3 cap (enforced in the controller, never read here).

**Income works in every branch that reaches a mine (spec item 4):** `Mine` and `MineInDisc` differ **only** in which finder the retarget gate consults and what an empty finder means. Everything from the arrival test down is **one shared block** — there is no second registration path to drift.

Additions to shipped functions, both one-liners, both listed for QA: `EnsureWalkingToNode` gains `bHasIssuedPointGoal = false;` at entry (mode-latch invalidation — see §7 hazard 1); `BeginPlay` gains the window close + tripwire (§2).

---

## 6. THE CASTLE INTERIOR ANCHOR — measured how, and the honest gap

**New API** (`Castle.{h,cpp}`, both pure additions):
- `FVector ACastle::GetInteriorAnchorLocation() const` — `UFUNCTION(BlueprintPure)`, **§7-pinned signature**. Returns `GetActorTransform().TransformPosition(InteriorAnchorRelativeLocation)` — the **actor transform**, never `ActorLocation + offset`, because `Castle_Red` is placed at **yaw 180** and a non-zero relative anchor must rotate with the castle or "deeper into the keep" inverts on one side of the map.
- `FVector ACastle::InteriorAnchorRelativeLocation` — `EditDefaultsOnly`, default `FVector::ZeroVector` per §8. Flagged tunable.
- `static ACastle* ACastle::FindNearestCastleForTeam(UWorld*, ETeamId, const FVector&)` — **new, additive, not in §7**. See §8 flag 1 for why it exists and what it deviates on.

**RESOLVED WORLD POINTS at the shipped placement:**

| Castle | Actor transform | `GetInteriorAnchorLocation()` |
|---|---|---|
| `Castle_Blue` | `(−25000, 0, 0)`, yaw 0 | **`(−25000, 0, 0)`** |
| `Castle_Red` | `(+25000, 0, 0)`, yaw 180 | **`(+25000, 0, 0)`** |

**How those were established, and their provenance** (I had no editor — dispatch says no editor/MCP):
- Placement: `handoffs/TASK-218.md` — "Castle_Blue / Castle_Red | X ∓8,000 → **X ∓25,000** (Red keeps yaw 180)".
- Cross-check, independent: `CONVENTIONS.md:282` records `SpawnBoxHalfExtent = 2460` putting the spawn-box inner edge at `|X| = 22540` — which is exactly `25000 − 2460`. Two documents, one number.
- At `ZeroVector` the transform reduces to the actor location for **any** rotation, so yaw 180 cannot perturb the Red value.

**That "inside the shell" is the right point, derived from law (`CONVENTIONS.md:707-711`):** `SM_Castle` bounds `2442 × 2460 × 2694` with a **ground-centre origin**, hollow, with the "interior floor FLAT at ground level, threshold step ≤ 40 uu". So local `(0, 0, 0)` is the interior floor's centre, and `Z = 0` is the **floor** — which is what a navmesh destination wants (a character's own location is its capsule centre ~90 uu higher; the mover projects).

**⚠️ THE HONEST GAP — I could not nav-project, and I am not claiming I did.** §5 says "MEASURE, DO NOT ASSUME"; without an editor the strongest thing available is a derivation plus a runtime probe, so I built the probe: **`ResolveMinerOrder` emits one `Log` line per miner carrying the resolved world point and the castle's location**, and `DriveToPoint` emits one `Warning` if `MoveToLocation` returns `Failed`. TASK-401 gets the measurement for free from the log with no editor probe. **The live "it nav-projects and lies inside the shell" confirmation is NOT discharged here and must be an item on TASK-401.**

**Failure posture, per §5:** a nav failure that strands the miner at the gate is ACCEPTABLE — logged once, the poll keeps retrying, **never a crash and never a fallback to attacking** (which this class structurally cannot do anyway). If PIE shows local `(0,0)` inside a keep/tower hull rather than the open hall, the fix is the **tunable, not the code**: nudge `InteriorAnchorRelativeLocation` toward `+Y` (the gate corridor mouth is on local `−Y` per `GateBlockerRelativeLocation.Y = −525`, so `+Y` is deeper into the keep).

**TASK-350 is reused, not reinvented:** the walk is an ordinary `MoveToLocation` with **`FilterClass = nullptr`** — an *unfiltered* query, which is exactly what lets the own team cross `UNavArea_{Blue,Red}CastleInterior` (the enemy's `UNavFilter_Team*` is what excludes it), and the `GateBlockerVolume` already ignores the own team's channel. No new mechanic, no new component, no level change.

---

## 7. ⚠️ THINGS QA SHOULD SCRUTINISE (flagged, not hidden)

1. **THE ANTI-REPATH BAND IS IMPLEMENTED, AND ONE HAZARD IN IT IS SUBTLE.** `DriveToPoint` re-issues only when `bMoveInFlight && bWithinBand` is false, measuring drift **goal-to-goal** (`Point` vs `LastIssuedPointGoal`), never goal-to-miner. **The hazard:** `GetMoveStatus() != Idle` is also true for a move toward a *gold node*. Without invalidation, switching mining → Follow while a node walk was in flight could read "in flight" + "within band" (if the hero happened to be near a stale latched goal) and leave the miner **walking to the mine under a Follow order**. Fix: `EnsureWalkingToNode` clears `bHasIssuedPointGoal` unconditionally at entry, and `StandInPlace` clears it too. **`LeaveMining` deliberately does NOT** — it runs on every poll of a non-mining order, and clearing there would defeat the band entirely and reproduce the TASK-280/282 mill. Please verify that reasoning; it is the one place a plausible "tidy-up" would silently break the requirement.
2. **`DriveToPoint` mirrors `ASummonedUnit::UpdateStateFollow` deliberately** — same 2D 150 uu arrival test, same last-issued-goal band, same "…and still walking" second term (a stopped follower must be allowed to re-issue or it stalls a whole band short). Diff the two bodies; they should read as siblings. Divergence would mean a miner follows differently from a Footman, which is the thing Jonathan asked not to happen.
3. **I read `GetGroupStationOffset()` rather than re-deriving the sunflower.** TASK-396 landed that public getter mid-task *specifically for this call site*. So `Follow` is literally `Anchor->GetActorLocation() + GroupStationOffset` — the identical expression the base uses — and Hold is `PositionCenter + GroupStationOffset`. **An earlier draft of this task derived its own golden-angle offset (the private-member wall TASK-397 flagged); that duplication is now deleted**, along with the two mirrored constants and the two cache members it needed. Zero formation code lives in `MinerUnit.cpp`. If QA reviewed an intermediate state, re-check.
4. **🚩 REINFORCEMENT MINERS INHERIT THE *STANCE*, so a miner played after **E** walks to the castle instead of mining.** This is pre-existing Shield-Wall law (`bHasIssuedCommand` is latched player-wide and every unit reads it live) applied to a new unit type — **not** a change I made, and distinct from board flag (a), which is about *group* orders not being inherited. It is consistent with how every other unit behaves, so I shipped it, but it is a genuine economy consequence and belongs on Jonathan's gate: *press E, then play a miner, and it hides instead of mining until you press T.*
5. **`bOutOfOrderDisc` and `FindBestMineInDisc`'s gate MUST agree** (both 2D, both boundary-inclusive, both measured mine-location-to-centre). If they ever diverge, a mine could be legal to the finder and illegal to the retarget test and the miner would oscillate every poll. They are written to match and cross-referenced in comments; worth an explicit check.
6. **The MineInDisc tier-1 upgrade searches the disc, not the map** — otherwise a Hold miner could be "upgraded" to a mine outside its circle, fail `bOutOfOrderDisc` next poll, and mill. Marked `DIFFERENCE 1c` in code.
7. **`Stand` under a dead hero deliberately does NOT fall back to mining.** A following miner the player pulled off the mines must not silently go back to work because the hero died — that is ruling 8 (HOLD POSITION) applied consistently. It does mean a dead hero costs the player that miner's income until respawn. Recorded as a deliberate reading, not an oversight.
8. **`CanTakeZoneOrders()` is unconditionally `true` while `CanFollowHero()` is conditional.** The asymmetry is the ruling, not sloppiness: Follow is *pushed* at spawn (hence the carve-out), whereas R/F only ever reach a miner because the player drew a circle around it. Nothing to opt out of.
9. **Log budget.** Every new log is one-shot-latched: `bLoggedNoMineInDisc` (per episode, re-armed), `bLoggedInteriorAnchor` (once per miner lifetime), `bLoggedNoOwnCastle` (re-armed when a castle resolves), `bWarnedPointMoveFailed` (once). No new per-poll log traffic on any path. Red/bot miners exit `ResolveMinerOrder` on one enum compare — **entirely unchanged, zero new cost**.

---

## 8. FLAGS FOR THE MANAGER / LATER PASSES

1. **`ACastle::FindNearestCastleForTeam` is a NEW signature not in the §7 registry.** The `names:` block says `ASummonedUnit::FindOwnCastle` — but that function is **private** on `ASummonedUnit` and its file is TASK-396's exclusive property this batch, so it was unreachable. The new static is a faithful mirror (same-team, `IsValid`, **skips destroyed**, nearest wins, first-found on tie) living on `ACastle`, which is the `AGoldNode::FindBestMineFor` idiom. **One deliberate deviation:** it uses squared 2D distance (the house arena metric) instead of the base's bounds-aware `GetDistanceToTarget`; with exactly one standing own castle per match the two cannot disagree about the winner. **Recorded consolidation for a later pass:** `ASummonedUnit::FindOwnCastle()` could become a one-line delegate to it.
2. **`FMinerOrder` gained two fields** beyond TASK-397's three (`Station`, `RepathTolerance`) and `EMinerOrderMode` gained `Stand`. Internal plain-C++ types of one class, never reflected, never serialized.
3. **Not done, deliberately:** nothing in `SummonedUnit`, the controller, `UnitCommand.h`, `cards.csv`, any `/Game/` asset, or `L_Arena`. No HUD work (board flag (f)).

---

## 9. M8 REPLICATION DECLARATION DUTY

**This task adds NO replicated property and NO new replicated class, so NO new relevancy tier is declared.** Stated explicitly because "nothing to declare" only counts when it is stated (manager ruling 11 / TASK-400 criterion 8).

- `ACastle::InteriorAnchorRelativeLocation` is **`EditDefaultsOnly` design-time data**, identical on both machines by construction — exactly like the shipped `GateBlockerRelativeLocation`/`GateBlockerExtent` gating tunables. Not replicated, and it must not be: replicating a constant would be pure bandwidth.
- `AMinerUnit::bFollowOnSpawn` is likewise `EditDefaultsOnly` design-time data; `bSpawnFollowEnrollWindowClosed`, the two point-goal members and the four log latches are plain non-reflected runtime scalars.
- `AGoldNode::FindBestMineInDisc` and `ACastle::{GetInteriorAnchorLocation, FindNearestCastleForTeam}` are pure reads. `AMinerUnit`'s and `ACastle`'s replication postures are byte-identical to before (`ACastle` keeps its declared **Tier A `bAlwaysRelevant`**; nothing here touches it).
- **M8 TEAM LAW honoured:** `GetFirstPlayerController()` appears nowhere; the controller resolves through `ASiegePlayerController::FindControllerForTeam(World, Team)`, and the castle through the team-filtered finder.
- Units are **server-only in M8 P1**, so every behavior here is **host/single-player-verifiable only** until P2.

---

## 10. COMPILE POSTURE

**Expected to compile only as part of the batch** — by design, and the state is now much better than at dispatch: TASK-395 and TASK-396 both landed in the working tree while I worked, so every §7 symbol this task calls now exists and I verified each against the *landed* code, not the registry text:

| Symbol | Where | Verified |
|---|---|---|
| `ESiegeGroupCommandType::Follow` | `UnitCommand.h:80` | ✅ appended, `Hold == 0` / `Ambush == 1` preserved |
| `ASiegePlayerController::GetFollowAnchor()` | `.h:316` public | ✅ |
| `GetFollowRepathTolerance()` / `FindUnitGroup` / `FindControllerForTeam` / `GetCurrentCommand` / `HasIssuedCommand` | controller public block | ✅ all `const`-callable through my `const ASiegePlayerController* const` |
| `ASummonedUnit::{CanFollowHero, CanTakeZoneOrders}` | `.h:397/406` public virtual | ✅ my overrides match character-for-character |
| `ASummonedUnit::GetGroupStationOffset()` | `.h:365` public | ✅ added by TASK-396 for this call site |
| `ASummonedUnit::{CanEverAttack, GetCommandGroupId, ClearCommandGroup}` | shipped public | ✅ |

**House compile traps checked:** no literal `*/` inside any doc comment (only inline `/*argname=*/` markers in code, the shipped pattern); every `UE_LOG` format string is a string literal (no `TCheckedFormatString` / C7595 exposure); no shadowing of inherited reflected members (`bWarnedNoWalkController` is still the distinctly-named one, and every new member name is unique in the hierarchy); **complete-type include law** — `Siegebound/Castle.h` added to `MinerUnit.cpp` for the `ACastle` calls, `EngineUtils.h` added to `Castle.cpp` for `TActorIterator`, `UnitCommand.h` already present for `FSiegeUnitGroup` / both enums. Brace, paren and bracket balance verified programmatically across all six files (comments and string literals stripped): all zero.
