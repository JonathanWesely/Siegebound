# TASK-531 — `FSiegeStuckStatics`, the pure stall-detection ladder — PROGRAMMER HANDOFF

- **Task:** TASK-531 [NP-3], UNIT-PATHING batch
- **Assignee:** gameplay-programmer
- **Status:** `ready-for-qa`
- **QA gate:** **TASK-537** (`.claude/pipeline/qa/TASK-537.md`) — this task names it per `SC-§29` / `NAV-§10`.
- **Law read in full before authoring:** CONVENTIONS `NAV-§3` (behaviour), `NAV-§8` (the pin), `NAV-§7` (naming), `NAV-§11` (M8), plus the standing anti-mill law ("FOLLOW command…" §4).

---

## M8 DECLARATION — VERBATIM

> **adds no replicated property, no new replicated class, no new relevancy tier.**

And the structural reason: **`FSiegeStuckState` is unreflected, so it cannot be replicated by accident.** `FSiegeStuckTuning` **is** reflected but it is CONFIG on the CDO (`EditDefaultsOnly`), identical on every machine by construction. The declaration is also present verbatim in `SiegeStuckStatics.h`'s file-header comment, as spec (9) requires.

---

## FILES TOUCHED — EXACTLY TWO, BOTH NEW

| file | lines | note |
|---|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeStuckStatics.h` | 257 | NEW. The pin, implemented. |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeStuckStatics.cpp` | 210 | NEW. `DEFINE_LOG_CATEGORY(LogSiegeStuck)` + three function bodies. |

**Nothing else moved.** No `Build.cs` change (this pair adds no dependency at all — `CoreMinimal` is the only include). No `.umap` write. No `SummonedUnit` / `MinerUnit` / `SiegeNavAreas` / `BattlefieldScatter` edit (532/533/534/535 own those). No call sites (532/533 wire it). No tests (536 owns them). No compile, no Git, no editor/MCP/PIE.

**Assets referenced:** none. This library touches no content path.

---

## 1. PINNED-REGISTRY CONFORMANCE — EACH SYMBOL QUOTED AS WRITTEN, NEXT TO THE REGISTRY VERSION

`NAV-§8` is the link contract and it is implemented **character-for-character**. The only difference anywhere below is **leading whitespace**: the registry block is indented with 4 spaces, the shipped header uses **tabs** (house style, and what every other file in `Siegebound/` uses). Column alignment *inside* a line is preserved exactly. ⛔ No symbol, type, order, default, access level or parameter name differs.

### 1a. Log category

| registry (`CONVENTIONS.md:2554`) | as written (`SiegeStuckStatics.h:73`) |
|---|---|
| `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeStuck, Log, All);` | `DECLARE_LOG_CATEGORY_EXTERN(LogSiegeStuck, Log, All);` |

`DEFINE_LOG_CATEGORY(LogSiegeStuck);` is at `SiegeStuckStatics.cpp:11`, per `NAV-§7` ("declared in `SiegeStuckStatics.h`, defined in its `.cpp`"). ⛔ **Nothing in this library logs** — the category is declared here purely for the wiring tasks (see §5).

### 1b. `ESiegeStuckAction` — plain enum, NOT a `UENUM`

| registry (`:2557`) | as written (`:87`) |
|---|---|
| `enum class ESiegeStuckAction : uint8 { None, Sidestep, WidenAndRepath, Abandon };` | `enum class ESiegeStuckAction : uint8 { None, Sidestep, WidenAndRepath, Abandon };` |

No `UENUM()`, no `GENERATED_*`, no reflection of any kind. Four enumerators, that order.

### 1c. `FSiegeStuckState` — POD, NOT a `USTRUCT`

| registry (`:2561-2568`) | as written (`:102-109`) |
|---|---|
| `struct FSiegeStuckState` | `struct FSiegeStuckState` |
| `    FVector ProgressAnchor         = FVector::ZeroVector;` | `	FVector ProgressAnchor         = FVector::ZeroVector;` |
| `    float   StalledSeconds         = 0.f;` | `	float   StalledSeconds         = 0.f;` |
| `    float   SecondsSinceEscalation = 0.f;` | `	float   SecondsSinceEscalation = 0.f;` |
| `    uint8   EscalationLevel        = 0;` | `	uint8   EscalationLevel        = 0;` |
| `    bool    bHasAnchor             = false;` | `	bool    bHasAnchor             = false;` |

