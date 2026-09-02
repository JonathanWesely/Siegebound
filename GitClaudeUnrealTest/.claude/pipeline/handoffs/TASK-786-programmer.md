# TASK-786 — `K-6` = **350 uu**, the pin reconciliation, and the stale derivation prose

**Agent:** gameplay-programmer · **Date:** 2026-09-02 · **Status:** ready-for-qa
**Suite delta: ⛔ ZERO tests added.** Still **12** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros; row (g) still holds **4** assertions. Pins and fixtures were **moved**, ⛔ never added ⇒ adds nothing to TASK-780's one suite total.

> ⚡ **SCOPE HISTORY:** item 1 was withdrawn mid-task (TASK-784 had landed 150 → 300), then **restored with a new number** when TASK-778 measured that 300 fails the fastest hero. I am now the only agent in `ClimbableTower.h`. **Final value shipped: `LadderContactRadiusUU = 350.f`.**

---

## 1. ⭐⭐ WHY 350 AND NOT 300 — THE TWO CLASSES NEED DIFFERENT ARITHMETIC

**This is the whole finding, and it is why 300 looked sufficient and was not.**

| | poll | dwell model | requirement | value |
|---|---|---|---|---|
| **Units** | `StateCheckInterval` 0.25 s | each poll credits a **whole 0.25 s** for one sampled instant ⇒ needs **two samples** ⇒ ⚠️ **probabilistic** | `R/v ≥ 2 × 0.25` | `2 × 0.25 × 600` = **300** |
| **Hero** | **per frame**, in `Tick` | dwell accrues `DeltaSeconds` ⇒ ⭐ **exact** | `R/v ≥ 0.35` | `0.35 × 937.5` = **328.125** |

⇒ ⭐⭐ **THE HERO BINDS, NOT THE ROSTER.** `350 / 328.125 = 1.0667` ⇒ **+6.67% margin.** Both figures re-derived here; both agree with the dispatch.

### The hero's four speeds — `WalkSpeed` 500 / `SprintSpeed` 750 (`HeroCharacter.h:999`/`:1003`), each ×1.25 with Swift Boots

| speed | | `R/v` @ 300 | `R/v` @ 350 | margin @ 350 |
|---|---|---|---|---|
| 500 | walk | 0.600 ✅ | 0.700 ✅ | ×2.00 |
| 625 | walk + Boots | 0.480 ✅ | 0.560 ✅ | ×1.60 |
| 750 | sprint | 0.400 ✅ | 0.467 ✅ | ×1.33 |
| **937.5** | **sprint + Boots** | **0.320 ⛔** | **0.373 ✅** | **×1.07** |

⚠️ **A player sprinting with Swift Boots at a ladder is the most likely first test of this feature**, and at 300 it fails **at every offset and every frame rate** — not marginally, structurally.

⭐ **Frame-rate robustness of that last row (my own addition — the margin is thin enough to deserve it):** the guaranteed sample count is `floor(W/f)` against a need of `ceil(0.35/f)`. It holds at **60 fps (22 vs 21), 30 fps (11 vs 11), 20 fps (7 vs 7)** and ⛔ **fails at 15 fps (5 vs 6)**. So 350 is sound across any playable frame rate, with 30 and 20 fps sitting exactly on the boundary.

### Units are unaffected by the 300 → 350 step
Phase counts, `clamp((R/v − P)/P, 0, 1) × 16`:

| card | v | @150 | @300 | @350 |
|---|---|---|---|---|
| Ogre | 250 | 16/16 | 16/16 | 16/16 |
| Knight / Longbowman | 300 | 16/16 | 16/16 | 16/16 |
| Archer tier | 350 | **11/16** | 16/16 | 16/16 |
| Footman / Militia | 400 | **8/16** | 16/16 | 16/16 |
| Sapper | 500 | **3/16** | 16/16 | 16/16 |
| Cavalry | 600 | **0/16** | 16/16 | 16/16 |

