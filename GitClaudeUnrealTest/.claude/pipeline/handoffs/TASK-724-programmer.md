# TASK-724 — [HIGH-2] THE ELEVATION DAMAGE BONUS — programmer handoff

**Agent:** gameplay-programmer · **Status on delivery:** `ready-for-qa` · **Gate:** TASK-730 · **Compile/suite/commit:** TASK-731
**Law applied:** `HIGH-§1` `§2` `§3` `§4` `§5` · `WM-§8d` · `SHIP-§9` · `SC-§33` · `SC-§15` · `AS-§12g`

**Jonathan, verbatim:** *"make their attacks deal more damage the higher elevation they are. I would say that for every 5 feet that their elevation increases, their damage multiplier increases by 10%."*

---

## 1. THE PINNED CONSTANT — AS WRITTEN, WITH ITS COMMENT

`Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h:923` (declaration comment at `:897-922`):

```cpp
/**
 *  ⭐⛔⛔ HIGH GROUND (TASK-724, HIGH-§1) — THE ELEVATION STEP, IN UNREAL UNITS, AND THE
 *  ARITHMETIC IS WRITTEN OUT HERE SO ⛔ NOBODY EVER RE-DERIVES IT:
 *
 *      Jonathan said "for every 5 FEET". "5 feet" is ⛔ NOT an engine unit.
 *      Unreal is CENTIMETRES, and 1 uu = 1 cm.
 *      1 ft = 30.48 cm  (exact, by international definition)
 *      5 ft = 5 × 30.48 = 152.4 cm  ⇒  ⭐ 152.4 uu
 *
 *  ⛔⛔ 152.4 IS THE ONLY NUMBER. ⛔ NOT 150 ("close enough" — wrong by 1.6% and it looks
 *  like a designer's round number), ⛔ NOT 152, ⛔ NOT 500, and ⛔⛔ ABOVE ALL NOT 5 —
 *  feet-as-units is wrong by 30×, it would make every unit on the field a god, and it
 *  would look entirely plausible in review. ⭐ This comment exists so a future "tidy-up"
 *  cannot round the value away without reading why it is not round.
 *  Tests/SiegeHighGroundTest.cpp asserts this default against 5 × 30.48 re-derived from
 *  the foot definition — the regression claim that catches exactly that tidy-up.
 *
 *  ⭐ EditDefaultsOnly BECAUSE THE TUNABLE IS THE POINT: he chose 5 ft and 10% in prose,
 *  so his next sentence retunes both WITHOUT a code change. A mechanic RULE, therefore ⛔
 *  NEVER a cards.csv column (the mechanic-rules-aren't-card-stats law; the Charge / Slayer
 *  / BattleCry magnitudes above are the precedent).
 *
 *  0 (or negative) disables the bonus cleanly — HeightAdvantageMultiplier returns exactly
 *  1.0 rather than dividing by zero.
 */
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|HighGround", meta = (ClampMin = "0"))
float HeightBonusStepUU = 152.4f; // HIGH-§1: 5 ft × 30.48 cm/ft
```