⛔ **No `USTRUCT`, no `GENERATED_BODY()`, no `UPROPERTY`, no API macro.** Five fields, that order, those types, those defaults. This is the field `NAV-§11` calls load-bearing: **an unreflected struct cannot be replicated by accident.**

### 1d. `FSiegeStuckTuning` — IS a `USTRUCT(BlueprintType)` (RULING 7a, a DECLARED `SC-§15` departure from the plan)

| registry (`:2573-2586`) | as written (`:126-154`) |
|---|---|
| `USTRUCT(BlueprintType)` | `USTRUCT(BlueprintType)` |
| `struct FSiegeStuckTuning` | `struct FSiegeStuckTuning` |
| `    GENERATED_BODY()` | `	GENERATED_BODY()` |
| `...float ProgressRadius      = 150.f;` | `	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float ProgressRadius      = 150.f;` |
| `...float MinSpeedSq          = 2500.f;` | `	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float MinSpeedSq          = 2500.f;` |
| `...float SidestepSeconds     = 1.5f;` | `	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float SidestepSeconds     = 1.5f;` |
| `...float WidenSeconds        = 3.0f;` | `	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float WidenSeconds        = 3.0f;` |
| `...float AbandonSeconds      = 6.0f;` | `	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float AbandonSeconds      = 6.0f;` |
| `...float EscalationCooldown  = 1.0f;` | `	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float EscalationCooldown  = 1.0f;` |
| `...float SidestepDistance    = 350.f;` | `	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float SidestepDistance    = 350.f;` |
| `...float SidestepLeaseSeconds = 2.0f;` | `	UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Stuck") float SidestepLeaseSeconds = 2.0f;` |

All eight fields, that order, `EditDefaultsOnly`, `Category = "Siegebound|Stuck"`, those defaults. Each carries a one-line doc comment (added; comments are not pinned). ⛔ **No API macro on the struct** — same as the pin; it is only ever used inside this module, so no export is needed.

### 1e. `FSiegeStuckStatics` — exactly three statics, all `public`

| registry (`:2588-2599`) | as written (`:156-256`) |
|---|---|
| `class GITCLAUDEUNREALTEST_API FSiegeStuckStatics` | `class GITCLAUDEUNREALTEST_API FSiegeStuckStatics` |
| `public:` | `public:` |
| `static ESiegeStuckAction Evaluate(bool bAdvancing, const FVector& Location, float VelocitySizeSq,`<br>`                                  float DeltaSeconds, const FSiegeStuckTuning& Tuning,`<br>`                                  FSiegeStuckState& State);` | identical, character-for-character |
| `static FVector ComputeSidestepGoal(const FVector& Location, const FVector& Goal,`<br>`                                   float SidestepDistance, int32 Attempt);` | identical, character-for-character |
| `static void    Reset(FSiegeStuckState& State);` | `static void    Reset(FSiegeStuckState& State);` (two spaces after `void`, as pinned) |

⛔ **Exactly three statics. No fourth helper, no private section, no member state** — the class holds nothing.

### 1f. Header shape

`#pragma once` → `#include "CoreMinimal.h"` → `#include "SiegeStuckStatics.generated.h"` (last include, required by the `USTRUCT`). `ESiegeStuckAction`, `FSiegeStuckState` and `FSiegeStuckTuning` share the header **per `NAV-§7`'s explicit `TeamId.h` exception** — ⛔ `NAV-§7` states outright that **QA must not flag this as a one-class-per-header violation.**

---

## 2. ⭐ THE ANTI-MILL WORST CASE — THE ARITHMETIC, COMPUTED AND STATED

*(QA gate criterion 1 requires the reviewer to compute and state this. The derivation is also written into `SiegeStuckStatics.cpp:107-134` so it is checkable at the code, not only here.)*

**The two brakes are independent, and EITHER ALONE bounds the request rate.** That redundancy is what makes the bound hold under arbitrary `EditDefaultsOnly` tuning rather than only at the shipped defaults.

**BRAKE 1 — the cooldown (time-bounded).** `SiegeStuckStatics.cpp:78`
```
if (State.SecondsSinceEscalation < Tuning.EscalationCooldown) { return None; }
```
Every fire sets `SecondsSinceEscalation = 0` (`:136`), and no fire is reachable while that clock is below `EscalationCooldown`. The gate sits **above** every rung test, so:
> **≤ 1 fire per unit per `EscalationCooldown` = ≤ 1 fire / unit / SECOND at the default 1.0.**

