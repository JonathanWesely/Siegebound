# TASK-360 — [AG-T2] `ASummonedUnit` boost state + compose points + NEW `ASorcererUnit` with the 3-point attack seal — programmer handoff

Status: **ready-for-qa** (files only — **NOT compiled**, no Git, no editor, no MCP, per the task's hard limits; TASK-366 compiles).
Implemented against **CONVENTIONS §3 + §4 + §7** of "Ancient Grounds + Sorcerer + 180° terrain symmetry (2026-08-01)" and Jonathan's approved plan `C:\Users\wesel\.claude\plans\there-is-one-new-glittery-bentley.md` **§3 + §4** (the plan file wins over board summaries).
Base tree: `main` @ `10f14de`, all target files clean at start. I took and held **exclusive ownership of `SummonedUnit.{h,cpp}`** for this batch — I touched no other existing file.

---

## Files touched

| File | Kind |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | edited |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | edited |
| `Source/GitClaudeUnrealTest/Siegebound/SorcererUnit.h` | **NEW** |
| `Source/GitClaudeUnrealTest/Siegebound/SorcererUnit.cpp` | **NEW** |

No `.Build.cs` edit is needed — the module has no explicit source list (UBT globs the directory), verified in `GitClaudeUnrealTest.Build.cs`.

---

## ⚠️ CROSS-TASK COMPILE DEPENDENCY (read this first)

`SummonedUnit.h` now **consumes** three symbols that **TASK-362 declares** in `HealthBarProvider.h` (PINNED, CONVENTIONS §7) and that I deliberately did **not** define:

- `FOnCombatantDamageBoostChanged` (the `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam`)
- `IHealthBarProvider::GetDamageBoostPercent()` — defaulted virtual
- `IHealthBarProvider::GetDamageBoostChangedDelegate()` — defaulted virtual returning a **pointer**

**This task's files cannot compile alone; they compile as one UBT module WITH TASK-362.** That is the designed contract, not a defect. `SummonedUnit.h` already includes `Siegebound/HealthBarProvider.h` (line 9, pre-existing), so no include change was required.

---

## Part-by-part against the 9-item spec

### (1) Two new virtuals — in the **`public:`** block ✅

`SummonedUnit.h:365-448` — a single labelled `//~ ─── ANCIENT GROUNDS ───` block appended to the **end of the public block**, immediately after `GetCommandGroupId()` and immediately **before** `protected:`.

- `virtual bool CanEverAttack() const { return true; }` — `SummonedUnit.h:390`
- `virtual bool IsAncientGroundEmpowerer() const { return false; }` — `SummonedUnit.h:400`

**NOT** placed beside `ShouldHoldDeathAnim()` (now `SummonedUnit.h:709`, which is `protected:`) — the manager's ruling-2 correction. The block carries an explicit in-code comment stating *why* it is public (`AAncientGround` calls these from outside), so a future agent cannot "tidy" it back into `protected:` without reading the reason.

### (2) THREE attack-seal guard points — all three present ✅

| # | Site | New line | What it does |
|---|---|---|---|
| 1 | `EnterAttack()` | `SummonedUnit.cpp:2050-2054` | `if (!CanEverAttack()) { EnterIdle(); return; }` — **stands down**, does not silently return |
| 2 | `UpdateStateGrouped()` | `SummonedUnit.cpp:1583-1597` | forces `CurrentTarget = nullptr` **instead of** the two `AcquireEnemyNearPoint` tiers (now in the `else`) |
| 3 | `PerformAttack()` | `SummonedUnit.cpp:2307` | `\|\| !CanEverAttack()` appended to the existing `bDead / !bStatsLoaded / bAIFrozen / bSpellFrozen` gate |

**Guard 1 recursion/idempotence check (please verify):** `EnterIdle()` (`SummonedUnit.cpp:2279`) early-returns when `State` is already `Idle` and never calls back into `EnterAttack()`, so the stand-down is a one-shot per state transition — no recursion, no per-tick churn.

**Guard 2 completeness check (please verify):** `UpdateStateGrouped` has a **third** `AcquireEnemyNearPoint` call — the HOLD monotone-upgrade at `SummonedUnit.cpp:1562`. It is **unreachable** for a sealed unit rather than separately guarded, because it sits inside `if (CurrentTarget && Group.Type == Hold)` and guard 2 forces `CurrentTarget` null **every** tick before the ladder proceeds; nothing else can set it for a sealed unit (the legacy `AcquireTarget` path is acquisition-dead at `AggroRadius = 0`). The reasoning is written into the code comment at `:1579-1582` — if QA disagrees, the fix is one extra `CanEverAttack()` term on the `Hold` condition.

### (3) Boost state — **integer** stacks ✅

- `int32 PermanentDamageStacks = 0` — `SummonedUnit.h:1209`, `UPROPERTY(VisibleInstanceOnly, Transient, …AllowPrivateAccess)`, in the private runtime-state block beside the other transient stat state.
- `float PermanentDamageBonusPerStack = 0.05f; // GDD §x.x` — `SummonedUnit.h:652`, `EditAnywhere`, in the **protected** `Siegebound|Keywords` mechanic-rule group (the Charge/Slayer/BattleCry magnitudes are the placement precedent).
- `int32 MaxPermanentDamageStacks = 80; // GDD §x.x` — `SummonedUnit.h:662`, same group.
- `float GetPermanentDamageMultiplier() const` — `SummonedUnit.h:448`, **`UFUNCTION(BlueprintPure)`**, `= 1.f + PerStack * float(Stacks)`.

**Deliberate operand order, worth a QA eye:** `GetDamageBoostPercent()` computes `100.f * PermanentDamageBonusPerStack * float(Stacks)`, i.e. `(100 × 0.05f)` **first**. `100.f * 0.05f` rounds to exactly `5.0f` in IEEE-754 single precision, so 20/40/60/80 stacks yield **exactly** 100.0 / 200.0 / 300.0 / 400.0 — which is the entire point of integer stacks and is what makes TASK-362's `CeilToInt(B/100)` banding land on band 1 (not 2) at exactly 100%.

### (4) Mutation + eligibility ✅

`SummonedUnit.cpp:779-831`, placed as a block immediately after `EndAuraDamageBuff()` (grouping it with the damage-modifier family).

- `CanReceiveDamageBoost()` = `!bDead && CanEverAttack() && AttackDamage > 0.f && Profile != ECardProfile::Support` — `:779-796`.
- `AddPermanentDamageStacks(int32)` — `:798-820`. `Stacks <= 0` no-ops; clamps to `MaxPermanentDamageStacks`; **broadcasts only when `NewStacks != PermanentDamageStacks`**, so a capped unit standing in a ground forever generates no per-second bar traffic.
- `ClearPermanentDamageStacks()` — `:822-831`. Zeroes and **broadcasts unconditionally**.
- **Authority is commented, not guarded** (as instructed): the in-code comment at `:800-804` states that the sole gameplay caller is `AAncientGround`'s tick, which gates on its own PUSHED `bAuthoritativeBoost` flag, and that a `HasAuthority()` guard here would buy nothing in M8 P1 where units are server-only.

### (5) Compose points — TWO, `AttackDamage` never mutated ✅

- **(a)** `ComputeOutputDamage()` — `SummonedUnit.cpp:2471`: `Output *= GetPermanentDamageMultiplier();`, exactly one line after the War Banner `Output *= AuraDamageMultiplier;`. Covers melee + ranged + every keyword unit through the one chokepoint. Exactly `1.0` at zero stacks, so the function's shipped "returns `AttackDamage` bit-for-bit for a non-keyword, un-auraed unit" contract is preserved.
- **(b)** `ApplyDetonation()` — `SummonedUnit.cpp:2758`: `const float BlastDamage = AttackDamage * GetPermanentDamageMultiplier();`, passed to `ApplyRadialDamage` at `:2766` (was raw `AttackDamage`). **This is the deliberate behavior change to a shipped unit** (Sapper). `AttackDamage` itself is still never written — the multiplier composes into a local, same as the aura.
- Charge/Slayer/Aura are **deliberately not** retro-applied in `ApplyDetonation` — recorded as a separate pre-existing gap, not fixed here (stated in-code at `:2756-2757`).

### (6) Reset ✅

`ClearPermanentDamageStacks()` in `HandleDeath()` — `SummonedUnit.cpp:2802`, immediately after the existing `OnHPChanged.Broadcast` at `:2792`.

**Ordering is load-bearing and intentional:** it sits **after** the `bSuicide → ApplyDetonation()` call at `:2781-2784`, so a boosted Sapper's death blast is still boosted (compose point b) and only *then* does the unit lose its stacks.

### (7) `IHealthBarProvider` overrides live HERE (not in TASK-362) ✅

- `FOnCombatantDamageBoostChanged OnDamageBoostChanged` — `SummonedUnit.h:146`, `UPROPERTY(BlueprintAssignable)`, declared right after `OnHPChanged`.
- `virtual float GetDamageBoostPercent() const override` — `SummonedUnit.h:154`, inline; returns **percent (0..400)**, not a multiplier.
- `virtual FOnCombatantDamageBoostChanged* GetDamageBoostChangedDelegate() override` — `SummonedUnit.h:156`, returns `&OnDamageBoostChanged` (never null — units *are* boostable).

**Broadcast-site audit (the "miss one and the bar is stale forever" criterion): `PermanentDamageStacks` has exactly TWO writers in the whole module** — `AddPermanentDamageStacks` (`:816`) and `ClearPermanentDamageStacks` (`:829`) — and **both** broadcast on the line immediately after the write (`:819`, `:830`). There is no third write site; `grep PermanentDamageStacks` returns only those two mutators, the inline getters, and the declaration. That two-writer discipline is what makes the criterion auditable rather than a promise.

### (8) M8 P2 duty — recorded, not omitted ✅

`SummonedUnit.h:1200-1206`, in the `PermanentDamageStacks` doc comment: it becomes `UPROPERTY(ReplicatedUsing = OnRep_PermanentDamageStacks)` when the unit fleet replicates, with the OnRep re-broadcasting `OnDamageBoostChanged` so the client's seed-then-bind path is identical to the server's. It also records that in M8 P1 units are server-only, so no remote client sees a boost bar at all (board ruling 13 — a known P1 state, not a defect of this task).

### (9) NEW `ASorcererUnit` ✅

`SorcererUnit.h` (85 lines) + `SorcererUnit.cpp` (37 lines).

- `ASorcererUnit : public ASummonedUnit`, `UCLASS()`, `GITCLAUDEUNREALTEST_API`.
- Ctor: `AggroRadius = 0.f`, `DefendRadius = 0.f`.
- `virtual bool CanEverAttack() const override { return false; }` — THE SEAL.
- `virtual bool IsAncientGroundEmpowerer() const override { return true; }`.
- **`StateCheckInterval` is NOT touched** — it stays at the base 0.25 s, and the ctor carries an explicit comment saying *do not* copy `AMinerUnit`'s `StateCheckInterval = 0` seal onto this class, because a commandable unit needs its state timer to resolve its group and walk to its station.
- **No spawn-path change.** Verified the claim rather than assuming it: `ASiegePlayerController::ResolveCardActorClass` (`SiegePlayerController.cpp:3083-3088`) composes the soft class path and accepts it on `ActorClass->IsChildOf(RequiredBase)` — `ASorcererUnit` passes that for `ASummonedUnit` by inheritance. `Profile` stays `Standard` in `cards.csv` (already shipped by the manager, `Docs/Data/cards.csv:31`), which is what keeps `IsGroupCommandEligible()` (`SummonedUnit.cpp:1648`, gating on `Profile == ECardProfile::Standard`) true.

**One deliberate addition beyond the literal spec, flagged for a QA ruling:** the ctor also sets `CardID = FName(TEXT("Sorcerer"))`. This is the `AMinerUnit` per-card-class precedent (`MinerUnit.cpp:32`) — the row identity *is* the class's nature. It is non-breaking in every direction (TASK-375's BP sets the same value, and the deferred-spawn `InitUnit(Team, "Sorcerer")` agrees), and it means a BP that forgets the field still binds its stats. **Every stat still binds from `DT_Cards` at BeginPlay, never from code.** If QA rules it out of scope, deleting that one line changes nothing else.