and beside it at `:936`:

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|HighGround", meta = (ClampMin = "0"))
float HeightBonusPerStep = 0.10f; // HIGH-§1: his "+10%"
```

**⭐ EXACTLY ONE `152.4` LITERAL EXISTS IN THE CODEBASE. Pasted grep (`grep -rn "152\.4" Source/ Docs/ Config/`), code lines only:**

| File:line | Kind |
|---|---|
| `SummonedUnit.h:904` · `:906` | **PROSE** — the derivation comment and the trap list |
| **`SummonedUnit.h:923`** | ⭐ **THE ONE CODE LITERAL** |
| `Tests/SiegeHighGroundTest.cpp:30,34,42,83,88,164,178,540,584` | **PROSE ONLY** (comments, test-name headers and one assertion message) — ⛔ the test file contains **ZERO** `152.4` code literals, by construction (see §5) |

⇒ **zero second literals.** Verified again after the final edit pass.

---

## 2. THE PURE SEAM

`SummonedUnit.h:644` — `public`, plain C++ static, ⛔ **not** a `UFUNCTION`, **exactly four parameters, none defaulted** (`SC-§33`), no world access, no actor access:

```cpp
static float HeightAdvantageMultiplier(float AttackerZ, float TargetZ, float StepUU, float BonusPerStep);
```

`SummonedUnit.cpp:3213-3238` — the body, minus its comments:

```cpp
if (!(StepUU > 0.f))
{
    return 1.f;
}
const float HeightAdvantageUU = FMath::Max(0.f, AttackerZ - TargetZ);
return 1.f + BonusPerStep * (HeightAdvantageUU / StepUU);
```

- **`R-1` height above the TARGET, positive only** — `FMath::Max(0, AttackerZ − TargetZ)`. Level ground and shooting UPWARD both return **exactly 1.0**; ⛔ no low-ground malus branch exists.
- **`R-4` continuous** — ⛔ no `FloorToFloat`, no rungs.
- **`R-3` uncapped** — ⛔ no `FMath::Min`, no ceiling, no `HeightBonusMaxMultiplier`.
- **Zero-divide guard written as `!(StepUU > 0.f)` rather than `StepUU <= 0.f`** so a NaN step — which fails every comparison — also lands in the guard instead of propagating a NaN into a damage number.

---

## 3. COMPOSE POINT — ONE SITE, AND THE LINE NUMBER MOVED

⚠️ **The spec cites `SummonedUnit.cpp:3206`. That was `ComputeOutputDamage`'s line BEFORE this diff; the seam definition was inserted above it, so the function now starts at `:3240`** and the elevation factor is at **`:3313-3320`**, immediately after the Ancient-Grounds factor, as specified:

```cpp
if (bRangedAttack && Target != nullptr)
{
    Output *= HeightAdvantageMultiplier(
        GetActorLocation().Z,
        Target->GetActorLocation().Z,
        HeightBonusStepUU,
        HeightBonusPerStep);
}
```

- ⭐ **Exactly one call site in the whole codebase.** Pasted grep — `grep -rn "HeightAdvantageMultiplier" Source/` outside the test file returns **three** lines: the declaration (`.h:644`), the definition (`.cpp:3213`) and **this one call** (`.cpp:3315`). ⛔ No second compose point.
- **Gated on the already-shipped `bRangedAttack`** (`SummonedUnit.h:1644`, bound from `Row->bRanged` at `SummonedUnit.cpp:1197`) — ⛔ not a CardID list, ⛔ not a name check. A future ranged card inherits the behaviour from its own row with zero code.
- **Melee is bit-for-bit unchanged:** the branch is skipped entirely, so `ComputeOutputDamage` still returns `AttackDamage` for a non-keyword, un-auraed melee unit.
- **Null `Target` ⇒ ×1.0** by skipping the branch.
- **Both Z values are `GetActorLocation().Z`, on both sides, no exceptions.** ⚠️ **Declared consequence:** a large-footprint target (a castle) reports its **ORIGIN Z**, which sits at its base — so a unit on level ground beside a castle reads as *above* it and earns a small bonus. Same convention on both sides is what keeps the difference meaningful; a mixed convention (origin vs capsule vs bounds) is how a sign error hides.
- ⚠️ **Declared timing:** for a ranged unit the multiplier is locked at **fire time** (`PerformAttack` composes, then `FireProjectileAt` carries the value), exactly as Charge / Slayer / Aura / Ancient Grounds already are. A projectile in flight does not re-evaluate if either party moves.

---

## 4. THE FIREWALL (`WM-§8d` / `SHIP-§9`) — DECLARED, NOT ASSUMED

- ⛔ **No `#include "WarMapWidget.h"`, no call to `HeightToBrightness`, no read of `ElevationReliefCeiling` or the elevation texture** from any file in this diff. The rule reads `GetActorLocation().Z` from live actors and nothing else.
- ⛔ **Reverse direction clean too:** `grep -n "HeightAdvantageMultiplier\|HeightBonus" WarMapWidget.{h,cpp}` ⇒ **no matches**.
- ⛔ **No tower awareness of any kind** — no `bIsOnATower`, no occupancy lookup, no tower query, no `#include` of `Tower.h` or `ClimbableTower.h` (which does not exist yet and is TASK-726's).
- 📌 **FLAGGED FOR THE GATE, in the open:** three **PROSE** occurrences of the string `HeightToBrightness` exist in this diff — `SummonedUnit.h:604`, `SummonedUnit.cpp:3215`, `SiegeHighGroundTest.cpp:49` — each citing the `WM-§2` **precedent** that `HIGH-§3` itself names ("cloning the `WM-§2` / `HeightToBrightness` precedent"). They are comments. **Zero includes, zero calls, zero symbol references.** Raised here rather than left for QA to find.

---

## 5. TESTS — `Tests/SiegeHighGroundTest.cpp` (NEW), 9 automation tests

**⚠️ CHECKED FIRST, as instructed:** the 13 shipped test files are account / assistant×4 / castle-transform / cloud / controls-help / deck-slots / keyboard / settings / stuck-statics / war-map. **None covers `ASummonedUnit` combat**, so this is a NEW file, ⛔ not a duplicate of anything.

**⭐ SUITE BASELINE 156 — COUNTED, NOT TRUSTED** (`grep -h "IMPLEMENT_.*_AUTOMATION_TEST" Tests/*.cpp | wc -l` ⇒ 156, matching the spec). **NEW EXPECTED TOTAL FOR TASK-731's GATE: 165 (+9).**

**⭐ THE ANTI-VACUITY DISCIPLINE, APPLIED DELIBERATELY** (last night's lesson: a test whose two sides are equal by construction proves nothing):

- **⛔ `152.4` never appears as a code literal in the test file.** Every expectation is built from `constexpr float StepUU = FeetPerStep * CentimetresPerFoot;` — **re-derived from the international definition of the foot and from the number in his sentence**, ⛔ never transcribed out of the header it asserts. A transcription would agree with a wrong header for ever.
- **Two candidate assertions were WRITTEN AND THEN DELETED for being unfailable**, each replaced by a comment saying why: (i) a `100 < StepUU < 200` band on the test's *own* constant (moved to test 8, where it is aimed at the *shipped* value and can fail); (ii) "a skipped multiplier is 1.0, therefore melee is unchanged" in test 9 (equal by construction against every possible implementation).
- **The hill-vs-tower pairs are sited at DIFFERENT absolute Z on purpose.** Two identical calls would be equal by construction and would pass against the very absolute-Z reading the test exists to refute.

| # | Test | What it catches — i.e. how it FAILS |
|---|---|---|
| 1 | `LevelGroundAndUphillShotsAreExactlyOneWithNoMalus` | Fails against a **symmetric formula** that dropped `max(0,·)` and let ΔZ go negative (a low-ground malus). Also pins level ground to **exactly** 1.0 — a 1.0000001 would be a balance change to every existing engagement |
| 2 | `OneFiveFootStepInUnrealUnitsIsExactlyTenPercent` | One step ⇒ ×1.10, ten steps ⇒ ×2.00. **A 5-uu rise must read ×1.0033, not ×1.10** — fails against a formula that skipped the divide-by-step (×1.50 there), the arithmetic shape the feet-as-units mistake produces |
| 3 | `TheCurveIsContinuousAndNeverStepped` | **Fails against a floored/stepped implementation**: half a step ⇒ 1.05 (a staircase returns 1.00), 0.9 step ⇒ 1.09, 1.5 steps ⇒ 1.15, plus a 120-sample **strict**-increase sweep at 1/40-step spacing — a staircase plateaus and dies on the first pair inside a rung |
| 4 | `TheBonusIsLinearInHeightAndNeverCompounds` | bonus(2h) = 2×bonus(h), with a `bonus > 0` guard so two zeroes cannot satisfy it. **Fails against `pow(1.1, n)` compounding**: two steps must be ×1.20 (and measurably below 1.21), ten steps ×2.00 (not ×2.594) |
| 5 | `WorstCaseMultipliersMatchTheLawTableAndAreUncapped` | Cross-checks the code against **`HIGH-§5`'s table decimals** (1.6562 / 1.7874 / **2.4436** / 6.2493) — two independent derivations made to agree, so the figures Jonathan was quoted cannot silently stop matching the game. **Fails against ANY invented cap**: the curve must still be climbing at 100,000 uu |
| 6 | `ADegenerateStepIsTotalAndNeverDividesByZero` | Zero step ⇒ finite and exactly 1.0; **negative** step ⇒ 1.0 (unguarded it returns **−0.44**, a negative damage multiplier that reads downstream as healing your target); zero bonus ⇒ 1.0. **Plus a clause that stops the whole test being satisfied by `return 1.f;`** — a healthy step must still produce > ×2 |
| 7 | `OnlyHeightAdvantageMattersSoAHillAndATowerAgree` | ⭐ **The R-1 / hill-vs-tower test. Fails against absolute world Z.** (a) a 1,200-uu hill in a valley vs a 1,200-uu tower on a 900-uu shelf — absolute Z reads ×1.787 vs ×2.378; (b) a tower-on-a-hill vs a pure hill of equal advantage sited 500 uu lower — absolute Z reads ×2.44 vs ×2.12; (c) **translation invariance** over six world offsets incl. ±25,000; (d) two units on the SAME platform ⇒ **exactly 1.0** (self-limiting) |
| 8 | `TheShippedStepIsFiveFeetConvertedToUnrealUnits` | ⭐ **THE REGRESSION CLAIM.** Reads both tunables off the CDO **by reflection** and checks the step against `5 × 30.48` re-derived here. **Fails if a tidy-up rounds it to 150**, and separately names 150 / 152 / 5 / 500 so the failure log says *which* mistake was made. **A rename also fails it** (the lookup returns null ⇒ explicit `AddError`, ⛔ never a silent skip). (d) then feeds the **shipped** defaults through the **shipped** seam ⇒ ×1.10, so constant and formula cannot drift past each other |
| 9 | `TheHeroAndMeleeUnitsAreStructurallyExcluded` | **`R-5`/`R-6`, asserted structurally.** (a) `AHeroCharacter` is **not** a child of `ASummonedUnit` — **fails the day anyone re-parents the hero**, which would hand the player's own character an uncapped bonus; (b) the tunables exist on `ASummonedUnit` (**positive control**, so the next claim cannot pass vacuously) and **do not** exist on `AHeroCharacter`; (c) `bRangedAttack` defaults to **FALSE** by reflection — **fails if anyone flips that default**, which would hand the bonus to every unit whose row failed to bind |

**⛔ DECLARED NON-COVERAGE (`SC-§32`), stated rather than faked:**
- **`ComputeOutputDamage` itself is untestable headlessly** — it is `private`, non-`const`, consumes the primed charge, and needs two live actors. ⛔ **No accessor was added just so a test could reach it** (that is shipped surface bought to make a test possible). Its instruments are **TASK-730 gate question 3** (one compose point · gated on `bRangedAttack` · melee still bit-for-bit) and Jonathan's playtest. Test 9 covers the structural facts the exclusion actually rests on.
- **The ranged ROSTER** (which cards carry `bRanged=true`) is `cards.csv` — **TASK-723's**, and inert until `DT_Cards` is reimported (TASK-731's editor step). This file asserts the C++ default of the gate flag, ⛔ not the shipped roster.

---

## 6. THE WORST CASE, FOR JONATHAN (`HIGH-§5`, his row `R-3`)

| Height advantage ΔZ | Arithmetic | Multiplier |
|---|---|---|
| 152.4 uu (his 5 ft) | 1 + 0.10 × 1 | **×1.10** |
| ~1,000 uu — tallest shipped hill crown | 1 + 0.10 × 6.5617 | **×1.656** |
| ~1,200 uu — `TOWER-§`'s platform on flat ground | 1 + 0.10 × 7.8740 | **×1.787** |
| **~2,200 uu — the tower ON a tall hill, target in a valley** | 1 + 0.10 × 14.4357 | ⚠️ **×2.44 (+144%)** |
| ~8,000 uu — a castle shell (theoretical; no unit can stand there today) | 1 + 0.10 × 52.4934 | ⛔ **×6.25** |

⚠️ **THE ONE TO WATCH IS ×2.44.** A Longbowman there deals **18 × 2.44 = 43.9 per shot at 3,600 range** (post-TASK-723) against a target that, if it is another Longbowman on flat ground, cannot reach it at all. ⛔ **No cap was invented** — it is his to keep or cap, and the sanctioned shape if he wants one is ONE `EditDefaultsOnly` `HeightBonusMaxMultiplier` (0 = uncapped), ⛔ never a magic number in the formula.

---

## 7. DECLARED DEVIATIONS AND ADDITIONS (`SC-§15`)

1. ⭐ **NO PUBLIC GETTERS WERE ADDED for the two tunables, and the test reads them BY REFLECTION instead.** The obvious route was the shipped `ACommanderNpc::GetEnemyRevealCost` / `ASummonedUnit::GetPermanentDamageBonusPerStack` precedent (a `BlueprintPure` one-liner over a protected UPROPERTY). **Rejected on purpose:** the `names:` block does not list getters, they would be Blueprint surface added solely for a test's convenience, and reflection is strictly better here — it adds **zero** shipped surface **and pins the property NAMES `HIGH-§1` specifies**, since a rename makes the lookup return null and the test raises an explicit `AddError` rather than passing vacuously. The reflection idiom is already house practice (`Tests/SiegeAssistantSelectionTest.cpp:174-198`).
2. **The compose-point line number moved from `:3206` to `:3240`/`:3313`** because the seam definition was placed immediately above `ComputeOutputDamage`, keeping the arithmetic adjacent to its only consumer (the `HeightToBrightness` layout precedent). No behavioural change.
3. **Two prose comments in `ComputeOutputDamage` were CORRECTED, not just added to** — the function's contract sentence (`.cpp:3242-3251`) and its header doc (`.h:1540-1550`) said "returns `AttackDamage` bit-for-bit for a non-keyword, un-auraed unit". That sentence would have **drifted the moment this factor landed**, so the word **MELEE** was inserted and flagged as load-bearing. (The M7.7 lesson: the shipped `Notes` said "in 400" while `AoERadius` was 700.) ⛔ No behaviour changed with it.
4. **New `UPROPERTY` category `"Siegebound|HighGround"`** rather than folding the tunables into `"Siegebound|Keywords"` — elevation is not a keyword, and the keyword block is a coherent GDD §3.0 set.
5. ⛔ **No `EditAnywhere`** — both tunables are `EditDefaultsOnly` exactly as `HIGH-§1` pins (the keyword magnitudes beside them are `EditAnywhere`; the law overrides the local precedent).

---

## 8. STANDING DECLARATIONS

- **📌 M8:** ⛔ no replicated property, ⛔ no RPC, ⛔ no new class tier, ⛔ no `GetLifetimeReplicatedProps` change. The composition is **server-authoritative exactly as every existing multiplier in `ComputeOutputDamage` already is** — the elevation factor rides the same authority path as Charge / Slayer / Aura / Ancient Grounds.
- **Airlock:** ⛔ no `Capture()`, ⛔ no `EnsureSnapshot()`, ⛔ Zone A untouched and not read, 🔒 the 552 latch untouched and unspent, ⛔ **no token figure anywhere in this diff** (`AS-§12g`).
- **Fences honoured:** ⛔ no `cards.csv` (TASK-723) · ⛔ no `WarMapWidget.cpp` (TASK-722) · ⛔ no tower actor (TASK-726) · ⛔ no `SiegeControlsHelpWidget.cpp` (TASK-729 — it is dirty in the tree from that parallel task, ⛔ not from this one) · ⛔ no compile (TASK-731 owns the only one) · ⛔ no editor, ⛔ no MCP, ⛔ no Git · ⛔ `Tools/Packaging/` and TASK-716/700 untouched.

## 9. FILES

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` | seam declaration `:644` · tunables `:923`/`:936` · `ComputeOutputDamage` doc corrected |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | seam definition `:3213` · compose point `:3313` · contract comment corrected `:3242` |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHighGroundTest.cpp` | **NEW** — 9 automation tests (suite 156 → **165**) |

## 10. WHAT QA SHOULD SCRUTINISE HARDEST

1. **The number at `SummonedUnit.h:923` — read it character by character.** `152.4f`. Not 150, 152, 5 or 500.
2. **The three PROSE `HeightToBrightness` citations** listed in §4 — confirm they are comments citing `HIGH-§3`'s own wording and that there is no include, call or symbol reference.
3. **That `Output *= HeightAdvantageMultiplier(...)` appears exactly once** and sits *after* `GetPermanentDamageMultiplier()`.
4. **The `!(StepUU > 0.f)` guard** — it is intentionally not `StepUU <= 0.f`, so NaN lands inside it.
5. **Every new assertion's failure mode.** The three §5 anti-vacuity notes are the ones to check: the test file's `StepUU` is derived (`5 × 30.48`), never transcribed; the hill/tower pairs sit at different absolute Z; and the two deleted assertions are documented in place rather than quietly dropped.