**BRAKE 2 — monotonicity (count-bounded).** `SiegeStuckStatics.cpp:102`
```
if (DesiredLevel <= State.EscalationLevel) { return None; }
```
Strictly greater, and `EscalationLevel` only ever rises within a stall (`:135`). There are exactly three levels, so:
> **≤ 3 fires per stall, ever.** Level 3 is `Abandon`, which issues **no** path request (it drops the goal — `NAV-§3` rung table), so **≤ 2 PATH REQUESTS PER STALL.**

A stall can only restart through `Reset`, and `Reset` on the Abandon path (`:152`) drops `bHasAnchor`, forcing a re-anchor and a fresh `AbandonSeconds` of accumulation before the next Abandon. So:
> **≤ 2 requests / `AbandonSeconds` = 2 / 6 s ≈ 0.33 requests / unit / second, sustained.**

**⇒ THE STATED RESULT**

| | bound | which brake |
|---|---|---|
| **CEILING, any tuning** | **≤ 1 extra path request per unit per second** | brake 1 |
| **SHIPPED DEFAULTS, sustained** | **≈ 0.33 extra path requests per unit per second** | brake 2 |
| **per stall, total** | **≤ 2 extra path requests** (Sidestep + WidenAndRepath; Abandon issues none) | brake 2 |
| **who pays it** | **ONLY units that are demonstrably not moving** | the re-anchor branch |

**AND ONLY FOR NON-MOVING UNITS IS STRUCTURAL, NOT A CLAIM.** A unit that is moving, or that has no active move request at all, exits at `SiegeStuckStatics.cpp:40-49` — it re-anchors, returns `None`, and can never reach the escalation block. Reaching line 135 requires `bAdvancing == true` **AND** `VelocitySizeSq < MinSpeedSq` **AND** displacement-from-anchor ≤ `ProgressRadius`, sustained for `SidestepSeconds`.

**AT SCALE.** A whole-army wedge at ~120 units: **≤ ~120 extra requests/s at the ceiling, ~40/s at the shipped defaults.** Bounded, and only in the case where the alternative is an unwinnable match.

**⛔ WHAT THE LAW FORBIDS, FOR CONTRAST.** The same ladder without those two lines re-fires its rung on every 0.25 s poll = **4 requests/unit/s = ~480/s at 120 units** — the TASK-280 / TASK-282 re-path mill wearing a rescue's clothes.

**COST PER POLL, STATED STRUCTURALLY** (⛔ no fps or ms claim — **no FPS baseline exists in this project**, `NAV-§2c`, so only the structural claim is defensible):
- **common path** (moving / idle-by-design): up to 3 compares, at most one `FVector::DistSquared`, one multiply, one ~32-byte POD store.
- **stalled path**: two float adds + one compare (brake 1 returns before the rung chain on all but ≤1 poll per second).
- **zero allocations, zero world queries, zero engine calls, zero logging, on every path.**
- For scale: `AcquireTarget` already runs a **full-world `GetAllActorsWithInterface` WITH a `TArray` allocation, per unit, on this very same 0.25 s poll** (`SummonedUnit.cpp:1453-1454` — verified first-hand).

---

## 3. `DeltaSeconds == 0` — THE BEHAVIOUR, EXACTLY

`AMinerUnit` sets `StateCheckInterval = 0.f` **by seal** — verified first-hand at `MinerUnit.cpp:61`, comment *"is a deliberate out-of-band value"*. TASK-533 drives this same ladder. `SiegeStuckStatics.cpp:60`:

```cpp
const float SafeDelta = (DeltaSeconds > 0.f) ? DeltaSeconds : 0.f;
```

| input | result |
|---|---|
| `DeltaSeconds == 0` | `SafeDelta = 0` ⇒ both clocks unchanged ⇒ **no rung ever advances**. `Evaluate` still re-anchors correctly and still returns `None`/a rung purely on already-accumulated time. |
| `DeltaSeconds < 0` | clamped to 0 — the clocks can never run backwards. |
| `DeltaSeconds` NaN | clamped to 0. ⛔ Written as `> 0.f ? … : 0.f` **rather than `FMath::Max`** precisely so NaN lands on 0 (every comparison against NaN is false). This is why the accumulators are provably finite and non-negative for the life of the state. |

