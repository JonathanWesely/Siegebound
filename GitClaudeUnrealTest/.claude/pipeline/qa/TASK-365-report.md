# QA Report — TASK-365 (ANCIENT-GROUNDS C++ lane pre-compile review)

**Verdict: PASS** — 7 of 7 tasks PASS. **0 BLOCKER · 2 WARN · 4 NIT.**

Reviewer: qa-reviewer · Date: 2026-08-01 · No edits, no engine, no Git (read-only review).
Scope: TASK-358, 359, 360, 361, 362, 363, 364 — all `ready-for-qa`.
Law: `CONVENTIONS.md` "Ancient Grounds + Sorcerer + 180° terrain symmetry (2026-08-01)" §1–§8 + the M8 NET RELEVANCY LAW.
Design authority: `C:\Users\wesel\.claude\plans\there-is-one-new-glittery-bentley.md`.

> **Report path note for build-master:** TASK-366's spec references `qa/TASK-365.md`; the orchestrator's dispatch
> specified `qa/TASK-365-report.md`, which is this file. **This file is the authoritative verdict.** Append compile
> errors here.

---

## Per-task verdict

| Task | File(s) | Verdict | Findings |
|---|---|---|---|
| TASK-358 | `BattlefieldScatter.{h,cpp}` · `ScatterConfig.h` | **PASS** | 1 NIT |
| TASK-359 | `AncientGround.{h,cpp}` (NEW) · `CaptureZone.h` (comment-only) | **PASS** | 0 |
| TASK-360 | `SummonedUnit.{h,cpp}` · `SorcererUnit.{h,cpp}` (NEW) | **PASS** | 1 NIT |
| TASK-361 | `BattlefieldScatter.{h,cpp}` · `ScatterConfig.h` | **PASS** | 2 WARN · 1 NIT |
| TASK-362 | `HealthBarProvider.h` · `CombatantHealthBar{Widget,Component}.h` · `…Component.cpp` | **PASS** | 0 |
| TASK-363 | `SiegeCheatManager.{h,cpp}` | **PASS** | 1 NIT |
| TASK-364 | `DeckBuilderWidget.cpp` | **PASS** | 0 |

---

## (A) DETERMINISM / DRAW ORDER — **PASS**

Full-file draw audit of `BattlefieldScatter.cpp` (`Stream.` / `MineStream.` / `GroundStream.`, every occurrence):

| Line | Stream | Draw |
|---|---|---|
| `:37`, `:46` | layer `Stream` | `SampleBiasedY` magnitude + sign (2) — **pre-existing, unchanged** |
| `:594` | layer `Stream` | primary X — **range narrowed to `[−HalfX, DrawMaxX]`, draw COUNT and ORDER unchanged** |
| `:607` | layer `Stream` | mesh index — pre-existing |
| `:613` | layer `Stream` | scale — pre-existing |
| `:654` | layer `Stream` | yaw (conditional on `bRandomYaw`) — pre-existing |
| `:1442`, `:1443` | `MineStream` | X then Y — **the only two, unchanged by TASK-358** |
| `:1716`, `:1717` | `GroundStream` | X then Y — **the only two** |

- **ZERO RNG draws in the rotation step.** The twin block (`:707–793`) contains no `Stream.` occurrence. The twin
  point, yaw, Z, proxy transform, spacing registration and skip counter are all computed or traced. ✅
- **`PlaceAncientGrounds` makes exactly two draws per attempt in fixed X-then-Y order** from
  `FRandomStream GroundStream(Seed ^ 0x41474E44)` (`:1608`). `grep GroundStream.` → 3 hits total (ctor + 2 draws),
  matching the handoff's claim. Everything downstream is draw-free and I verified each by reading it:
  `ClearsEveryMine` (`:1670`), `ResolveSurfaceZ` (`:1656`), the rotation, both `RemoveBlockingInstancesInDisc`
  calls, the fallback (`:1748–1780`), both `SpawnActor`s, the divergence guard, every log. ✅
- **Layer stream and mine stream gained zero draws.** The ancient-ground feature moves no existing layout:
  every seed keeps its exact mine pair, and the layer field changes only because of TASK-358's transform. ✅
- **Byte-identical log lines on a same-seed re-run.** `GenerateScatter seed=%d mirror=%s …` (`:340`),
  `Layer '%s': placed …` (`:801`), `MinesPass …` (`:1583`), `AncientGroundsPass seed=%d P=… M=… fb=… culls=%d`
  (`:1874`) are all pure functions of the seed + the config + level geometry. No time, no `FMath::Rand`,
  no pointer or name ordering in any value. ✅
- **host == client.** `PlaceAncientGrounds(Seed, bAuthoritativeGenerate)` is placed at `:395`, **before**
  `RunScatterPasses`' `if (!bAuthoritativeGenerate) { … return; }` at `:401`, so the client's
  `OnRep_GenerationIndex` path (`:322`) runs the identical pass off the replicated `ChosenSeed`. `PlaceMines`
  runs before it on both machines, so `ClearsEveryMine` reads an identical mine set. ✅
- **Existing seeds producing new layouts: NOT FILED** (manager ruling 8 / TASK-140 precedent / CONVENTIONS §1).
  The contract that must hold is intra-build reproducibility + host==client, and both hold.

**Independently verified TASK-358's "six sites, not five" claim.** All six `(−P.X, −P.Y)` conversions are present
and paired in `PlaceMines`: `:1356` (`Pm`), `:1468` (keep-clear at the twin), `:1518` (fallback twin `GroundZAt`),
`:1531` (twin clearance disc), **`:1553` (the twin `SpawnActor` — the one the plan omitted)**, `:1575` (log). The
programmer was right that the plan's list was incomplete, and the omitted line is the one that actually moves the
shipped actor.

**The hill-parity rollback deletion is sound.** I checked the load-bearing step of the argument myself: `ScatterLayer`
emits every hill twin **inline during pass 1** (`RunScatterPasses:356–362` places all `bAllowOnHills == false` layers,
the hills included, before `PlaceMines` at `:380`), and `FindHillSurfaceAt`'s slope gate reads **only** `BestNormal.Z`
(`:1257`), which a Z-axis rotation leaves invariant. So `bHillP == bHillM` and `OutZP == OutZM` by construction. The
retained `bHillP != bHillM` branch is a loud Warning that still ships the pair on honestly traced Z — correct residual,
not a re-introduction.

