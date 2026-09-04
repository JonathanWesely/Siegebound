# TASK-812 — [ST-1] The two upgrade series + the scale-exclusion predicate

**Agent:** gameplay-programmer · **Status:** ready-for-qa · **Date:** 2026-09-02
**Law:** `STACK-§1` (the series) · `STACK-§2` (the exclusion) · `STACK-§5` `J-10`/`J-4`/`J-6` · `STACK-§7` (names + M8) · `SC-§33` · `SC-§36.1` · `SC-§37` · `HIGH-§1`
**Gate:** TASK-814 (QA) · **Caller:** TASK-813 · **Compile:** TASK-816 owns Lane B's only one — ⛔ nothing compiled here.

---

## 1. Files touched (⛔ three, and ⛔ none of them collide with any other task in this batch)

| File | What |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/Building.h` | +2 pure statics, +1 virtual predicate, +1 getter, +1 mutator, +2 `EditDefaultsOnly` tunables, +2 private state members |
| `Source/GitClaudeUnrealTest/Siegebound/Building.cpp` | the three function bodies, parked directly beneath the `MaxHP` bind (`:249-252`) |
| `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h` | the `false` override ⛔ ONLY — one line of code + its reason |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeBuildingStackTest.cpp` | **NEW** — 10 tests |

⛔ `SiegePlayerController.{h,cpp}` **untouched** · ⛔ no WidgetBlueprint · ⛔ no mesh/art · ⛔ no `cards.csv` · ⛔ no `TOWER-§8.*` symbol (no socket, no climb line, no rung plane, no standoff) · ⛔ no `HIGH-§` edit · ⛔ no `M_Ghost` · ⛔ no compile, editor, MCP or Git. Airlock: no `Capture()`/`EnsureSnapshot()`, Zone A untouched, no token figure.

---

## 2. THE TWO SERIES, AS WRITTEN — and the cap behaviour

```cpp
static float ABuilding::StackHeightMultiplier(int32 UpgradeCount);   // min(1 + n, MaxStackHeightMultiplier)
static float ABuilding::StackHealthMultiplier(int32 UpgradeCount);   // StackHealthStep ^ n, UNCAPPED
```

- **HEIGHT — ADDITIVE, `+1 ×` the ORIGINAL per upgrade, SATURATING.** ×1, ×2, ×3, ×4, **×5** and then frozen. Integer `FMath::Min`, then **one** widening to float — "the cap is reached exactly" is a promise only integer arithmetic can keep.
- **HEALTH — MULTIPLICATIVE, `1.5ⁿ`, ⛔ UNCAPPED.** Computed by **repeated multiplication, ⛔ not `FMath::Pow`**: 1.5 and its low powers are exactly representable in binary32 (3ⁿ/2ⁿ, exact through n = 15), so `1.5² = 2.25` and `1.5³ = 3.375` — **the two numbers he wrote himself** — come back bit-for-bit rather than as powf's rounding of them. The `!IsFinite` break is a runaway guard (fires only past float overflow, n ≈ 1750), ⛔ **not a cap**.
- **AT THE CAP his own sentence rules:** `ApplyStackUpgrade()` past the cap **still succeeds**, height does not move, `MaxHP` still ×1.5, `StackUpgradeCount` keeps advancing (`J-6`). ⛔ A refusal there would be the silent behaviour change `STACK-§5` refused.

### ⚖️ The ruling I built, and why it is not a coin flip (`J-0`)
Under the **doubling** reading the reachable heights are ×2, ×4, ×8, ×16 — **Jonathan's own ×5 cap is UNREACHABLE**, so the ceiling sentence he wrote would describe a state the game can never enter. Under the enumeration ×5 lands **exactly**, on the 4th upgrade. One reading makes his cap sentence mean something; the other makes it dead text. ⭐ **The HEALTH half of his summary sentence is correct in BOTH readings and is shipped verbatim** — only the height half is loose, and it is flagged to him as `J-0`, not silently discarded.

### Tunables (`HIGH-§1` — each with its consequence beside it in the header)
- `int32 MaxStackHeightMultiplier = 5` — `EditDefaultsOnly`, `ClampMin = 1`. **Integer**, because the cap must be *landed on*.
- `float StackHealthStep = 1.5f` — `EditDefaultsOnly`, `ClampMin = 1.0`. The comment **says** it is uncapped by design, and there is deliberately **no** `MaxStackHealthMultiplier` beside it.