⛔ **Never divides by the delta** — there is no division anywhere in the file, so a zero delta has no second failure mode. ⛔ **Never asserts** — no `check()`, no `ensure()`, no `checkf` anywhere in the pair. ⛔ **`DeltaSeconds` is a parameter and `StateCheckInterval` does not appear in either file** (grep-able). This is the same latent shape `TrackChargeMovement` already carries — verified at `SummonedUnit.cpp:2733`, `ChargeMoveElapsed += StateCheckInterval;`, harmless there only because `bCharge` gates it out. It is **not** reproduced here.

---

## 4. PURITY — WHAT MAKES TASK-536 HEADLESS

⛔ No `UWorld`, no `AActor`, no `UObject`, no `AAIController`, no `UPathFollowingComponent`, no `UNavigationSystemV1`, no engine singleton, no allocation, no `TArray`, no RNG, no clock read, no `static` mutable state, no logging. **Every input is a parameter; every output is a return value or a write to the caller's `FSiegeStuckState`.**

The `.cpp` includes **only** its own header (`SiegeStuckStatics.cpp:3`), and the header includes **only** `CoreMinimal.h` + its `.generated.h`. That is the whole dependency set — which is also why **no `Build.cs` change is needed or made** (`NAV-§8`: a `Build.cs` edit in this batch is a FINDING).

`ComputeSidestepGoal` determinism: every term is a parameter or a compile-time constant (`FVector::YAxisVector`). Same inputs ⇒ same `FVector`, every call, every machine.

---

## 5. ⚠️ WHAT QA SHOULD SCRUTINISE (I am naming these rather than hoping they are missed)

**(1) `LogSiegeStuck` is declared and defined here but never *used* here.** Deliberate, per spec (7): *"NO LOGGING FROM `Evaluate`"*. A `UE_LOG` on a per-unit 0.25 s poll at 120 units is exactly the cost this feature claims not to have. TASK-532/533/534 own every call and the pinned `escalate:` / `level=` / `action=` / `stalled=` tokens. ⛔ Please do not read "no reference in this pair" as dead code.

**(2) `SidestepLeaseSeconds` is in `FSiegeStuckTuning` but nothing in this library reads it.** Correct — it is TASK-532's lease duration (`NAV-§8` pins it into the tuning struct so it is not a bare literal at a call site, `NAV-§7` tunables row).

**(3) Same-line `UPROPERTY(...) float X = 1.f;` has no precedent in this project.** Every other `UPROPERTY` in `Source/GitClaudeUnrealTest/` is on its own line. I kept the pin's one-line form because `NAV-§8` is character-for-character law, and I **verified it is UHT-legal against shipping engine source** rather than assuming: `Engine/Plugins/Runtime/EditableMesh/Source/EditableMesh/Public/EditableMeshTypes.h:197-228` uses exactly this shape, including in-class default initializers (`int32 VertexPositionIndex=0;`, `FVector VertexTangent=FVector::ZAxisVector;`).

**(4) `FVector::DistSquared` returns `double`; `FMath::Square(ProgressRadius)` returns `float`.** The comparison promotes the float — standard, no narrowing, no `/W4` warning. **Precedent already in the shipped tree:** `SummonedUnit.cpp:2731`, `GetVelocity().SizeSquared() > FMath::Square(ChargeMoveSpeedThreshold)` — the identical shape, which compiled clean at TASK-526 (0 errors / 0 warnings).

**(5) A large delta may jump straight from level 0 to level 3.** A frame hitch (or a miner polled after a long gap) can make `DesiredLevel` skip rungs. That is the honest reading — the unit really has been stalled that long — and it issues **strictly fewer** requests than climbing rung-by-rung would, so it cannot weaken the bound in §2.

**(6) Non-finite `Location` / `VelocitySizeSq` land on the stalled branch and are NOT specially guarded.** A NaN compares false against every threshold, so a garbage-velocity unit is treated as stalled and the ladder runs on it — bounded by both brakes, so the worst outcome is ≤1 rescue attempt per second on a unit whose velocity is already garbage. I did **not** add a `Location.ContainsNaN()` guard: it is extra work on the hot path for an input no shipped caller can produce (`GetActorLocation()` / `GetVelocity().SizeSquared()`). **Stated as a deliberate decision, not an oversight** — if QA wants the guard, it is three compares and I will add it.