---

## What is **NOT** needed — so QA does not hunt for it

1. **The legacy Standard body and the DEFEND stance need no guard.** Both are already dead for a Sorcerer via the ctor: `AggroRadius = 0` makes `AcquireTarget()` reject every candidate at any distance > 0, so `CurrentTarget` is never acquired and the Standard body never enters Attack (this also covers the ONE synchronous `UpdateState()` that `LoadStatsAndStart` runs inside `Super::BeginPlay`); `DefendRadius = 0` makes the DEFEND branch's `AcquireEnemyNearPoint(own castle, 0)` disc empty, so it falls back to marching home. Guard 1 in `EnterAttack()` is the backstop for both regardless.
2. **Siege and Detonate paths are unreachable for a Sorcerer** — `UpdateStateSiege()` is only dispatched for `Profile == Siege` (the row is `Standard`), and `Detonate()`/`ApplyDetonation()` only run for `bSuicide` (false). The `ApplyDetonation` compose point exists for the **Sapper**, not the Sorcerer.
3. **No `SiegeGameMode` / Play Again work.** Play Again's step 2 destroys every unit, so stacks die with the actors.
4. **Match-end freeze deliberately does NOT reset the boost** — a permanent boost survives to the end screen (CONVENTIONS §4). Not a miss.
5. **No `HasAuthority()` guard on the mutators** — by design (see part 4). The authority gate lives on `AAncientGround`'s pushed flag, TASK-359's file.
6. **No `.Build.cs` change** — UBT globs the module directory.
7. **No `ABuilding` / `AHeroCharacter` change** — the `IHealthBarProvider` boost virtuals are *defaulted* by TASK-362, and those two files are not mine.