### ⚠️ ONE DESIGN CALL QA SHOULD RULE ON — the tunables are read from the **CDO**
`STACK-§7` pins the series at **one parameter, none defaulted** *and* pins the two tunables **`EditDefaultsOnly`**. Those two clauses jointly **force** a CDO read (`GetDefault<ABuilding>()`) inside the statics — there is no other implementation that satisfies both, and the `HeightAdvantageMultiplier` precedent of passing tunables as parameters is closed off by the pinned arity. Consequences, **declared rather than discovered later**:
- ⭐ It is still headless: a CDO read costs **no world and no actor instance**, and `SiegeClimbableTowerTest`/`SiegeGhostPawnTest` already read CDOs under "⛔ zero world".
- ⚠️ It makes the cap and the step **game-wide**: a BP child that re-authored either would be **ignored by the series**. That matches his sentence (a statement about the game, not a per-card stat), and it is written into `MaxStackHeightMultiplier`'s declaration.
- Both statics degrade to the **identity** on a null CDO, and neither restates the tunable as a fallback literal (`HIGH-§1`: exactly one copy of each number in the codebase).

---

## 3. THE EXCLUSION PREDICATE — structural, and where it is overridden

```cpp
// Building.h (public)
virtual bool CanScaleFootprint() const { return true; }

// ClimbableTower.h (public) — the ONLY code this task added to that file
virtual bool CanScaleFootprint() const override { return false; }
```

`STACK-§2`'s reason is written on **both** declarations: a scaled `SM_WatchTower` moves `LadderFoot`/`LadderTop` and the rung plane, which fires **`TOWER-§8.5a`'s own VOIDING CONDITION** ⇒ the deck-breach licence dies, `§8.5`'s outright refusal applies, and **the climb stops working ENTIRELY** — worse than "the deck gets too high". Plus at ×2 the lean goes **76.0° → 82.87°** (×5 → 88.1°) and the rung pitch doubles ⇒ the shipped `A_SiegeBiped_Climb` grips air, the F5 defect class verbatim, with every readback still correct.

⭐ **ONE predicate covers BOTH consumers** and that is a measurement, not a convenience: the upgrade scales Z, the wheel scales X/Y, and the voiding condition fires on **any** non-uniform scale. TASK-815 uses the **same** predicate — ⛔ not a second check.

⛔ **No `CardID` string compare anywhere on this path**, and it is asserted rather than promised (test 6(d)): `Building.h`, `Building.cpp` and `ClimbableTower.h` name `"WatchTower"` **zero** times on code lines, and `ClimbableTower.h` reads `CardID` **zero** times. The scanner's self-check proves it *can* find the token — it finds the one legitimate occurrence, a socket-diagnostic `UE_LOG` in `ClimbableTower.cpp:414` (a **message**, not a branch). ⭐ `ATower` — the ArrowTower/BombTower/BallistaTower/CrystalTower family he actually meant — is asserted **`true`**, so the exclusion costs him nothing he asked for.

---

## 4. 🧑 WHAT ELSE I SHIPPED, AND WHY — read this one first, QA

The board spec item (5) pins the **health** application here and says nothing explicit about the **height** application. I shipped **both**, inside one mutator:

```cpp
bool ABuilding::ApplyStackUpgrade();   // ⛔ deliberately NOT a UFUNCTION
```

**The argument, so this reads as a derivation and not a land-grab:**
1. `MaxHP`/`CurrentHP` are **private** with `AllowPrivateAccess`. The health half **cannot** live in the placement path, which is exactly why `STACK-§5` `J-10` says "apply it where `MaxHP` is already set". ⇒ a mutator on `ABuilding` is definitionally owed by this task.
2. TASK-813 §2 says the confirm does "`StackUpgradeCount++`, then TASK-812's two multipliers applied". `StackUpgradeCount` is private too ⇒ **813 cannot increment it** without this mutator. So the mutator is the surface `SC-§36.1` boarded the pair around.
3. Splitting the height half out would force the controller to reach `VisualMesh` (protected) **and** the authored-baseline Z, i.e. more shipped surface in the placement path to do less. One upgrade, one place.

If QA rules the height half out of scope it is a **three-line deletion** (the `if (VisualMesh)` block) plus test 8 — but TASK-813 then needs a replacement seam and I'd rather that finding land here than at integration.