⭐ My formula reproduces **all six** of TASK-784's independently simulated counts (11, 8, 3, 0) exactly — two different methods agreeing.
⛔ **"No unit could ever climb at 150" is refuted** (Ogre and Knight were 16/16); the real defect was **intermittency** — *"sometimes it works"* is worse than dead because nobody can report it. ⚠️ **The units were already fixed at 300; the 300 → 350 step buys them nothing and is paid entirely for the hero.**

---

## 2. ⚠️ THE ACCIDENTAL-GRAB COST AT 350 — **MEASURED, NOT SCALED**

Breakeven passer-by offset (window `(√(R²−d²) − d/√3)/v` = 0.35), at **v = 300**:

| R | half-width | step |
|---|---|---|
| 150 | ±57.8 | — |
| 300 | ±202.1 | **3.5×** for a 2× radius |
| **350** | **±247.2** | **+22.3%** for a **+16.7%** radius rise |

⛔ **A linear scale from ±204 would have guessed ±238 and understated it by ~9 uu.** The growth is super-linear at *every* step because the dwell only ever eats a **fixed** `v × 0.35 = 105 uu` of approach, which is a shrinking fraction of a growing disc.

⭐ **Model validated before publication:** my closed form reproduces the shipped fixture's own QA-passed figures **exactly** — `d=0 → 0.5000 s`, `d=100 → 0.1802 s` at R=150/v=300, against its comment's "0.500"/"0.180". It also agrees with 784's *driven* measurement (±62 → ±204 at v=250 on a 1/40 grid).

**🧑 The figure for Jonathan: a pawn marching past its own tower is now grabbed from up to ~247 uu to the side (was ~58 at `K-5`).**

⚠️⚠️ **CORRECTION TO MY OWN EARLIER NOTE — the "free dwell lever" is now VOID.** Before the hero model was in scope I reported that the dwell could rise 0.35 → 0.50 s at zero radius cost. **That is false at 350:** a 0.50 s dwell needs `R ≥ 0.50 × 937.5 = 468.75` for sprint+Boots and `≥ 375` even for plain sprint. **The real headroom is 0.35 → `350/937.5` = 0.373 s.** The free band was a unit-only artifact and I am retracting it.

---

## 3. ⭐⭐ THE TANGENT-DISC RE-CHECK — THE PREMISE GETS STRONGER AT EVERY STEP

XY separation between endpoints is **fixed at 300 uu**; Z separation 1,200.

| R | discs | consequence |
|---|---|---|
| 150 | exactly **TANGENT** | a ground pawn merely **grazed** the foot's rim |
| 300 | **overlap**, 300-uu lens | each endpoint's centre lands **exactly on** the other's rim |
| **350** | **overlap**, 400-uu lens | each centre is **50 uu inside** the other's disc |

⇒ the Z resolution is no longer merely tidy — it is **the only thing** keeping a pawn at the foot from being range-eligible for the deck, and vice versa.

✅ **IT SURVIVES BY CONSTRUCTION.** `IsAtTopEndpoint` (`SiegeLadderClimbStatics.cpp:213`) compares `|P.z − Top.z|` against `|P.z − Foot.z|` and takes **⛔ no radius term at all**. **No radius can perturb it.** Read, not assumed.

**⛔ WHAT WOULD ACTUALLY MAKE IT FAIL** — the answer you asked for: it is a **midplane test at `PlatformHeightUU`/2 = 600 uu**. A ground pawn stands at capsule half-height **88** ⇒ **512 uu of margin**. It fails only if a pawn can stand **above 600 uu at the foot**: a deck lowered under ~176 uu, or a second structure tall enough to stand on beside the ladder. ⚠️ **A radius change can never cause it. A `PlatformHeightUU` change can.** Now written into the row.

### ⭐ The row that proves it was re-sited, not relaxed — and 350 improves it
Under the deck socket the pawn is exactly **300 uu** from the foot: out of range at `K-5`, **exactly on the rim** at 300 (the inclusive `<=` on `300² == 300²`), and at **350 a clean 50 uu inside**. ⭐ **The boundary case is gone rather than merely tolerated.**

