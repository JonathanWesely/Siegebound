# TASK-363 — `USiegeCheatManager::SetTestDamageBoost` — the deterministic PIE lever

**Agent:** gameplay-programmer · **Status:** ready-for-qa · **Date:** 2026-08-01
**NO compile · NO Git · NO editor · NO MCP** (per spec). `SummonedUnit.{h,cpp}` **UNTOUCHED** (TASK-360 owns it).

---

## Files touched (both exclusively owned by TASK-363)

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeCheatManager.h`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeCheatManager.cpp`

No other file was read-modified. No asset references introduced.

---

## Pinned signature — character-for-character

```cpp
// CONVENTIONS §7, USiegeCheatManager — UFUNCTION(exec)
void SetTestDamageBoost(float Percent, bool bAllFriendly);
```

**Consumed from TASK-360 (declared there, called here — never redefined):**
`CanReceiveDamageBoost()` · `AddPermanentDamageStacks(int32)` · `ClearPermanentDamageStacks()` ·
`GetDamageBoostPercent()` · `GetPermanentDamageMultiplier()` · `GetTeamId()`.
All six verified **public** in the live `SummonedUnit.h` at review time (public block `:121–450`;
`CanReceiveDamageBoost` `:410`, `AddPermanentDamageStacks` `:426`, `ClearPermanentDamageStacks` `:437`,
`GetPermanentDamageMultiplier` `:448`, `GetDamageBoostPercent` `:154`, `GetTeamId` `:126`).

---

## The four manager rulings, and where each one lives

| Ruling | Implementation |
|---|---|
| Percent→stacks uses the unit's **own** `PermanentDamageBonusPerStack`, never a hardcoded `0.05` | `SiegeCheatManager.cpp` — reflection read, resolved once, evaluated **per unit** so a per-Blueprint override is honoured. The literal `0.05` appears **nowhere** in either file. |
| **`Clear` then `Add`**, never a raw field write | `Unit->ClearPermanentDamageStacks(); if (Stacks > 0) Unit->AddPermanentDamageStacks(Stacks);` — commented as load-bearing. |
| `bAllFriendly == false` targets crosshair/nearest, mirroring `ApplyTestDamage` | Shares `TraceFromCrosshair` and the new `ResolveNearestSearchOrigin` helper with `ApplyTestDamage`. |
| Above-cap values **clamp** (`500` ≡ `400`) | Clamping is inherited from `AddPermanentDamageStacks`, not re-implemented here. |

### Why Clear-then-Add is genuinely required (not stylistic)
`AddPermanentDamageStacks` is **additive** and broadcasts **only on an actual change**. Without the
`Clear`, `SetTestDamageBoost 100` typed twice would give 200%, and typing the same value twice would
broadcast nothing the second time. `Clear` broadcasts **unconditionally**, `Add` then broadcasts the
new value — so every invocation drives `OnDamageBoostChanged` end-to-end and the bar repaints.

---

## The one design decision QA should look at hardest: reflection for the per-stack read

`SetTestDamageBoost` reads `PermanentDamageBonusPerStack` via
`ASummonedUnit::StaticClass()->FindPropertyByName(...)` + `FFloatProperty::GetPropertyValue_InContainer`,
**not** `Unit->PermanentDamageBonusPerStack`.

**Why — this is deliberate, not cleverness for its own sake:** CONVENTIONS §7's registry pins
**functions only**. The UPROPERTY's **access level is not pinned anywhere**, and every other card stat
on `ASummonedUnit` (`AttackDamage`, `Profile`, `bDead`) is declared `private:`. At the time this task
was written, TASK-360 had landed the header's public *function* block but **had not yet declared the
boost UPROPERTYs at all** — so a direct member read was a coin-flip on compiling, in a batch where UBT
compiles the whole module as one unit. The reflection read is access-level agnostic, reads the
**instance** value, and depends only on the property **name**, which *is* pinned verbatim by
CONVENTIONS §4 and already appears verbatim in TASK-360's shipped inline bodies
(`SummonedUnit.h:154` and `:448`).

**If QA prefers a direct read**, the swap is one line, contingent on TASK-360 landing the property
`public:` — behaviourally identical. I did **not** ask TASK-360 to widen the access level, because that
would mean editing a file this task is forbidden to touch and would push a public surface onto
`ASummonedUnit` that nothing in the shipping game needs.