**(7) Encoding / line endings.** Both files are UTF-8 **without BOM** with **LF** endings — matching `SiegeKeyboardLayoutStatics.h` (the most recent precedent, also emoji-bearing, LF-only, no BOM). Note the tree is already mixed: `SiegeCombatStatics.cpp` is CRLF. There is no `.gitattributes`. Flagged for build-master so a whitespace question does not surface at TASK-538.

**(8) The gate's own criteria 2, 3, 5, 6, 7 do not apply to this task** — no timers, no lease, no traversability code, no `.umap`/`Build.cs`/`TilePoolSize`/`AgentRadius`/crowd component, no telemetry — all owned by 532/534/535/529/530.

---

## 6. 🚩 THREE THINGS I THINK ARE WRONG OR INCOMPLETE IN THE SPEC — FLAGGED, AND THE PIN IMPLEMENTED ANYWAY

Per RULING 1: *"A task that finds the pin inconvenient FLAGS IT in its handoff and implements the pin anyway."* All three are implemented exactly as pinned. **None of them is a defect in this file.**

### 🚩 FLAG A — ⛔ **`ComputeSidestepGoal`'s `Attempt` HAS NO PINNED PRODUCER, SO THE ALTERNATION THE SPEC REQUIRES CANNOT ACTUALLY HAPPEN.** *(The one I would most want a manager ruling on.)*

The spec requires the sidestep side to alternate on `Attempt` parity. My function does that, correctly and totally, for any `int32`. **But nothing in the pinned surface can supply a varying `Attempt`:**

- Within one stall, `Sidestep` fires **exactly once** (brake 2 — monotonic). So `Attempt` can only vary **across** stalls.
- `FSiegeStuckState` (pinned, `NAV-§8`) has **no attempt counter**, and `Reset` clears the whole struct on every Abandon and every re-anchor — so nothing survives a stall.
- `ASummonedUnit`'s pinned members are `StuckState`, `StuckTuning`, `SidestepGoal`, `SidestepLeaseRemaining`, `LastStuckTickTimeSeconds`. **None of them is a per-stall counter either.**

⇒ **With the pinned surface as written, TASK-532's only pinned-legal choice is a constant** (most likely `StuckState.EscalationLevel`, which is **always 1** whenever `Sidestep` fires). **A constant `Attempt` means every sidestep for a given unit goes the same way, forever** — so a unit wedged on the left face of a rock sidesteps *into* the rock on every stall, and the alternation is dead code.

**This does not block TASK-531** — my function is correct for whatever it is handed. It needs a manager ruling for TASK-532: either add a small persistent counter to `ASummonedUnit` (a departure from the pinned member list), or accept that `Attempt` is constant and record that the alternation is aspirational. ⛔ I did **not** add a field to `FSiegeStuckState` to fix it — that would be editing the pin, which is exactly what RULING 1 forbids.

### 🚩 FLAG B — ⚠️ THE SPEED TEST IS AN **`OR`**, SO ANY UNIT WITH NONZERO-ISH VELOCITY ESCAPES THE LADDER ENTIRELY, EVEN IF IT MAKES NO NET PROGRESS.

The pinned rung-0 condition is `!bAdvancing **OR** speed ≥ threshold **OR** escaped ProgressRadius`. Because the speed clause is an `OR`, a unit **sliding along a rock's collider** — or oscillating in place — at ≥ 50 uu/s (`MinSpeedSq = 2500`) **re-anchors on every poll and is never rescued**, even though its net displacement is zero. The displacement-from-anchor test alone would catch that case, because it measures *net progress*, which is the thing that actually matters; the speed clause is a cheap early-out that strictly *weakens* it.

⚠️ **I am NOT asserting this is the observed wedge shape.** `NAV-§0` ruling 1 is explicit that Jonathan has **not** tracked whether the wedging is early-match or throughout, and no spec, handoff or QA finding may be written as if either hypothesis were established. This is a **playtest risk to watch**, nothing more. Recorded because if Jonathan reports *"units still wedge on rocks after this ships"*, this is the first thing to check — and the fix is inside the pinned signature (drop the speed clause, or lower `MinSpeedSq`), needing **no re-pin and no recompile** for the tuning route.