⛔ **I did not relax the expectation to `NotHeadingIn`** — that would have proved **nothing**, because the *broken* XY resolution yields `NotHeadingIn` too (degenerate bearing from directly underneath). ⭐ Velocity turned to `Outward` so the hypotheses diverge in **both** outputs:

| resolution | verdict | `bAscending` |
|---|---|---|
| by **Z** (correct) | `Climb` | **true** |
| by **XY** (the defect) | `NotHeadingIn` | false |

The ascent-flag half is the pure Z proof — written on **every** path including refusals, radius-independent.

---

## 4. ⭐ TEST 16 RECONCILED BY ⛔ NOT TOUCHING IT — AND IT NEEDS NO CHANGE

You flagged that test 16's `2 × Poll × 600` derivation is unit-shaped. **It is — and I verified it does not break.**

✅ **Tests 16(a)/(b)/(c) never read the shipped radius for any assertion.** 16(a) computes `RequiredRadiusUU` from the **poll and roster** and asserts it equals 300; 16(b) drives the predicate at that derived 300 and at a derived 150; 16(c) compares two derived radii. The shipped value is read only in a `RadiusUU > 0` self-check. ⇒ **350 changes none of them.**

⭐ **So there is nothing to weaken, and I weakened nothing.** The correct reading: **16 answers a different question** — *"what radius does the roster need at the shipped poll?"* — and its answer (300) is still true. It simply **cannot see** the hero's per-frame requirement, which is **higher**. I recorded that distinction in the pin comment so the next reader does not mistake 16's 300 for the whole constraint.

✅ **Test 15(c) does read the shipped radius, and it passes with more room than at 300:** `ReliableCeiling = R/2P = 700`, so all six cards sit in the RELIABLE branch and all are 16/16. ⭐ Cavalry's window goes 0.500 → **0.583 s**, so it is no longer the exact-boundary case it was at 300. 15(a)'s edge self-check also holds (0.95 × 350 = 332.5 ⇒ negative window ⇒ 0 phases).

---

## 5. THE FIXTURE ROWS THE RADIUS MOVED UNDER (items 2, and the real work)

`PinnedContactRadiusUU` is **not only the pin** — it is also the radius fed to every behavioural scenario. Moving it silently changed the geometry under six rows. **Measured across all three radii:**

| probe | R=150 | R=300 | R=350 | action |
|---|---|---|---|---|
| passer-by `d=100` | refused | ⛔ **ADMITTED** | ⛔ **ADMITTED** | offset → **300** |
| `FarFromFoot(−300)` | TooFar | ⛔ **Climb** | ⛔ **Climb** | → **−600** |
| under-deck | TooFar | Climb | Climb | re-sited (§3) |

⛔ **Not fixed by splitting the constant.** Two radius constants is two numbers that drift — the defect `ClimbableTower.h`'s own banner names — and the abduction row would then prove safety at a radius the game **does not ship**. ✅ Fixed by moving the **fixture** and keeping one number.

⚠️ **The passer-by offset went 100 → 300, not 250.** At R=350 an offset of 250 refuses by only **0.015 s** — a coin flip dressed as an assertion. **300 refuses by 0.326 s** and the pawn is still inside the disc for **1.202 s, nearly four dwells** ⇒ ⭐ the row now isolates the **cone** as the sole refuser. At the old `d=100` the proximity term was carrying part of the claim.

**The pin, its label and the citation moved together:** 150 → **350**, label now cites `K-6` **and states the hero derivation**, ⛔ still **typed from the law** and never read back off the CDO. TASK-784's "expect exactly one red row" is discharged.

---

## 6. ⛔ THE SOCKET-FALLBACK WARNING — UNTOUCHED (item 4)

⛔ **Zero edits to `ClimbableTower.cpp`.** `:391` is still `UE_LOG(..., Warning, ...)`, verbatim, degrade-**open**. Verified after all edits.