---

## Things I want QA to scrutinise

1. **`public:` placement of the two virtuals** (`SummonedUnit.h:390`, `:400`) — the single highest-consequence detail in this task; protected would fail to link against `AAncientGround`.
2. **All three guard points cited above by file:line**, per ruling 6 — QA must confirm each, not just `EnterAttack`.
3. **Guard 2's unreachability argument** for the third `AcquireEnemyNearPoint` (the HOLD monotone upgrade at `:1562`) — reasoning is in the code; disagree and it is a one-term fix.
4. **The `ApplyDetonation` change is intentional** and changes shipped Sapper behavior when (and only when) the Sapper carries stacks. Multiplier is exactly 1.0 otherwise, so the shipped 80-damage blast is byte-unchanged for every unboosted Sapper.
5. **Pinned-signature conformance** against CONVENTIONS §7, character-for-character — nine symbols, none "improved". I added **no** unpinned public API to `ASummonedUnit`.
6. **The `CardID` ctor line in `SorcererUnit.cpp`** — the one deliberate addition beyond the literal spec (justified above); a ruling either way is fine.

---

## Cross-task notes (not defects, not my files)

- **For TASK-363 (`SetTestDamageBoost`) and the TASK-367/377 human gate:** stacks are **5%-quantised by construction**, so an arbitrary percent is not exactly representable. The plan's PIE table calls for `SetTestDamageBoost 101` to snap to a **nearly-empty band-2 (dark blue)** bar. That only holds if the cheat rounds **UP** (`CeilToInt(Percent / 5)` ⇒ 21 stacks ⇒ 105% ⇒ band 2); a `RoundToInt` would give 20 stacks ⇒ exactly 100% ⇒ **full band-1 light blue**, and the gate step would read as a failure of the *feature* when it was a rounding choice in the *lever*. Flagging early so it is decided rather than discovered at the human gate. `GetDamageBoostPercent()` is public and is the readback for asserting the result.
- **For TASK-362:** my `GetDamageBoostPercent()` returns **percent 0..400**, matching the `FOnCombatantDamageBoostChanged(float BoostPercent)` payload and the `Band = Clamp(CeilToInt(B/100), 1, 4)` banding — not a 1.0-based multiplier.
- **For TASK-359:** `CanReceiveDamageBoost()` returns false for a Sorcerer on its own (`CanEverAttack()` false **and** row Damage 0), so the ground's "count empowerers, then `continue`" skip is belt-and-braces rather than the only thing preventing self-boost.
- **Balance (already flagged as batch item iii, restated because this task owns the levers):** two of the three brakes live in my file — `MaxPermanentDamageStacks` (80) and `PermanentDamageBonusPerStack` (0.05); the third is `AAncientGround::BoostTickInterval`. All three are `EditAnywhere`/UPROPERTY, so tuning at TASK-377 needs no code change.

---

## Verification performed (no compile — per the task's hard limits)

- Read-back of every inserted region in context; brace balance and control flow of the `UpdateStateGrouped` `if/else` restructure checked by eye against the original.
- `grep` audit of all 13 new symbols across the three files (every declaration, definition and use site enumerated).
- Non-ASCII sweep: the four files contain **no** zero-width/NBSP characters (an early draft of one doc comment used a zero-width space to dodge a nested `*/`; it was rewritten, and the scan is clean).
- `ECardProfile::Support` confirmed against `CardRow.h:31-37`.
- `MinAttackCadence = 0.05f` confirmed at `SummonedUnit.cpp:77` (the 20×/s justification for three guards).
- `IsGroupCommandEligible()`'s `Profile == Standard` gate confirmed at `SummonedUnit.cpp:1648`.
- Spawn-path `IsChildOf(RequiredBase)` confirmed at `SiegePlayerController.cpp:3084`.