---

## (B) THE AUTHORITY INVERSION — **PASS** (the batch's highest-risk item, all four sub-checks green)

1. **`HasAuthority` appears ZERO times in `AncientGround.cpp`.** Verified by full-file grep: 0 matches — not a call,
   not a comment, not a string. The only occurrence in the pair is `AncientGround.h:65`, inside the class doc that
   *explains the ban*. ✅
2. **`bAuthoritativeBoost` defaults `false` (fail-closed)** — `AncientGround.h:211`. An un-`Init`'d ground never
   boosts. ✅
3. **`InitAncientGround(bool)` is its only writer** — `AncientGround.cpp:130`. `grep bAuthoritativeBoost` returns the
   declaration, that one write, and the single read at `ApplyBoostTick:160`. ✅
4. **TASK-361 threads the flag into BOTH grounds** — `BattlefieldScatter.cpp:1831` (Primary) and `:1841` (Twin),
   both taking `RunScatterPasses`' own `bAuthoritativeGenerate` parameter, never a re-derived value. ✅

**The subtlest correct decision in the batch, and I want it on the record so nobody "fixes" it:** the boost timer is
armed **unconditionally** in `BeginPlay` (`AncientGround.cpp:104–109`) and the **tick body** gates. That is right.
`World->SpawnActor<AAncientGround>()` runs `BeginPlay` *inside* the spawn call, i.e. **before** the scatter gets the
pointer back to call `InitAncientGround` — so an arm-on-the-flag design would leave the flag `false` at arm time **on
the server too** and produce a ground that never boosts anywhere. Gating the body is order-independent and
fail-closed. Cost: one predicted branch per second across exactly two actors.

**Diagnostic accepted and endorsed:** `AncientGroundInit authoritativeBoost=%s P=(…) halfExtent=(…)` (`:137`). On a
client both grounds MUST print `false`; on host/standalone both MUST print `true`. This localizes the fault to the
scatter's threading rather than the actor's gate, for free. TASK-377 should grep it on both machines.

---

## (C) NET RELEVANCY TIER — **PASS**

`AAncientGround` declares **TIER C — NOT REPLICATED** in `AncientGround.h:50–60` **and** in
`handoffs/TASK-359-programmer.md`. `bReplicates` is left at the `AActor` default and never written (deliberate — a
redundant `= false` invites a future "fix"); the constructor carries the Tier-C comment at `:35–38`. No
`GetLifetimeReplicatedProps`, no `OnRep_`. The rationale is correct: the actor holds no replicated truth and rides the
Tier-A `ChosenSeed`/`GenerationIndex`, exactly like the `AGoldNode` mine pair. Its no-collision/no-nav claim is also
correct — `UDecalComponent` derives from `USceneComponent`, not `UPrimitiveComponent`, so it cannot touch the
traversability guarantee. ✅

---

## (D) THE THREE ATTACK-SEAL GUARDS — **ALL THREE VERIFIED BY FILE:LINE** — **PASS**

| # | Site | Line | Code |
|---|---|---|---|
| 1 | `EnterAttack()` | `SummonedUnit.cpp:2050–2054` | `if (!CanEverAttack()) { EnterIdle(); return; }` |
| 2 | `UpdateStateGrouped()` | `SummonedUnit.cpp:1583–1586` | `if (!CanEverAttack()) { CurrentTarget = nullptr; } else { …two AcquireEnemyNearPoint tiers… }` |
| 3 | `PerformAttack()` | `SummonedUnit.cpp:2307` | `if (bDead \|\| !bStatsLoaded \|\| bAIFrozen \|\| bSpellFrozen \|\| !CanEverAttack())` |

**Guard 1 is the real seal and it is placed at the true chokepoint.** I traced every `EnterAttack()` call site:
`UpdateState` legacy Standard body (`:1307`), `UpdateStateStandardCommanded` DEFEND (`:1441`) and ATTACK (`:1507`),
`UpdateStateGrouped` (`:1603`), `UpdateStateSiege` (`:1787` region). All four funnel through `EnterAttack`. Standing
down via `EnterIdle()` rather than a silent `return` is correct: `EnterIdle` (`:2279`) early-returns when already
`Idle` and never calls back into `EnterAttack`, so there is no recursion and no per-tick churn.

**Guard 2's unreachability argument for the third `AcquireEnemyNearPoint` (the HOLD monotone upgrade at `:1562`) —
I checked it and it holds.** That call sits inside `if (CurrentTarget && Group.Type == Hold)` at `:1539`, which runs
*before* the guard in the same invocation. Guard 2 forces `CurrentTarget = nullptr` at the end of **every** grouped
tick, so the block is entered on tick N+1 only if something set the target between the two. I enumerated all 15
`CurrentTarget = ` writes in the file (`:657, :889, :1280, :1288, :1435, :1484, :1489, :1534, :1554, :1564, :1585,
:1591, :1595, :1639, :1778`): for a sealed unit every one either assigns `nullptr`, is inside the `else` guard 2
owns, is `AcquireTarget()`-sourced (dead at `AggroRadius = 0` — `AcquireTarget:1349` rejects every candidate at
`Distance > AggroRadius`), is `AcquireEnemyNearPoint(castle, DefendRadius = 0)` (empty disc), or is the Siege body
(`Profile == Siege`, unreachable for a `Standard` row). **No extra guard term is needed.** Guard 1 backstops all of
them regardless.

**Why three matter, confirmed on disk:** `MinAttackCadence = 0.05f` (`SummonedUnit.cpp:77`) and the row is
`Sorcerer,…,0,0,0,…` (`cards.csv:31`), so `AttackCadence = FMath::Max(Row->Cadence, 0.05f)` would arm a **20 hits/s**
loop for any Cadence-0 unit that reached Attack.

**(E) also verified here:** `CanEverAttack()` (`SummonedUnit.h:390`) and `IsAncientGroundEmpowerer()` (`:400`) are in
the **`public:`** block — the block runs `:121` to `protected:` at `:450`. `AAncientGround` calls both from outside
the hierarchy; `protected:` would not link. ✅

---