**Failure mode is loud, never silent:** unresolvable property ⇒ `UE_LOG(..., Error, ...)` naming the
property and **refusing** — it never guesses a magic number. Note the clear-only path is placed
**before** this read on purpose, so `SetTestDamageBoost 0 true` (the "boost bar and frame vanish" gate
row) still works even in that failure state.

---

## Rounding: CEIL, not round-to-nearest — and why the whole gate depends on it

The stack grid is coarse (5%/stack), so `101%` is **not representable**. Round-to-nearest maps
`101` → 20 stacks → **100%**, which would silently re-test the row above it and make the gate's
"just past a band boundary" row **impossible to perform**. Ceil maps `101` → 21 stacks → **105%**
(band 2, nearly empty) — exactly what the plan §7 table demands.

An epsilon of `1e-4` **stacks** is subtracted before the ceil so an *exact* boundary never over-ceils
by one. This is not cargo-culted: `0.05f` is really `0.05000000074…`, so `1.0 / 0.05f` = `19.9999997`
and ceil correctly gives 20 — but that lucky sign is a property of the authored value. A per-stack
bonus that happened to round *down* would make the same division `20.0000003`, ceil would return 21,
and `SetTestDamageBoost 100` would silently produce 105%, breaking the "exactly 100% = FULL light
blue" row. The epsilon sits ~20× above the worst-case float error (at the 80-stack cap) and ~2000×
below the finest distinction the gate makes (`100` vs `101` = 0.2 stacks).

Conversion runs in **double**; `FMath::CeilToInt64` is used explicitly because
**`FMath::CeilToInt(double)` returns `int64` in UE5** (`GenericPlatformMath.h:377`) — an implicit
narrow to `int32` would be a C4244. The clamp bounds the value far below `INT32_MAX`, so the cast is
exact by construction.

### Full PIE table verification (plan §7 step 2), computed at the shipped `PermanentDamageBonusPerStack = 0.05f`

| Console | Raw stacks | → stacks | `GetDamageBoostPercent()` | Band / expected render |
|---|---|---|---|---|
| `SetTestDamageBoost 50 true` | 9.99999985 | 10 | **50.0%** | 1 · half light blue |
| `SetTestDamageBoost 100 true` | 19.99999970 | 20 | **100.0%** | 1 · **full** light blue |
| `SetTestDamageBoost 101 true` | 20.19999970 | **21** | **105.0%** | 2 · nearly empty dark blue (Frac 0.05) |
| `SetTestDamageBoost 200 true` | 39.99999940 | 40 | **200.0%** | 2 · full dark blue |
| `SetTestDamageBoost 250 true` | 49.99999925 | 50 | **250.0%** | 3 · half purple |
| `SetTestDamageBoost 300 true` | 59.99999910 | 60 | **300.0%** | 3 · full purple |
| `SetTestDamageBoost 350 true` | 69.99999896 | 70 | **350.0%** | 4 · half black |
| `SetTestDamageBoost 400 true` | 79.99999881 | 80 | **400.0%** | 4 · full black |
| `SetTestDamageBoost 500 true` | 99.99999851 | 100 → **clamped 80** | **400.0%** | 4 · **identical to 400** ✅ |
| `SetTestDamageBoost 0 true` | — (clear-only path) | 0 | **0.0%** | row hidden |

All ten land **exactly**. The 500-row clamp comes from `AddPermanentDamageStacks`, so the checklist
row is meaningful (it proves the *shipping* cap, not a cheat-local one).

---

## Targeting

- **`bAllFriendly == true`** — `GetAllActorsOfClass(ASummonedUnit)`, filtered to `GetTeamId() == MyTeam`
  **and `CanReceiveDamageBoost()`**.
- **`bAllFriendly == false`** — crosshair trace first (must hit a **friendly `ASummonedUnit` that
  `CanReceiveDamageBoost()`**), else nearest eligible friendly from `ResolveNearestSearchOrigin`.

### ⚠️ Two judgement calls — please rule rather than assume they are bugs