⭐ **785's ruling recorded so it is not re-litigated:** the fallback **stays degrade-open**. A fallback that refuses to start a climb creates exactly the unreachable deck `TOWER-§8.4(A)` forbids, and it would aim at the **wrong pawn** — the licence at risk was the **hero's** (51.62 → 61.32); the **unit's 69.32 was never in question**. And **`TASK-779` item (7) makes the ABSENCE of that exact Warning string TASK-780's PIE confirmation** ⇒ moving its severity moves a string a live gate reads. **Boarded, ⛔ not slipped in.**

---

## 7. THE STALE DERIVATION PROSE — REPAIRED (item 3)

`ClimbableTower.h` (now `:454-459`): **`−450`/`−150` → `−460`/`−160`**, **`86`/`86` → `96` (foot) / `76` (top)**. Both re-derive from the shipped literals: `|−460 − (−364)| = 96`, `|−160 − (−236)| = 76`. ✅ TASK-783's measurements check out.

Added the `SC-§36` framing: the translation is **pure**, so `Δ = (300, 0, 1200)`, length 1,236.9 and lean 76.0° are invariant — **only these two clearances moved, in opposite directions** (foot 86→96, top 86→76). `ClimbableTower.cpp:67-68/90-91` already carried the corrected pair; **this header was the last stale copy.**

---

## ⚠️ FOR QA / THE MANAGER — OUTSIDE MY FENCE

1. ⛔⛔ **PROSE DEBT, NOW TWO RADII STALE:** `SiegeLadderClimbStatics.cpp:210` **and** `.h:472` still say the discs are **TANGENT at a 150 uu radius**. False at 300, more false at 350 (400-uu overlap lens). **Needs boarding** — same silent class as the `86`/`86` debt this task just repaired.
2. ⚠️ **RESIDUAL — buffed Cavalry improves but does not close.** Battle Cry (+25%) puts Cavalry at 750 uu/s ⇒ **10/16 phases at 300, 14/16 at 350** — better, still not reliable. Full reliability needs `2 × 0.25 × 750` = **375 uu**. ⛔ Declared, not slipped in; 375 would push the grab window past ±270.
3. ⭐ **The hero's dwell headroom is now only 0.35 → 0.373 s** (see §2 retraction). Any future dwell rise must move the radius with it.
4. ℹ️ Noted and **not touched** per your instruction: the tower still refuses the hero on identity (`EvaluateLadderEntry` → `NotAnAdmittedClimber`). My radius work is independent of it.

---

## FILES TOUCHED

- `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.h` — `LadderContactRadiusUU` **300 → 350** + its re-derived comment (item 1) · the socket-clearance prose (item 3).
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp` — the pin + the six fixture rows.
- `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp` — ⛔ **zero edits.**

## VERIFICATION

⛔ No compile (one compile, TASK-780), ⛔ no editor, ⛔ no MCP, ⛔ no Git.

⭐ I ported `WantsToClimb`/`Disarm` faithfully — the inclusive `<=` proximity test, `GetSafeNormal2D`'s zero-vector degeneracy, `MinContactSpeedUU = 1`, latch-before-dwell ordering, endpoint-swap dwell reset — and ran **every row of tests 11 and 12 at R=350: 23/23 green.** The same port, swept across 150/300/350, is what produced the flip table in §5, so each fixture move is backed by a measured flip rather than by reasoning alone.

## PRESERVED (all QA-passed, verified untouched)

Ninth-exit guard `bLadderOccupied = ActiveClimber.IsValid()` with **no identity comparison** · `EvaluateLadderEntry` · sockets via `GetSocketTransform(..., RTS_Actor)` · `CanTeamAscend` · the fail-open pathfinding layer · the cone and dwell terms — ⭐ **the bearing still swings** (row (f) moves the pawn; at d=300 the swing is the sole refuser) · both endpoints resolved **by Z** (re-checked at 350, §3) · the re-entry latch · ⛔ **the no-polling-member guard** — the tower gained **no** tick, timer, poll or interval member.