## (E) PINNED SIGNATURES (CONVENTIONS §7) — character-for-character — **PASS**

| Pinned symbol | Declared | Consumed by | Match |
|---|---|---|---|
| `virtual bool CanEverAttack() const` | `SummonedUnit.h:390` (**public**) | `AncientGround.cpp` (via `CanReceiveDamageBoost`), `SorcererUnit.h:78` override | ✅ |
| `virtual bool IsAncientGroundEmpowerer() const` | `SummonedUnit.h:400` (**public**) | `AncientGround.cpp:189`, `SorcererUnit.h:81` override | ✅ |
| `bool CanReceiveDamageBoost() const` | `SummonedUnit.h:410` / `.cpp:779` | `AncientGround.cpp:197`, `SiegeCheatManager.cpp:190/437` | ✅ |
| `void AddPermanentDamageStacks(int32 Stacks)` | `SummonedUnit.h:426` / `.cpp:798` | `AncientGround.cpp:226`, `SiegeCheatManager.cpp:552` | ✅ |
| `void ClearPermanentDamageStacks()` | `SummonedUnit.h:437` / `.cpp:822` | `SummonedUnit.cpp:2802`, `SiegeCheatManager.cpp:479/549` | ✅ |
| `float GetPermanentDamageMultiplier() const` **BlueprintPure** | `SummonedUnit.h:447–448` | `.cpp:2471`, `.cpp:2758`, `SiegeCheatManager.cpp:582` | ✅ |
| `float GetDamageBoostPercent() const override` | `SummonedUnit.h:154` | `CombatantHealthBarComponent.cpp:120`, `SiegeCheatManager.cpp:558` | ✅ |
| `FOnCombatantDamageBoostChanged* GetDamageBoostChangedDelegate() override` | `SummonedUnit.h:156` | `CombatantHealthBarComponent.cpp:127` | ✅ |
| `void InitAncientGround(bool bAuthoritative)` | `AncientGround.h:105` / `.cpp:124` | `BattlefieldScatter.cpp:1831/1841` | ✅ |
| `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatantDamageBoostChanged, float, BoostPercent)` | `HealthBarProvider.h:34` | `SummonedUnit.h:146/156`, `CombatantHealthBarComponent.cpp:127` | ✅ |
| `void SetDamageBoost(float FillFraction, float R, float G, float B, float RowOpacity)` BIE | `CombatantHealthBarWidget.h:96` | `CombatantHealthBarComponent.cpp:179/201` | ✅ |
| `void SetTestDamageBoost(float Percent, bool bAllFriendly)` UFUNCTION(exec) | `SiegeCheatManager.h:113` / `.cpp:393` | console | ✅ |

Interface defaults are correct: `GetDamageBoostPercent() { return 0.f; }` and
`GetDamageBoostChangedDelegate() { return nullptr; }` are the **only two non-pure** methods on `IHealthBarProvider`
(`HealthBarProvider.h:88/96`), so `ABuilding` and `AHeroCharacter` need zero changes — **confirmed by module-wide
grep: neither file contains either symbol.** ✅

---

## (F) BOOST CORRECTNESS — **PASS**

- **Integer stacks, never a float** — `int32 PermanentDamageStacks = 0` (`SummonedUnit.h:1209`,
  `VisibleInstanceOnly, Transient, AllowPrivateAccess`). ✅
- **Clamped at `MaxPermanentDamageStacks`** — `FMath::Clamp(PermanentDamageStacks + Stacks, 0, MaxPermanentDamageStacks)`
  (`.cpp:810`). ✅
- **`AddPermanentDamageStacks` broadcasts ONLY on an actual change** — `Stacks <= 0` no-ops (`:805`), an already-capped
  unit returns before the write (`:811`), broadcast at `:819` immediately after the write at `:816`. A capped unit
  standing in a ground forever generates zero per-second bar traffic. ✅
- **`ClearPermanentDamageStacks` broadcasts UNCONDITIONALLY** — `:829` write, `:830` broadcast, no guard. ✅
- **BROADCAST COMPLETENESS — audited, not taken on trust.** Module-wide grep for `PermanentDamageStacks` returns
  **exactly two writers**: `SummonedUnit.cpp:816` and `:829`. **Both broadcast on the next line.** There is no third
  write site anywhere in `Source/`. This is what makes the "every mutation" criterion auditable rather than a promise,
  and it is the single reason the boost bar cannot go stale. ✅
- **Both compose points present** — `ComputeOutputDamage:2471` (`Output *= GetPermanentDamageMultiplier();`, exactly
  one line after the War Banner `Output *= AuraDamageMultiplier;` at `:2461` — covers melee AND ranged AND every
  keyword unit through the one chokepoint) and `ApplyDetonation:2758`
  (`const float BlastDamage = AttackDamage * GetPermanentDamageMultiplier();`, passed to `ApplyRadialDamage` at
  `:2766`). ✅
- **`AttackDamage` NEVER mutated in place** — grep of the whole file: no `AttackDamage =` outside
  `LoadStatsAndStart`'s row bind. Both compose points write a local. ✅ Multiplier is exactly `1.f` at zero stacks, so
  every unboosted unit is bit-for-bit unchanged and the shipped 80-damage Sapper blast is byte-identical.
- **`HandleDeath` clears** — `:2802`, and the **ordering is right**: it sits *after* the `bSuicide → ApplyDetonation()`
  call at `:2781–2784`, so a boosted Sapper's death blast is still boosted and only then are the stacks lost. ✅
- **Match-end freeze deliberately does NOT reset** — confirmed absent, **NOT filed** (CONVENTIONS §4). ✅
- **Seed-then-bind is UNCONDITIONAL-seed-then-CONDITIONAL-bind** — `CombatantHealthBarComponent.cpp:120`
  (`PushDamageBoost(Provider ? Provider->GetDamageBoostPercent() : 0.f);`) then `:125–131` (bind only if the delegate
  pointer is non-null). The unconditional seed is what drives a non-boostable owner's row to `RowOpacity 0` instead
  of its design-time state. ✅

### BOUNDARY EXACTNESS — verified in single precision, and it holds