### 🚩 FLAG C — ⚠️ THE LADDER NEVER GIVES UP, SO A TRULY UNREACHABLE GOAL CYCLES FOREVER AT ~0.33 req/unit/s.

`Abandon` calls `Reset`, and the standing body re-issues the same move to the same goal next poll (`SummonedUnit.cpp:2440`). The unit re-wedges, and the full 6 s ladder runs again — indefinitely. **That is bounded and by design** (`NAV-§12`: *"The ladder RESOLVES a stall; it does not PREVENT one"*), and it is exactly the steady state the 120-unit arithmetic in §2 describes. Recorded because *"the same unit keeps abandoning"* will otherwise read as a bug report rather than as the designed outcome.

### ➕ NOTE FOR TASK-536 (tests), not a spec defect

**At the shipped defaults, BRAKE 1 NEVER FIRES.** `SecondsSinceEscalation` accumulates from the start of the stall alongside `StalledSeconds`, so it already reads 1.5 when `StalledSeconds` first reaches `SidestepSeconds` — the gate is open and `Sidestep` fires exactly on time; the inter-rung gaps (1.5 s, 3.0 s) both exceed the 1.0 s cooldown. **Brake 1 only bites under retuning**, which is the case it exists for (`FSiegeStuckTuning` is `EditDefaultsOnly`). ⇒ **A test suite that only uses default tuning will never exercise brake 1 at all.** TASK-536 should include at least one case with the thresholds pushed together (e.g. `SidestepSeconds = 1.5`, `WidenSeconds = 1.6`) to prove the cooldown actually gates, plus one with `EscalationCooldown = 0` to prove brake 2 holds the ≤2-requests-per-stall bound on its own.

---

## 7. STATUS

`TASK-531` → **`ready-for-qa`**. Gate: **TASK-537**.

---

# ═══ QA LOOP 1 (2026-08-04) — WARN-1, COMMENT-ONLY CORRECTION ═══

**Status:** `ready-for-qa` (loop 1 of 3) · **Gate: TASK-537**, re-gate is `SC-§27` **diff-scoped**
**QA report fixed against:** `.claude/pipeline/qa/TASK-537.md` — WARN-1 (RULING 2)
**Files touched this loop:** `SiegeStuckStatics.cpp` and `SiegeStuckStatics.h`
⛔ **COMMENT-ONLY. NOT ONE LINE OF LOGIC CHANGED.** No signature, no field, no default, no
control flow, no `#include`. `Evaluate`, `ComputeSidestepGoal` and `Reset` are byte-identical in
behaviour; both brakes are byte-unchanged.

> **adds no replicated property, no new replicated class, no new relevancy tier.**

`FSiegeStuckState` is still unreflected; `FSiegeStuckTuning` is still `EditDefaultsOnly` CDO
config. A comment cannot change either.

## 1. ⛔ MY §2 ABOVE IS SUPERSEDED. THE CLAIM WAS FALSE AND IT WAS MINE

§2 of this handoff says *"The two brakes are independent, and EITHER ALONE bounds the request
rate."* **That is false, and I wrote it.** TASK-536's FINDING 1 measured it and QA re-derived it
independently. ⭐ **Brake 2 bounds the COUNT PER STALL, not the RATE — because a stall's DURATION
is itself a tunable.** `AbandonSeconds` is `EditDefaultsOnly`, so the two-requests-per-stall bound
alone permits those requests to recur as fast as the thresholds allow.

⚠️ **§2's table row *"CEILING, any tuning — ≤ 1 extra path request per unit per second — brake 1"*
is the specific line that is wrong: the ceiling is not "any tuning", it is "any tuning with
`EscalationCooldown ≥ 1.0`."** The `≤ 2 requests per stall` row and the `only non-moving units pay`
row are both correct and stand.

## 2. QA'S DERIVATION, AS SHIPPED INTO THE COMMENTS (their numbers, not a re-estimate)

Poll period `P = 0.25 s` on both drivers. A cycle is `Sidestep -> Widen -> Abandon -> one dead
re-anchor poll` (Abandon's `Reset` drops `bHasAnchor`, so the next poll **must** take the
re-anchor branch and return `None`) — at least **4 polls carrying 2 path requests**.