**What `ApplyStackUpgrade()` does, in order:** refuse on `!HasAuthority()` (M8) · refuse on `bDestroyed` · **refuse on `!CanScaleFootprint()`** · capture the authored Z once · `++StackUpgradeCount` · set `Scale.Z = AuthoredHeightScaleZ × StackHeightMultiplier(n)` (**Z only** — `J-4`, "keeping the same width and length"; X/Y are the wheel's and are inherited verbatim) · `MaxHP ×= StackHealthMultiplier(1)`, `CurrentHP += (NewMax − OldMax)` (`J-10` — grants the new hit points, ⛔ does **not** repair damage) · push through the **existing** `OnHPChanged` (⛔ no second push).

Three sub-decisions worth QA's eye:
- **The predicate is re-asked at the building**, not only in the placement path. Two independent mechanisms on purpose: a future caller that forgets `STACK-§2` must still be unable to fire the voiding condition.
- **The height is RECOMPUTED from a captured baseline, ⛔ not multiplied in place.** An in-place `*= 2` gives the same wrong number as the doubling reading, and the cap could then only be approached, never landed on. `AuthoredHeightScaleZ` is **not** a cached multiplier — it is the "original height" the additive series is *defined against*; without it an additive series cannot be expressed as a transform at all. It is captured **lazily on the first upgrade**, not at `BeginPlay`, which makes it independent of the placement path's spawn/scale ordering and is safe because the wheel scales **X/Y only**.
- **The health step is taken through the SERIES** (`StackHealthMultiplier(1)`), not off `StackHealthStep` directly, so the number a HUD preview shows and the number the building actually gains can never disagree.

---

## 5. ✅ The two banked measurements — VERIFIED, ⛔ not redone

- **`HIGH-§` does not apply to buildings at all.** `HIGH-§4` pins the selector as `bRanged == true` **AND** `CardType == Unit` ⇒ a ×5 `ArrowTower` gains **ZERO** damage from height. ⛔ No `HIGH-§` edit, ⛔ no balance spill. (`HIGH-§3`'s "never a special case for towers" pays out again — the rule never learned towers exist, so a tall one cannot break it.)
- **Collision + navmesh scale for free.** `Building.h:43-51` — `VisualMesh` is the root with an explicit `BlockAll` profile and `SetCanEverAffectNavigation(true)` ⇒ a scaled component carves a scaled hole with zero new code. Confirmed in the constructor at `Building.cpp:37-55`. A Z-only stack changes **no XY nav footprint at all**.

---

## 6. M8 DECLARATION (`STACK-§7`) — explicit, ⛔ not boilerplate

`StackUpgradeCount` is **AUTHORITATIVE GAME STATE** — it drives `MaxHP`. ⇒ it is **server-set at confirm** and **the client may never author it**. `ApplyStackUpgrade()` refuses on `!HasAuthority()` with a warning, and is **deliberately not a `UFUNCTION`** so no Blueprint-callable entry point onto it exists at all (the guard is the belt; not shipping the button is the braces). ⛔ **No new RPC. ⛔ No new relevancy tier.** The resulting HP rides the already-shipped `OnHPChanged` push (TASK-130). Nothing in this task is client-local presentation, so none of the "client-local pre-gate" boilerplate from other batches applies.

---

## 7. TESTS — `SC-§37`. **My delta: +10.**

⚠️⚠️ **THE ABSOLUTE TOTAL IS A MOVING TARGET AND I AM REPORTING IT MEASURED, NOT ASSUMED — TASK-814/816 MUST RECONCILE, ⛔ NOT ADD MY NUMBER TO THE BRIEFED ONE.** The briefed baseline was **306**, which I confirmed by count at dispatch. Between then and now **two other Lane-A/B files landed in parallel**: `SiegeCardHandKeyLabelTest.cpp` (**+5**, TASK-807) and `SiegePlacementTest.cpp` (**+10**, TASK-735). ⇒ counted just now across `Siegebound/Tests/`:

| | count |
|---|---|
| baseline at my dispatch | 306 |
| + TASK-807 `SiegeCardHandKeyLabelTest.cpp` | +5 |
| + TASK-735 `SiegePlacementTest.cpp` | +10 |
| + **TASK-812 `SiegeBuildingStackTest.cpp` (mine)** | **+10** |
| **live total, measured** | **331** |

⭐ **My contribution is unambiguously +10.** "306 → 316" would have been the honest number in isolation and is **wrong as an absolute** — the difference is other people's landed work, not mine, and TASK-813/815 will move it again before the compile.

**Frame check (`SC-§37` "check first"): there is ⛔ no existing `ABuilding` test file.** `SiegeClimbableTowerTest.cpp` (14 tests) is a **`TOWER-§`** frame — the ladder, the refused subsystems, the `HIGH-§3` parity — and `STACK-§` is a new namespace whose subject is the *base class*. ⇒ new file `Tests/SiegeBuildingStackTest.cpp`, ⛔ not a duplicate frame. **I added nothing to `SiegeClimbableTowerTest.cpp`** and verified my new members cannot disturb it: its member scans are `ExcludeSuper` (so `ABuilding`'s additions never enter `AClimbableTower`'s list), `CanScaleFootprint` is not a `UFUNCTION` so it enters no reflection table, and none of the new names contain its banned tokens (`Occupan`/`Garrison`/`Capacit`/`Full`/`Ranged`/`Fall`/`Land`).

| # | Test | Asserts |
|---|---|---|
| 1 | `HeightIncrementsAreConstantWhileHealthRatiosAreConstant` | the two series are different **in kind**: height increments constant, health **ratios** constant, height ratios **not** constant, health increments **not** constant |
| 2 | `TheHeightCapIsReachedExactlyAndTheDoublingReadingCanNeverReachIt` | ⭐⭐⭐ **the decisive one** — see §8 |
| 3 | `HeightSaturatesAtTheCapWhileHealthGrowsWithoutBound` | 20 terms past the cap: height frozen **and** health strictly increasing, in the **same** loop |
| 4 | `BothSeriesAreExactlyIdentityAtZeroUpgradesAndClampNegativeInput` | ⚠️ the **trivial** rows, deliberately isolated: identity at n=0, clamp below 0, and that the runaway break is a guard not a cap |
| 5 | `TheSeriesReadTheirEditDefaultsOnlyTunablesAndAreNotUFunctions` | cap is `int32`; both are `EditDefaultsOnly`; saturation **equals** the tunable and `Health(1)` **equals** the step (⇒ no second literal); ⛔ neither series is a `UFUNCTION`; no cached-multiplier member exists |
| 6 | `CanScaleFootprintIsFalseOnTheClimbableTowerTrueElsewhereAndNamesNoCard` | `false` on `AClimbableTower`, `true` on `ABuilding` **and on `ATower`**; ⭐ asked **through an `ABuilding*`** (a shadowed non-virtual passes every other row and fails that one); zero `"WatchTower"`/`CardID` on code lines |
| 7 | `TheUpgradeGrantsTheNewHitPointsAndNeverRepairsExistingDamage` | on a **damaged** instance (300/120): MaxHP ×step, CurrentHP moves by the **delta**, and the missing HP is **identical** before and after — at n=1 **and** n=2 |
| 8 | `TheUpgradeScalesZOnlyFromTheAuthoredBaselineAndNeverAccumulates` | authored Z=2.0, wheel-set X/Y=1.4: Z tracks **baseline × series**, X/Y **untouched**, and at n=2 it is **not** the doubling/in-place answer |
| 9 | `AtTheHeightCapTheClickStillBuysHealth` | walks to the cap, then one more: height frozen **exactly**, MaxHP still ×step, count still advancing (`J-6`) |
| 10 | `AClimbableTowerRefusesTheUpgradeAtTheBuildingItselfNotOnlyInThePlacementPath` | refuses; **no-op** (count, Z, MaxHP, CurrentHP all unmoved); stable across 6 attempts; ⭐ with a control `ABuilding` proving the **identical call succeeds** |

**Every assertion can fail, and the instruments are self-checked:** the series are proven **non-constant** before "the increments are equal" is claimed · the reflection walk is proven live on `InitBuilding` before two absences are claimed · the source scanner is proven able to **find** `"WatchTower"` before four zeroes are claimed · the scratch root is proven to be a `UStaticMeshComponent` before any scale is measured · and test 10's refusal is proven to be **about the tower** by a control building taking the same call successfully.

---

## 8. ⭐ MY N ≥ 2 DISCRIMINATING ROWS — the ones that die if the wrong series ships

**The blind spot is asserted, not asserted around.** Test 2 opens with two *passing* rows: `Height(0) == Doubling(0)` (both ×1) and `Height(1) == Doubling(1)` (both ×2). ⇒ **n ≤ 1 discriminates nothing**, and the file says so in an assertion rather than a comment.

| Row | n | Shipped | Rejected reading | Where |
|---|---|---|---|---|
| shipped series ≠ doubling series | **2** | ×3 | ×4 | 2(a) |
| shipped series ≠ doubling series | **3** | ×4 | ×8 | 2(a) |
| shipped series ≠ doubling series | **4** | ×5 | ×16 | 2(a) |
| height **ratios** are not constant | **2** | 3/2 ≠ 2/1 | constant 2 | 1(b) |
| health ≠ **additive** health | **2**–5 | 2.25 | 2.00 | 1(d) |
| applied mesh Z ≠ doubling / in-place `*=` | **2** | 6.0 | 8.0 | 8(b) |
| applied MaxHP ≠ additive health | **2** | 675 | 600 | 7(c) |
| ⭐⭐⭐ **the cap is reached EXACTLY** | **4** | `Height(4) == 5.0`, tolerance **ZERO**, with `Height(3) < 5` strictly and `Height(5..10) == 5` | — | 2(b) |
| ⭐⭐⭐ **doubling STEPS OVER the cap** | — | independently built `2ⁿ` walked 24 terms: **no term equals 5**, and it goes 4 → 8 straight past it | — | 2(c) |

The last two together are the whole `STACK-§1` argument, mechanised: his stated maximum is **unhittable** under the summary reading and **hit exactly** under the enumeration.
⚠️ Row 2(c) is guarded: if `MaxStackHeightMultiplier` is ever retuned **to a power of two** the "unreachable" argument genuinely stops applying, so the test emits a **warning naming the retune** instead of going quietly red on legitimate work. Every other row is unconditional.
⛔ **No expectation in the file is transcribed from the spec.** Every number is either re-derived from the `EditDefaultsOnly` tunables (so a retune moves code and expectation together) or independently constructed in the fixture as the *rejected* reading.

---

## 9. 🔍 WHAT QA SHOULD SCRUTINISE

1. **§4 — the height half of `ApplyStackUpgrade()`.** The one judgment call. Rule it in or out; the argument and the cost of removing it are both above.
2. **§2 — the CDO read inside the pinned statics.** Forced by `STACK-§7`'s two clauses in conjunction, but it is the only place I read state inside a "pure" seam. The game-wide-vs-per-card consequence is declared in the header.
3. **`AuthoredHeightScaleZ`.** Is it a "second copy of the height scale" under `STACK-§7`? My reading: no — it is the baseline, not a multiplier, and an additive series has no transform expression without it. Lazily captured so no spawn-ordering assumption is baked in.
4. **`MaxHP ×= StackHealthMultiplier(1)` vs `×= StackHealthStep`.** Identical value; I routed it through the series so the previewed number and the granted number share one expression. If QA prefers the literal tunable read, it is a one-token change.
5. **Test 4 is deliberately trivial** (n = 0 and negative input). It is separated and labelled so a reviewer cannot mistake it for a discriminating row.
6. **Overflow/NaN guards.** `FMath::Clamp(UpgradeCount, 0, Cap)` before the `1 +` (an unclamped `INT32_MAX + 1` wraps **negative** ⇒ an inverted mesh); `!(Step >= 1.f)` written NaN-safe (NaN fails every comparison and must land in the guard, not in `MaxHP`).
7. **Statless buildings.** `MaxHP == 0` (the missing-row failure mode) stays 0 through an upgrade. Intentional and commented — 0 × anything is 0.

## 10. ⚠️ DECLARED, ⛔ NOT FIXED (out of fence)

- **`STACK-§3`'s projectile-muzzle question is TASK-813's**, by its own spec item (6). I did not investigate it and make no claim either way.
- **The ×5 UV stretch is real, shipped as-is and named** (`J-11`). No task, no re-author.
- **`BeginPlay` is never exercised by these tests** (no world in this directory), so the `DT_Cards` HP bind and the team-material step are not covered here — only the arithmetic on top of them.
- **No replication is exercised.** The M8 guard is asserted as *present* and as *not blocking the server path*; it is not proven to reject a real client.