`GetDamageBoostPercent()` (`SummonedUnit.h:154`) is
`100.f * PermanentDamageBonusPerStack * static_cast<float>(PermanentDamageStacks)`. Multiplication is
left-associative, so `(100.f * PerStack)` evaluates first. `0.05f` is exactly `0.0500000007450580596923828125`;
`100 × that = 5.00000007450580596923828125`. The float spacing at 5.0 is `2^-21 ≈ 4.77e-7`, so the excess `7.45e-8`
is **well under half an ULP** and the product rounds to **exactly `5.0f`**. Then `5.0f × {20,40,60,80}` is exact in
binary: **100.0 / 200.0 / 300.0 / 400.0 with zero error.** The operand order is load-bearing and correctly chosen.

That exactness is what makes TASK-362's banding correct at the boundaries. I recomputed every band:

| BoostPercent | `CeilToInt(B/100)` → Band | `Frac` | Renders |
|---|---|---|---|
| 100.0 | 1 | (100−0)/100 = **1.0** | full light blue ✅ |
| 105.0 | 2 | (105−100)/100 = 0.05 | nearly empty dark blue ✅ |
| 200.0 | 2 | **1.0** | full dark blue ✅ |
| 250.0 | 3 | 0.5 | half purple ✅ |
| 300.0 | 3 | **1.0** | full purple ✅ |
| 350.0 | 4 | 0.5 | half black ✅ |
| 400.0 | 4 | **1.0** | full black ✅ |
| 500.0 (unreachable) | clamped 4 | 2.0 → `Min` → **1.0** | identical to 400 ✅ |

`FMath::CeilToInt(BoostPercent / 100.f)` takes the **float** overload (returns `int32`) — no int64 narrowing, no
C4244. `FMath::Clamp(int32, int, int)`, `FMath::Max(float, float)`, `FMath::Min(float, float)` all deduce cleanly.

---

## (G) FILE OWNERSHIP — **PASS**

- **TASK-362 did not touch `SummonedUnit.{h,cpp}`.** Its four files are `HealthBarProvider.h`,
  `CombatantHealthBarWidget.h`, `CombatantHealthBarComponent.{h,cpp}`. `CombatantHealthBarWidget.cpp` correctly
  unchanged (a BIE has no C++ body). ✅
- **`Building.{h,cpp}` / `HeroCharacter.{h,cpp}` untouched** — the defaulted-virtual promise held; module-wide grep
  finds neither `GetDamageBoostPercent` nor `GetDamageBoostChangedDelegate` in either. ✅
- **TASK-361's edits sit ON TOP of TASK-358's, not beside them.** `ScatterConfig.h` carries TASK-358's
  `EScatterSymmetryMode`/`SymmetryMode` block **and** TASK-361's six `Scatter|AncientGrounds` fields, both intact.
  `BattlefieldScatter.cpp` carries the rotational twin block (`:691–793`), the converted mines (six sites), the
  deleted rollback (`:1374–1419` is now the argument comment + the residual Warning) **and** `PlaceAncientGrounds`
  (`:1588`). No TASK-358 work was reverted or duplicated. ✅
- **TASK-363 and TASK-364** touched only their own files. ✅
- No dangling references to retired symbols: `bMirrorSymmetric` survives only in explanatory comments (6 sites,
  all doc text); `OutInjectedSide` / `OutFootprintCulls` / `inj=` survive only in the comments that record their
  deletion. ✅

---

## (H) THE STANDING COMPILE TRAPS — swept, **PASS**

- **Shadow law (C4457/C4458/C4459 = hard errors).** I enumerated every new local/param/loop variable across all
  eleven files against the inherited reflected members of `AActor`/`ACharacter`/`ASummonedUnit`/`UWidgetComponent`/
  `UCheatManager`/`ASiegeBattlefieldScatter`. **No collisions.** The one shape worth naming: `ScatterLayer`'s twin
  block re-declares `ProxyScaleVec` / `ProxyZ` / `ProxyXf` (`:777–779`), but the primary's copies (`:684–686`) are in
  a **sibling** scope, not an enclosing one — no shadowing. `AAncientGround::ApplyBoostTick`'s `UWorld* World` does
  not shadow anything (`AActor` exposes `GetWorld()`, not a `World` member).
- **Complete-type include law.** `BattlefieldScatter.cpp:18` includes `Siegebound/AncientGround.h` (it dereferences
  `AAncientGround` at `:1831/:1841/:1859` and `Destroy()`s at `:455`). `AncientGround.cpp` includes
  `Siegebound/SummonedUnit.h` (dereferences), `EngineUtils.h` (`TActorIterator`), `Components/DecalComponent.h`,
  `Components/SceneComponent.h`, `Materials/MaterialInterface.h`, `TimerManager.h`, `Engine/World.h`,
  `Siegebound/TeamId.h`, `GitClaudeUnrealTest.h`. `AncientGround.h` includes `Engine/TimerHandle.h` (FTimerHandle by
  value) and `UObject/SoftObjectPtr.h` (TSoftObjectPtr by value) and forward-declares the three pointer-only types.
  `SiegeCheatManager.cpp:13` includes `UObject/UnrealType.h` for `FFloatProperty`/`CastField`.
  `CombatantHealthBarComponent.cpp` includes both `CombatantHealthBarWidget.h` and `HealthBarProvider.h`. ✅
- **No literal `*/` inside a doc comment** in any of the eleven files. ✅
- **`FString::Printf` / `UE_LOG` format strings are all literal `TEXT("…")`** — no computed format string anywhere
  (UE 5.8 `TCheckedFormatString` / the TASK-268 C7595 trap). Verified at `BattlefieldScatter.cpp:1574` (`PairsLog`
  accumulation), `:1874`, `AncientGround.cpp:91/137`, `SiegeCheatManager.cpp:581/587`. ✅
- **`GetFirstPlayerController()` — BANNED, and absent.** Module-wide grep returns three hits, all inside comments
  documenting the ban (`SiegePlayerController.h:401`, `SiegeSessionSubsystem.{h,cpp}`). No new gameplay code calls
  it. `SummonedUnit.cpp:1233/1264` correctly use `ASiegePlayerController::FindControllerForTeam`. ✅