| | rate | rests on |
|---|---|---|
| **WORST CASE over legal `EditDefaultsOnly` tuning** (`EscalationCooldown = 0`, thresholds `0.25 / 0.50 / 0.75`) | **2.0 requests / unit / s** | nothing — this is the unbraked-rate case |
| **SHIPPED DEFAULTS** (`1.5 / 3.0 / 6.0`, cooldown `1.0`) — 2 requests / 6.25 s | **0.32 / unit / s** | brake 1 + brake 2 |
| **`NAV-§3`'s `≤ 1 / unit / s` ceiling** | holds | ⭐ **`EscalationCooldown ≥ 1.0` ALONE** |
| forbidden per-poll mill, for contrast | 4.0 / unit / s | — |

⇒ **The pathological legal tuning is HALF the mill and TWICE the stated ceiling.** That is why a
false safety property asserted in *shipped source* was worth correcting before the commit.

## 3. THE TWO EDITS

**(a) `SiegeStuckStatics.cpp`, the block above `State.EscalationLevel = DesiredLevel;`** — the
heading *"EITHER BRAKE ALONE BOUNDS THE REQUEST RATE"* is gone. It now labels brake 1 **"TIME —
this is the rate brake"** and brake 2 **"COUNT PER STALL — ⛔ NOT a rate brake"**, carries the
4-poll cycle derivation, the 2.0 / 0.32 / ≤1-conditional-on-cooldown results, and says in terms
that the ceiling is **a property of the TUNING, not a structural property of the ladder**. The
"only non-moving units pay" half is explicitly kept and marked as the part that **is** structural.
The 120-unit scale line was re-derived to match (~38/s at defaults, ~120/s at the ceiling,
~240/s at the pathological tuning).

**(b) `SiegeStuckStatics.h`, the `FSiegeStuckTuning` doc comment** — carried the same false
sentence (*"EITHER ALONE bounds the request rate"*) and gets the same correction, plus the
operative instruction for whoever tunes this next: ⛔ **treat `EscalationCooldown` as the anti-mill
knob and do not lower it below 1.0 without re-deriving the ceiling.** ⚠️ This matters because that
comment sits directly above eight `EditDefaultsOnly` fields Jonathan can change **without a
recompile** — it is the last thing a tuner reads before typing a number.

⛔ Both edits name `qa/TASK-537.md` WARN-1 as the source, so the correction is traceable rather
than looking like a drive-by reword.

## 4. FLAG A IS NOW CLOSED — BY TASK-532/533, NOT BY THIS FILE

My **FLAG A** (*"`ComputeSidestepGoal`'s `Attempt` has no pinned producer, so the alternation
cannot happen"*) was ruled **the BLOCKER**. It is fixed by a free-running `uint8`
`ASummonedUnit::SidestepAttemptCount` — deliberately **outside** `FSiegeStuckState`. ⛔ **Nothing
in `SiegeStuckStatics.{h,cpp}` changed for it**, which is the outcome FLAG A asked for: I refused
to edit the pinned struct then, and the pin is still untouched now. `ComputeSidestepGoal`'s parity
contract was always correct — it is now **reachable**. **FLAG B** (the rung-0 `OR`) and **FLAG C**
(the ladder never gives up) both ship as WARNs with named remedies; no change.

## 5. ⚠️ STILL OWED BY THE MANAGER — ⛔ NOT MINE TO EDIT

`CONVENTIONS.md` `NAV-§3` (`:2479-2482`) still states *"WORST CASE: ≤ 1 extra path request per
unit per second"* **unconditionally**. It must be qualified with *"at `EscalationCooldown ≥ 1.0`"*.
⛔ I did not touch `CONVENTIONS.md` — it is manager-owned. The shipped source and this handoff now
disagree with the law until that lands, and **the source is the corrected one.**

## 6. ⚠️ ONE SIDE EFFECT, DECLARED

Expanding the `Evaluate` comment moved lines below it: `State.EscalationLevel = DesiredLevel;` is
now `:152` (was `:135`), `Abandon`'s internal `Reset(State)` is `:169` (was `:152`), the `Side`
parity line is `:212` (was `:195`), and `Reset`'s body is `:226` (was `:209`). Every citation to
those I could reach in **source files owned by this loop** was updated — mostly to **symbol**
references rather than line numbers, per QA's own NIT-5 (*"grep the symbol, never the line"*).
⚠️ Citations living in **handoffs, `CONVENTIONS.md` and `TASKBOARD.md` are NOT updated** and will
read stale; the symbols are unambiguous.