1. **Eligibility filters target selection on both paths, including the clear path.** The spec says
   `bAllFriendly == true` ⇒ "every live friendly `ASummonedUnit`"; I filter by `CanReceiveDamageBoost()`
   (which *includes* `!bDead`, so "live" is satisfied). **Rationale:** it is the same predicate
   `AAncientGround`'s tick uses, so the cheat can never grant a stack the real granter would refuse —
   that *is* "route it through the shipping path". **Clearing an ineligible unit is a no-op by
   construction:** an ineligible unit can never have accrued stacks (the ground tick and this cheat
   gate on the identical predicate, and `HandleDeath` clears), so nothing is left stranded. Without
   the filter, `bAllFriendly=false` would happily pick a **Sorcerer** as "nearest friendly" and the
   tester would see nothing happen — the worst possible failure during a human gate.
2. **Aiming at a friendly-but-ineligible unit logs a Warning and falls through to nearest**, rather
   than refusing. One rule, and the tester is told *why* the unit under their crosshair was skipped,
   so the boost never appears somewhere unexplained.

---

## The one edit to a shipped function — please diff this specifically

`ApplyTestDamage` had a 12-line inline block resolving its nearest-search origin. I extracted it
**verbatim** into `ResolveNearestSearchOrigin(APlayerController*)` and called it from both cheats, so
`SetTestDamageBoost` mirrors `ApplyTestDamage`'s targeting by **sharing** the code rather than copying
it. Behaviour is unchanged:

```cpp
// BEFORE (inside ApplyTestDamage)                 // AFTER
FVector Ref;                                       Target = FindNearestEnemy(
if (const APawn* RefPawn = PC->GetPawn())              World, MyTeam, ResolveNearestSearchOrigin(PC));
{   Ref = RefPawn->GetActorLocation(); }
else
{   FVector ViewLocation = FVector::ZeroVector;
    FRotator ViewRotation = FRotator::ZeroRotator;
    PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
    Ref = ViewLocation; }
Target = FindNearestEnemy(World, MyTeam, Ref);
```

The helper adds `PC` null-guards the original did not need (it is only reached after a `!PC` early
return); they are harmless and make the helper safe for any future caller. **If QA judges any edit to
a shipped path out of scope for this task, say so and I will revert `ApplyTestDamage` and inline the
logic in the new function instead** — the duplication is the only cost.

---

## Null-safety / no-crash audit ("never a crash on an empty field")

| Input | Behaviour |
|---|---|
| No owning `PlayerController` | Warning, return |
| No `UWorld` | Warning, return |
| `Percent` NaN or ±inf | Warning, return (**NaN would slip past every ordered comparison into the stack math**) |
| Empty field / no eligible friendly | Warning naming the team + the fix (`SummonTestUnit Footman false`), return |
| `Hit.GetActor()` null | `Cast` → null → `IsValid` false → falls to nearest |
| Property unresolvable | **Error**, return — never guesses |
| A unit with `PerStack <= 0` | Counted, skipped, Warning; other units still processed |
| `Percent <= 0` | Clear-only path, before any per-stack read |

No raw field writes. No per-tick work — every scan is once per typed console command.

---

## Non-shipping by construction

Unchanged from the existing class contract: the engine only instantiates a `UCheatManager` in
non-shipping builds with cheats enabled, so `SetTestDamageBoost` is unreachable in a shipped game.
The class-header comment block was extended with a fourth bullet naming the grant path it routes
through, matching the three existing bullets.

---

## Notes for TASK-365 (QA) and TASK-377 (Jonathan's gate)

- **This task cannot be behaviourally verified until TASK-360 compiles** — every consumed symbol is
  declared there. Pre-compile review is signature-level plus the arithmetic above.
- **`GetDamageBoostPercent()` is used for the readback** (not `GetPermanentDamageMultiplier()`)
  because it is the exact value `UCombatantHealthBarComponent` bands on — so the log line reports
  what the **bar** will show, after the cap clamp. The multiplier is logged too (single-target case),
  which is the direct readback for the plan's "a Footman boosted 4 s deals exactly 12 × 1.20 = 14.4"
  check.
- **Suggested gate ordering:** run the whole boost-bar checklist with `bAllFriendly = true` on a
  single summoned Footman — it removes crosshair aiming from the loop entirely, which is the point of
  the lever on a locked desktop.
- **Not in scope, deliberately:** no `HasAuthority()` guard (M8 P1 unit fleet is server-only, and
  CONVENTIONS §4 explicitly says a guard here buys nothing); no replication of the cheat.