- **Never-per-tick.** `AAncientGround` sets `PrimaryActorTick.bCanEverTick = false` (`:33`) and runs a 1 Hz looping
  timer cleared in `EndPlay` (`:118`). `SetTestDamageBoost` scans once per typed console command. `PushDamageBoost`
  is delegate-driven. No new tick or poll anywhere in the batch. ✅
- **Naming law.** `AAncientGround` / `ASorcererUnit` plain-name gameplay actors in
  `Source/GitClaudeUnrealTest/Siegebound/`; `EScatterSymmetryMode`; `Scatter|AncientGrounds` category;
  `SiegeboundCardGlossary::SorcererRole` + `GlossaryCardID_Sorcerer(TEXT("Sorcerer"))` matching the cards.csv row
  name character-for-character. ✅
- **Null-safety.** Every new dereference is guarded: `ApplyDecalFootprint` early-returns on `!GroundDecal`;
  `ApplyBoostTick` guards `!World` and `IsValid(Unit)`; `PlaceAncientGrounds` guards `!World || !ScatterConfig`,
  both `SpawnActor` results, and `Mine.Get()` inside `ClearsEveryMine`; `PushDamageBoost` returns on a null
  `LiveBar`; `SetTestDamageBoost` guards PC, World, non-finite `Percent`, an unresolvable property, an empty field,
  and a null crosshair hit. ✅ **Two engine-API sanity checks:** `MarkRenderStateDirty()` called from the
  constructor (via `SetFadeScreenSize`/`SetSortOrder`/`ApplyDecalFootprint`) is a no-op on an unregistered
  component — safe; `FMath::CeilToInt64(double)` is the correct explicit-width call in `SiegeCheatManager.cpp:540`.

---

## (I) TRUTH — TASK-364's composed panel vs shipped behavior — **PASS**

I checked every clause of both new glossary strings against code **on disk**, not against the spec:

| Clause | Verified at | ✓ |
|---|---|---|
| "It never attacks — no order will make it strike" | `SorcererUnit.h:78` + the three guards `SummonedUnit.cpp:2050 / :1583 / :2307` | ✅ |
| "an enemy walking into it is ignored" | `SorcererUnit.cpp:23` `AggroRadius = 0.f` ⇒ `AcquireTarget:1349` rejects every candidate | ✅ |
| "it deals no damage of its own" | `cards.csv:31` Damage 0 | ✅ |
| "It still takes your unit orders" | `Profile Standard` in the row + `IsGroupCommandEligible` gate; ctor deliberately leaves `StateCheckInterval` at 0.25 s (`SorcererUnit.cpp:30–35`) | ✅ |
| "While it stands inside an ancient ground" | `AncientGround.cpp:184` zone gate on the empowerer count | ✅ |
| "every friendly unit … in that same ground" | `AncientGround.cpp:223` `SorcererCount[TeamBucketIndex(Unit->GetTeamId())]` — own team only | ✅ |
| "**that fights**" (the narrowing qualifier) | `AncientGround.cpp:197` → `CanReceiveDamageBoost` `SummonedUnit.cpp:792–795` | ✅ |
| "hits harder" (damage only) | both compose points; nothing touches HP or speed | ✅ |
| "for each second it spends there" | `AncientGround.cpp:106–109` 1 Hz looping timer at `BoostTickInterval = 1.0f` | ✅ |
| "kept in full when that unit walks back out" | no removal path exists — nothing in `AncientGround.cpp` subtracts | ✅ |
| "lost only when it dies" | `SummonedUnit.cpp:2802` | ✅ |
| "stacks … to a hard ceiling" | `.cpp:810` clamp to `MaxPermanentDamageStacks` | ✅ |
| "A second sorcerer … builds it twice as fast" | `AncientGround.cpp:193/223` — one stack per friendly sorcerer per tick | ✅ |
| "miners, healers and sorcerers themselves gain nothing" | Miner: Damage 0 (`cards.csv:5`) · Cleric: `Profile Support` (`cards.csv:13`) · Sorcerer: `CanEverAttack` false **and** skipped as an empowerer at `AncientGround.cpp:189–195` | ✅ |

**No clause states anything the code does not do**, and the deliberate narrowing from "every friendly unit" to
"every friendly unit **that fights**" is the *correct* call — the unqualified wording would have been false.
Blast radius is zero: the branch is an `else if (CardID == GlossaryCardID_Sorcerer)` (`DeckBuilderWidget.cpp:876`)
inside the existing role chain that starts at `:864`, reachable only for that exact CardID, with the chain's prior
conditions byte-identical. `AppendStatLines` needed no change (the melee branch at `:836` requires `Row.Damage > 0`
*inside* a `Row.Range > 0` block; both are 0). String literals are ASCII-only; non-ASCII appears only in comments,
which is established house style in this module.

---

## RULINGS on every flagged judgement call

**R1 — TASK-359, the comment-only `CaptureZone.h` edit: ACCEPTED. NOT scope creep.**
Verified comment-only by reading the file. Two additions, both inside `/** … */` blocks: the
`🔧 KNOWN DEBT — 2-MIRROR WITH AAncientGround` paragraph appended to the class doc (`CaptureZone.h:81–93`) and the
`⚠️ PAIRED TUNABLE with AAncientGround::ZoneHalfExtent` note on the property (`:170–172`). **Zero symbols, zero code,
zero behavior, zero compile surface**, revert-safe with two deletions. `CaptureZone.h` is owned by no other task in
this batch. The programmer was right to surface the contradiction rather than silently pick a side, and CONVENTIONS
§2's "record the 2-mirror as debt in BOTH headers" is now genuinely satisfied.

**R2 — TASK-361, `PlaceAncientGrounds(int32 Seed, bool bAuthoritativeGenerate)` (two-arg vs CONVENTIONS §2's
one-arg): ACCEPTED.**
The same §2 clause demands `InitAncientGround` be called with the flag `RunScatterPasses` carries; the two sentences
cannot both be satisfied literally, and the authority law is the load-bearing one. The method is **private**, appears
**nowhere** in the §7 pinned registry (only `InitAncientGround(bool)` does), and has no other caller — so the extra
parameter has **zero cross-task compile surface**. The rejected alternatives are both worse: a member stash is
order-coupled and one refactor from a stale `true` on a client; a self-`HasAuthority()` read would return the *right*
answer here (this actor really is replicated) but would split one authority decision into two sources that can
drift — and would model the exact pattern `AAncientGround` must never copy.
→ **Manager action (not a blocker):** amend CONVENTIONS §2's one-arg wording to the shipped two-arg form so the
next reader does not re-open it.

**R3 — TASK-361, no keep-clear disc test: ACCEPTED (conclusion). See WARN-1 (the cited arithmetic is wrong).**
I recomputed this against the shipped config rather than the plan. `CastleKeepClearRadius = 1500.f`
(`ScatterConfig.h:354`) and `PlayerStartKeepClearRadius = 800.f` (`:358`) — **not the 4500 the plan and the code
comment cite.** At the real values the castle disc bites at `|X| ≥ 23500` and the band ceiling is `21000`, so a disc
test would be a **provable no-op**, clearing by 2500 uu at the center and 1660 uu at the 21840 footprint edge. The
PlayerStart disc at `(−23800, 0)` r=800 is 2800 uu clear. The spawn box (`SpawnBoxHalfExtent = (2460, 2460)`,
`Castle.h:260`) starts at `|X| = 22540`, 700 uu beyond the footprint edge. **The ceiling really is the exclusion in
closed form; the test is correctly omitted.** The ground is also no-collision/no-nav, so keep-clear — a
traversability construct — does not apply to it in the first place.

**R4 — TASK-361, the pass emits the rotational pair regardless of `SymmetryMode`: ACCEPTED.**
"One per side, placed symmetrically" is a **fairness law** from Jonathan's directive, not a terrain-aesthetics
toggle. `Asymmetric` is a retired off-state that already needs a fresh Jonathan ruling to select at all, and an
asymmetric single-ground mode is unspecified — inventing one would be the actual scope violation. Branching here
would also let a designer hand one team an objective the other cannot reach.
→ **Manager action:** one line in CONVENTIONS §2 recording that the ancient-ground pass is `SymmetryMode`-independent
by design.

**R5 — TASK-358, twin yaw is now unconditionally `Fmod(Yaw + 180, 360)`: ACCEPTED.**
CONVENTIONS §1 states the transform with **no `bRandomYaw` exception**. The old
`Layer.bRandomYaw ? Fmod(Yaw+180,360) : 0.f` was a fake-reflection artifact: a proper rigid rotation rotates the mesh
too, so a fixed-yaw twin left at yaw 0 would face the wrong way. Correct behavior change, correctly flagged.

**R6 — TASK-358, the per-layer loop runs `DivideAndRoundUp(InstanceCount, 2)` pairs: ACCEPTED.**
CONVENTIONS §1 is explicit that `TargetCount` counts primary + twin ("target 340 becomes ~170 pairs"). The old
`bMirrorSymmetric` path ran `InstanceCount` iterations *and* emitted a twin from each — a latent **count-doubler**
in a mode that never shipped. The halved budget is what makes the ~15,000 `AddInstance` perf budget identical to
today's asymmetric field, which is the plan's explicit claim. Density per half is unchanged (`InstanceCount` over the
full area ≡ `InstanceCount/2` over half), each iteration keeps the full `MaxPlacementAttemptsPerInstance`, and the
odd-target +1 overshoot is documented in-code and on the log line. Both behavior changes correctly surfaced.

**R7 — TASK-362, the defensive `FMath::Min(Frac, 1.f)`: KEEP IT. Do not delete.**
It is behavior-neutral for `0 < B <= 400` (I verified the whole in-range table above) and unreachable through the
shipping path today — but `FMath::Clamp` caps the band **index**, not the numerator, so it is the only thing standing
between a future >400% source and a `SetPercent` argument greater than 1. Two lines of comment, zero cost, real
protection. The programmer offered to delete it; **the answer is no.**

**R8 — TASK-362, no `EndPlay` unbind for the boost delegate: ACCEPTED as parity.**
The shipped HP binding (`HandleOwnerHPChanged`, since TASK-130) has no unbind either; component and owner die
together, and dynamic delegates drop bindings to destroyed UObjects. Adding one here **alone** would be the
inconsistency. Adding both is a legitimate hygiene follow-up, out of scope for this task.

**R9 — TASK-363, CEIL rounding (+ the 1e-4 stack epsilon): ACCEPTED, and it is load-bearing.**
I recomputed all ten PIE rows in double at `PerStack = 0.05f`:
`50→10 (50.0%) · 100→20 (100.0%) · 101→21 (105.0%) · 200→40 · 250→50 · 300→60 · 350→70 · 400→80 · 500→100 clamped
to 80 (400.0%) · 0→clear`. **All ten land exactly as the handoff's table states.** Round-to-nearest would map
`101 → 20 stacks → exactly 100%`, silently re-testing the row above it and making the gate's most important row
(*"snaps near-empty; fill AND outline flip to dark blue"*) **impossible to perform**. The epsilon's sizing argument
is also correct and correctly *not* cargo-culted: it is ~20× above the worst-case float error at the 80-stack cap
and ~2000× below the finest distinction the gate makes.

**R10 — TASK-363, the reflection read of `PermanentDamageBonusPerStack`: ACCEPTED AS SHIPPED for this batch.**
It is access-level agnostic, reads the **instance** value (so a per-Blueprint override is honored), depends only on
a property **name** that CONVENTIONS §4 pins verbatim, and fails **loud** — an unresolvable property logs `Error` and
**refuses**, never guessing a magic number. The clear-only path is deliberately placed *before* it so
`SetTestDamageBoost 0 true` still works in that failure state. And the reasoning was sound at authoring time: the
property's access level is not pinned anywhere, and every other card stat on that class is `private:`. It is
nonetheless a workaround — see the SIMPLIFICATION verdict.

**R11 — TASK-363, extracting 12 lines out of the shipped `ApplyTestDamage` into `ResolveNearestSearchOrigin`:
ACCEPTED. Do NOT revert.** I diffed the semantics line by line. The helper (`SiegeCheatManager.cpp:110–123`) is
behaviourally identical on the only reachable path: `ApplyTestDamage` early-returns on `!PC` at `:315`, so the added
null guards are dead there; the original's `FVector Ref` was assigned on **both** branches, so the helper's
zero-initialized fallback changes nothing. The single call site is `:349`, in the same position and with the same
argument the inline block produced. Sharing beats copying here, and the second caller (`:455`) is the point.

**R12 — TASK-364, qualitative magnitudes: ACCEPTED.**
CONVENTIONS §8 ("interpolated from those properties **or stated qualitatively** — never a hardcoded number that can
drift") is newer and card-specific; it beats the file's older GLOSSARY-MIRROR RULE, and the task spec restates it
verbatim. Interpolation genuinely does not compile: `PermanentDamageBonusPerStack` (`SummonedUnit.h:652`) and
`MaxPermanentDamageStacks` (`:662`) are both inside the `protected:` block that opens at `:450` — I confirmed this
on disk. The string carries **no number at all**, and the real values plus the exact re-interpolation instruction
are recorded at the declaration. The two comment-only edits to the glossary header (the stale "28 descriptions"
count and the mirror-rule exception note) are correct and in scope.

**R13 — TASK-360, the `CardID = FName(TEXT("Sorcerer"))` ctor line: ACCEPTED.**
Exact `AMinerUnit` precedent (`MinerUnit.cpp:32`) — the row identity *is* the class's nature. Non-breaking in every
direction (TASK-375's BP sets the same value, and the deferred `InitUnit(Team, "Sorcerer")` agrees), and every
**stat** still binds from `DT_Cards` at BeginPlay. No change requested.

---

## THE SIMPLIFICATION VERDICT — **YES, two public getters would be the better design**

`PermanentDamageBonusPerStack` and `MaxPermanentDamageStacks` landing in `protected:` forced **two independent
workarounds in the same batch** — TASK-363's reflection read and TASK-364's qualitative text, which will silently go
stale the day 5%/400% is retuned (and retuning is *expected*: CONVENTIONS §4 names both as balance levers). Neither
workaround is wrong; the access level is what is costing us.

**Concretely, as a scoped follow-up task (NOT a blocker on this batch — do not hold the compile for it):**

Add to `ASummonedUnit`'s **public** block, beside `GetPermanentDamageMultiplier()` (`SummonedUnit.h:448`):

```cpp
UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
float GetPermanentDamageBonusPerStack() const { return PermanentDamageBonusPerStack; }

UFUNCTION(BlueprintPure, Category = "Siegebound|Unit")
int32 GetMaxPermanentDamageStacks() const { return MaxPermanentDamageStacks; }
```

Call sites that simplify:

1. **`SiegeCheatManager.cpp`** — delete the reflection block entirely: `:500–508` (`CastField<FFloatProperty>` +
   `FindPropertyByName` + the "refusing to guess" `Error` path), the `SiegeCheatPerStackPropertyName` constant
   (`:60`), and the now-unneeded `#include "UObject/UnrealType.h"` (`:13`). Replace `:519` with
   `const float PerStack = Unit->GetPermanentDamageBonusPerStack();`. Net **−15 lines**, still per-instance, still
   honors a per-Blueprint override, and one fewer failure mode to test.
2. **`DeckBuilderWidget.cpp:71`** — `SorcererGroundBoost` becomes a `…Fmt` string + `FString::Printf` interpolating
   `GetDefault<ASummonedUnit>()->GetPermanentDamageBonusPerStack() * 100.f` (⇒ "+5%") and
   `GetMaxPermanentDamageStacks() * GetPermanentDamageBonusPerStack() * 100.f` (⇒ "+400%"). That moves the card off
   CONVENTIONS §8's fallback branch onto its **preferred** branch and permanently removes the drift risk. This is the
   only line that changes, exactly as the programmer's own comment predicts.
3. *(Optional, low value)* `CombatantHealthBarComponent.cpp:195`'s "80 == exactly +400%" comment could become live —
   leave it; the component should not couple to the unit class for a comment.

Both getters are trivially inlinable, add no state, and are exactly the shape `GetPermanentDamageMultiplier()`
already established one line above them.

---

## Findings

- **[WARN] `BattlefieldScatter.cpp:1629–1634` (+ `handoffs/TASK-361-programmer.md §8`, + plan §2's band table) —
  the no-keep-clear-disc rationale cites a castle keep-clear radius that does not exist.** The comment argues
  *"the |X| ≤ 21000 ceiling is the castle keep-clear (r=4500 at ±25000) in closed form … 25000 − 21000 = 4000, so a
  disc test would actively contradict the specced band edge."* The shipped values are
  `CastleKeepClearRadius = 1500.f` (`ScatterConfig.h:354`) and `PlayerStartKeepClearRadius = 800.f` (`:358`). At the
  real values a disc test is a **no-op**, not a contradiction: the disc bites at `|X| ≥ 23500`, 2500 uu beyond the
  ceiling. **The decision is correct and ACCEPTED (R3); only the derivation is wrong** — and it is wrong in the
  direction that could mislead: a future tuner reading it would believe the band edge already sits *inside* a
  keep-clear disc (it does not, by 2500 uu) or that 4500 is the binding constraint on raising
  `AncientGroundMaxAbsX` (it is not — `SpawnBoxHalfExtent 2460` ⇒ the box edge at `|X| = 22540` is, leaving 700 uu
  past the 21840 footprint edge). *Suggested fix (comment text only, no behavior):* replace the sentence with the
  real numbers. Manager: the same 4500 figure appears in the approved plan's §2 table and should be corrected there
  so it is not re-derived next batch.

- **[WARN] `handoffs/TASK-361-programmer.md §1` — the "3-second grep for QA" is factually wrong.** It states
  `grep -c "HasAuthority" BattlefieldScatter.cpp → 0` and *"There is no HasAuthority() anywhere in
  BattlefieldScatter.cpp — not before this task, not after."* There are **two**: `:210` (`BeginPlay`) and `:237`
  (`GenerateScatter`). **Both are correct, shipped M8 D9 guards** on an actor that genuinely *is* replicated, and
  neither is in or near `PlaceAncientGrounds`. **No code change.** Recorded because build-master's verification grep
  will return `2`, and an unqualified `2` against a handoff promising `0` reads as a regression when it is not. The
  invariant that actually matters — zero `HasAuthority` in `AncientGround.cpp`, and the pass threading rather than
  re-deriving — is **verified green** (section B).

- **[NIT] `BattlefieldScatter.cpp:1792–1797` (TASK-361)** — the `FlatParityBreaks` `SymmetryAssert` reports
  *"broke on %d of %d candidate(s)"* against `MaxGroundAttempts` (the literal 48) rather than attempts actually
  consumed, so an early accept would read "1 of 48". Cosmetic, in a line that is provably unreachable on a healthy
  field. *Fix if ever touched:* report against the loop's final `Attempt` count.

- **[NIT] `BattlefieldScatter.cpp:571–573` (TASK-358)** — `Placed` can exceed `Layer.InstanceCount` by 1 on an odd
  target (341 ⇒ 171 pairs ⇒ 342). Documented in-code and in the handoff; recorded here only so the `placed 342
  (target 341…)` log line is not filed as a defect at TASK-366.

- **[NIT] `SiegeCheatManager.cpp:538–540` (TASK-363)** — a positive but sub-resolution `Percent`
  (below ~0.005% at the shipped 0.05/stack) drives `RawStacks − epsilon <= 0`, so the command clears and logs
  *"applied 0 stack(s) → bar reads 0.0%"* for a positive request. Correct by magnitude and ~4 orders of magnitude
  below anything the PIE gate types. Recorded only so it is not discovered as a surprise.

- **[NIT] `SorcererUnit.cpp:12` (TASK-360)** — the `CardID` ctor assignment beyond the literal spec.
  **RULED IN** (R13); no action.

---

## Notes for build-master (TASK-366)

1. **Compile the batch as ONE unit.** Several files cannot compile standalone **by design** and that is not a
   defect: `SummonedUnit.h` consumes `FOnCombatantDamageBoostChanged` + the two defaulted interface virtuals from
   TASK-362's `HealthBarProvider.h`; `AncientGround.cpp` calls three `ASummonedUnit` methods TASK-360 declares;
   `SiegeCheatManager.cpp` calls six of them; `BattlefieldScatter.cpp` needs `AAncientGround`. I verified all
   twelve pinned signatures match **character-for-character across the declaring and consuming tasks** (section E),
   so the batch links.
2. **Expect a wide recompile.** `HealthBarProvider.h` is included by `SummonedUnit.h`, `Building.h`,
   `HeroCharacter.h`, `CombatantHealthBarWidget.h` and `SiegeCheatManager.cpp`. Adding *defaulted* (non-pure)
   virtuals is source-compatible for all of them — no existing override changes.
3. **No DataAsset edit is required.** `DA_BattlefieldScatter` silently drops the retired `bMirrorSymmetric` on load
   and takes `SymmetryMode`'s default `Rotational180` — **that default flip IS the behavior change this batch
   ships.** It also gains the six `Scatter|AncientGrounds` properties at their C++ defaults, which **are** the law.
4. **Determinism proof (your step 3): grep these four lines and diff two same-seed runs.**
   `GenerateScatter seed=… mirror=rot180 layers=… corridorHalfY=…` ·
   `Layer '…': placed … (target …, sym=rot180, pairs …, twinSkipped=0, zMismatch=0, …)` ·
   `MinesPass seed=… mineStream=… …` · `AncientGroundsPass seed=… P=(…) M=(…) fb=no culls=…`.
   **`twinSkipped`, `zMismatch`, and every `SymmetryEscape` / `SymmetryAssert` token must be 0 / absent on a healthy
   run** — a non-zero value is a real signal, not noise. Confirm `P` and `M` on the AncientGroundsPass line are
   **exact `(−X, −Y)` antipodes** and that two `AAncientGround` actors exist.
5. **Log-format changes to expect (all intentional, none breaking your greps):** `mirror=` keeps its KEY, its VALUE
   is now `rot180` / `asymmetric`; the MinesPass **`inj=` token is GONE** (its injection was deleted) and `culls=`
   is now exactly `CullsP + CullsM`; the layer line gained `sym=`, `pairs`, `twinSkipped=`, `zMismatch=` while every
   pre-existing key is untouched.
6. **Authority spot-check, cheap and worth it:** grep `AncientGroundInit` — **standalone/host must print
   `authoritativeBoost=true` on both grounds.** (Client `false` is TASK-377's two-machine check.)
7. **Measure nav settle time (your step 4).** TASK-358 changes the *distribution* of dirty area, not its total, but
   the ~178 s TASK-349 figure must be re-measured, not assumed.
8. **Known interim visual, NOT a regression (TASK-362 §5):** `DrawSize` is now `(90, 22)` while
   `WBP_CombatantHealthBar` is still a single `ProgressBar` until TASK-367+368, so **every overhead health bar
   renders ~2× taller** in your boot-PIE smoke. It self-corrects the moment the `BarStack` tree exists. Do not file it.
9. **`SetDamageBoost` on an unimplemented BIE is a safe no-op** — the boost row simply does not paint before
   TASK-368. Nothing in the batch requires the widget tree to exist to compile or run.

## Notes for the manager (no task is blocked on these)

- **CONVENTIONS §2 amendment ×2:** record the shipped two-arg `PlaceAncientGrounds(int32 Seed, bool
  bAuthoritativeGenerate)` (R2), and record that the ancient-ground pass is `SymmetryMode`-independent by design (R4).
- **Correct the `r=4500` castle figure** in the approved plan's §2 band table and in the code comment (WARN-1). The
  shipped radius is 1500; the binding constraint on `AncientGroundMaxAbsX` is the 2460 spawn box, not a castle disc.
- **Scoped follow-up worth filing:** the two public getters (SIMPLIFICATION verdict above). It retires TASK-363's
  reflection read and moves TASK-364's card text onto CONVENTIONS §8's preferred interpolation branch, removing a
  known drift risk on two FLAGGED balance levers.
- **Optional hygiene follow-up:** add `EndPlay` unbinds for **both** `UCombatantHealthBarComponent` delegate
  bindings (HP *and* boost) together, or neither (R8).
- **Recorded, not filed (all correct by absence):** no `Reset…()`/`SiegeGameMode` edit for `AAncientGround`; no
  `RegroundAncientGrounds`; no `AncientGroundClass` config field; no MID on the ground decal; match-end freeze does
  not clear the boost; `Building`/`HeroCharacter` untouched; `ValidateTraversability`'s culls stay side-agnostic and
  now log `SymmetryEscape` when they fire.
