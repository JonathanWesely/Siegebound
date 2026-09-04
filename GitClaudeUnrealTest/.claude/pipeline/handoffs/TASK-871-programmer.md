# TASK-871 — the four-corner footprint slope trace — ⚙️ gameplay-programmer handoff

**Status:** `ready-for-qa` → gate **`TASK-914`** (⛔ its own gate; `qa/TASK-872.md` covers `TASK-870` **alone** and must not be cited for this — `SC-§29`).
**Delivered:** 2026-09-03. Tree base: `1aa0fee`. ⛔ No compile, ⛔ no editor, ⛔ no MCP, ⛔ no Git.

---

## (0) ⛔ THE RELAYED MEASUREMENT, RE-MEASURED MYSELF — AND IT MATCHES, WITH TWO CORRECTIONS

`SC-§40` cl. 3 + cl. 9. Symbol located **BY SYMBOL** (`SC-§38`), not by the row's line numbers.

| claim (relayed from `qa/TASK-816.md` W-4 / `R-3`) | my own measurement at `1aa0fee` | verdict |
|---|---|---|
| the slope gate is a **SINGLE straight-down trace at the cursor** | `bool ASiegePlayerController::IsGroundSlopePlaceable(const FVector& Point) const` — **one** `LineTraceSingleByChannel` at `Point ± 500 Z`, `ECC_Visibility`, adjudicated `SlopeDegrees <= MaxPlacementSlopeDegrees` | ✅ **CONFIRMED** |
| it takes **no footprint** | the signature had **no radius parameter at all**, and its **one** caller passed `PlacementLocation` alone | ✅ **CONFIRMED** |
| `FootprintRadius` "already exists one line above the gate" | it is computed **9 lines above** the gate and already feeds the two clearance gates | ✅ **CONFIRMED in substance** — "one line" is a dated hint, not a defect |
| the ghost is **player-adjustable up to ×1.5** | `PlacementFootprintMax = 1.5f`, `PlacementFootprintMin = 1.0f`, both reflected `EditDefaultsOnly` | ✅ **CONFIRMED** |
| the ghost's footprint is the **SCALED** bounds | `GhostMesh->CalcBounds(GhostMesh->GetComponentTransform())` | ✅ **CONFIRMED** (`STACK-§6`) |

### ⚠️ CORRECTION 1 — the threshold is **20°**, and I did **not** derive it from `AgentMaxSlope`
`MaxPlacementSlopeDegrees = 20.f` (`SiegePlayerController.h`, `EditDefaultsOnly`, `ClampMin 0 / ClampMax 90`). ⛔ Not 32.005°, ⛔ not 44°. The dispatch's warning was well placed and is honoured twice: the shipped code reads the tunable, and **the tests read it off the CDO by reflection** rather than transcribing it.

### ⚠️ CORRECTION 2 — the arena's steep ground is **~27.5°**, not the 40° the report illustrates with
Shipped hill law (`CONVENTIONS`, M6.6 "Climbable hill meshes"): every hill face ≤ **30°**, authored at **~27–27.5°**, crowns ≤ **8°**. ⇒ the overhang lands on a **shallower** flank than W-4's example. ⛔ **This does not weaken the finding** — 27.5° still exceeds the 20° limit by a wide margin — but the "40°" is illustrative, not a measurement of this arena.

### ⭐ AND ONE THING THE ROW DID NOT SAY — I FOUND IT WHILE WRITING THE TEST
⛔⛔ **The shipped arithmetic fails *OPEN* on a NaN surface normal.** `FMath::Acos(double)` is
`acos((V<-1.0) ? -1.0 : ((V<1.0) ? V : 1.0))` — **both comparisons are false for NaN**, so a NaN
maps to `1.0` and `acos(1.0) = 0` ⇒ **a NaN normal read as *perfectly flat* and was ADMITTED.**
Measured directly in `GenericPlatformMath.h:534`, ⛔ not inferred. **Fixed** (now reads 180° and
refuses). Practically unreachable from Chaos, but it is a real property change beyond "tighten the
gate" and is therefore **declared, not smuggled** — see §"Behaviour changes" row 3.

---

## (2a) ⛔ THE BALANCE-VISIBLE HALF — **what now fails that did not**, measured

**The short answer: NOT "a lot". It is a bounded, nameable set — hill crowns, and nothing else.**

**Instruments** (⛔ named, and their limits stated): `MaxPlacementSlopeDegrees` read from the header; hill geometry from `CONVENTIONS` M6.6's mesh table; the arena floor from M6.5 (*"the EXISTING arena floor slab is SCALED"* — a **flat slab**). ⛔ **No PIE, no editor** — this is geometry, not observation.

| ground class | today | after | change |
|---|---|---|---|
| **the flat arena slab** (the overwhelming majority of the 7380-half-extent spawn box + capture zones) | all samples 0° ⇒ pass | all samples 0° ⇒ pass | ⭐ **ZERO. Bit-identical.** |
| **hill FLANKS** (~27–27.5°) | centre reads 27.5° > 20° ⇒ **already refused** | still refused, same message | ⭐ **ZERO** |
| **hill CROWNS** (≤8°) | centre reads ≤8° ⇒ **passes, however far the building overhangs** | passes only if the footprint square fits on the crown | ⚠️ **THIS IS THE WHOLE CHANGE** |

**The crown budget, derived:** a placement is refused when `R·√2 > CrownRadius`, i.e. **`R > CrownRadius / √2`**.

| shipped hill | crown | threshold at ×1.0 | threshold at ×1.5 (wheel max) |
|---|---|---|---|
| `SM_Hill_01` knoll | r **220** | `R > 155.6` | `R > 103.7` |
| `SM_Hill_02` hill | r **320** | `R > 226.3` | `R > 150.9` |
| `SM_Hill_03` ridge | **1400×350** (half-extents 700 × 175) | `R > 123.7` (short axis) | `R > 82.5` |

⛔ **WHAT I COULD NOT MEASURE, SAID PLAINLY:** the actual shipped `R` per building card. It comes from the ghost mesh's **scaled bounds at runtime** and needs the editor. Two **DATED HINTS** from `CONVENTIONS`, flagged as relayed (`SC-§40`) and ⛔ **not** treated as facts: `TOWER-§7` records the WatchTower footprint as **~750 uu** (⇒ R ≈ 375) and the old 2,700 as *"roughly 10× a wall's"* (⇒ a wall ≈ 270 uu, R ≈ 135).

**On those hints:** a wall-sized structure (R ≈ 135, corner at 191) still fits `SM_Hill_01`'s crown at ×1.0 but **not at ×1.5** (corner 286 > 220). A WatchTower-sized one (R ≈ 375, corner at 530) **no longer fits any shipped hill crown**.

### 🧑 THE ONE GENUINELY NEW PLAYER-VISIBLE BEHAVIOUR — for Jonathan's eye, not mine to tune
⭐ **On a hill crown, wheeling a building UP can now turn a green ghost RED.** That is *correct* — the building really is overhanging — but it is **new feel**, and it is exactly the ×1.5 interaction W-4 named. ⚖️ **I have tuned nothing.** If the crowns feel too tight in play, the levers are his: `MaxPlacementSlopeDegrees`, or the flagged corner-distance decision below.

---

## The change

### Fence honoured
⛔ **The slope gate ONLY.** ⛔ Gate chain **not reordered** (`Units` still last, `first-failing-rule-wins` intact). ⛔ `EffectiveBuildingClearance`'s `max`-not-sum **untouched**. ⛔ **No second clamp** on `PlacementFootprintScale` (`R-4c`). ⛔ No art, no WBP, no `DT_Cards`. ⛔ No enum touched.

### Files
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp` (**existing tracked file extended — ⛔ no new file, so `TL-§5d`'s orphan trap does not apply**)

### Four new pinned pure statics (`public`, plain C++, ⛔ not `UFUNCTION`s, ⛔ no defaulted parameter — `SC-§33`)
```
static int32   NumPlacementSlopeSamples(float FootprintRadius);
static FVector PlacementSlopeSampleOffset(int32 SampleIndex, float FootprintRadius);
static float   PlacementSurfaceSlopeDegrees(const FVector& ImpactNormal);
static bool    IsSurfaceNormalWithinSlopeLimit(const FVector& ImpactNormal, float MaxSlopeDegrees);
```
They sit beside `TASK-735`'s four, `TASK-813`'s three and `TASK-815`'s three, for the same reason: `IsGroundSlopePlaceable` needs a `UWorld` and a live trace and can **never** be reached headlessly, but the **geometry it traces is the whole feature**.

### The gate
`IsGroundSlopePlaceable(const FVector& Point, float FootprintRadius) const` — loops `NumPlacementSlopeSamples`, offsets by `PlacementSlopeSampleOffset`, adjudicates via `IsSurfaceNormalWithinSlopeLimit`. **One** `LineTraceSingleByChannel` call serves all samples; the query params and the ±Z bracket are built **once** and shared.

### ⛔ THE FOOTPRINT IS NOT RE-DERIVED
The gate **consumes** the frame's existing `FootprintRadius` — the same value the two clearance gates are fed, read once from the ghost's **SCALED** bounds. ⛔ Zero `CalcBounds` / `TryGetPlacementFootprintRadius` / `GetStaticMeshComponent` inside the gate (**asserted**, test 31(b), each with a positive control).

### (4) ⛔ THE DEGRADE IS **KEPT**, AND I AM SAYING SO AS REQUIRED
An unknown footprint (missing ghost / missing mesh / degenerate bounds) yields `0`, `NumPlacementSlopeSamples(0) = 1`, and the probe **is the shipped single straight-down trace at the cursor, byte-for-byte**. ⛔ No hard refusal on a missing measurement — that would make an art-pipeline hiccup unplayable. The call site passes the **bare `FootprintRadius`**, exactly as its `HasBuildingClearance` sibling does, rather than re-deciding the degrade with a second expression.

---

## Behaviour changes — all three, declared

1. ⭐ **THE FIX.** A footprint corner on ground steeper than the limit now refuses. This is the point. Balance impact measured above.
2. ⚠️ **A REFUSAL MAY CHANGE ITS MESSAGE.** Tightening gate 4 means a placement that today refuses at `Obstacle`/`Clearance` may now refuse at `Slope` ("Too steep") if a corner sample lands on something steep. ⛔ **Nothing is reordered** — this is inherent to tightening a gate that already sits second, and the task asked for it. Flagged because `TASK-735`'s non-regression argument was partly about message stability.
3. ⚠️ **NaN NORMAL: fail-OPEN → fail-CLOSED.** See §(0). In the gate's declared direction; unreachable in practice.

### ⛔ AND ONE PLACE THE GATE DELIBERATELY DOES **NOT** BITE — the flagged degrade
**A corner trace that MISSES is SKIPPED, ⛔ never refused.** The **CENTRE** keeps its shipped fail-closed semantics **exactly** (miss ⇒ refuse, same Verbose line, same wording). A corner over nothing has no surface, therefore no slope; refusing on it would invent a new refusal class out of an **absent** measurement (house null-safety law) and would silently make every arena edge and every gap unbuildable **with nothing in any log to say why**.
⇒ ⭐ **The gate can only become stricter WHERE IT MEASURED SOMETHING, and ⛔ nothing that refuses today can start passing.**

### ⚖️ THE FLAGGED DECISION QA SHOULD PRESS ON — corners at `(±R, ±R)`, ⛔ not at distance `R`
`PlacementFootprintRadiusFromBounds` returns a **half-extent** (`max(|X|,|Y|)`), ⛔ not a circumradius. The thing it half-describes is a **box**, and a box of half-extent `R` has corners at `(±R, ±R)`. Deriving the samples from *what the value is* is the reading that cannot drift away from it.
⚠️ **The price, declared:** for a **round** or strongly **oblong** footprint the short-axis corners are sampled **beyond the mesh** — over-refusing by up to **~41%** of the radius. That is the exact mirror of the residual `PlacementFootprintRadiusFromBounds` already declares in the *other* direction (its circle **under**-covers the rectangle's corners).
⚖️ **Why it is accepted HERE and refused THERE:** the unit gate over-refusing costs playability against a **dense, mobile** hazard in a busy spawn box; steep terrain is **sparse and static**. And this is the one gate in the family that **already fails closed**, so over-refusing is its declared direction and under-refusing is the defect it exists to prevent.
🧑 **One word flips it, and it is a one-line change** in `PlacementSlopeSampleOffset`.

---

## Tests — 3 new, in the existing placement frame

**The anti-fake pairing, named first, because a *tightened* gate is the one change a test can be green about for the worst possible reason** (*"it refused"* is also what a gate that refuses **everything** reports):

- ⭐ **THE NEGATIVE CONTROL** (`TASK-914` (c), verbatim): test 30(b) — a **genuinely flat pad at the shipped `PlacementFootprintMax`**, with **all** samples really taken, **is still ADMITTED**. Paired with 30(b2) proving the probe genuinely reached **further** at ×max than at ×min, so the control is ⛔ not vacuous.
- ⭐ **THE POSITIVE CLAIM**: test 30(c1)/(c2) — the **centre alone admits** a crown-with-overhang configuration (*this is what passes today, and it is the defect*), while the **full probe refuses it**. 30(c3) proves **every** corner does it, not one privileged index.
⇒ a refuse-everything implementation fails (b); a refuse-nothing one fails (c); ⛔ neither can pass.

**Test 29 — the offsets are DERIVED, ⛔ never transcribed** (`SC-§37`; `TASK-914` (c)).
⭐ **The row a transcribed corner CANNOT satisfy:** `Offset(i, 2R) == 2 · Offset(i, R)` **exactly**, and `Offset(i, R) == R · Offset(i, 1)`, across three radii including one that is nobody's shipped number (`137.5`). A hardcoded `FVector(200,200,0)` passes every other row in the file and fails this one. Plus: sample 0 is exactly the zero vector; each corner is `|X| = |Y| = R` with `Z == 0`; the four corners are **distinct** and cover **all four quadrants** (asserted as a **property scan**, not a table); the degrade takes exactly one sample for `0`/negative/NaN/+inf; out-of-range indices answer the centre.

**Test 30 — the adjudicator.** Both angles **derived from the CDO's own limit** (`limit × 0.5` crown, `limit × 2` flank, clamped below 90). Boundary is `<=` — asserted **without a trig round-trip** (a flat surface against a limit of exactly `0` is admitted under `<=`, refused under `<`), plus 30(d2) proving the predicate is *exactly* `degrees <= limit` and nothing else across seven angles. Round-trip check catches a radians/degrees or sign slip. Fail-closed rows for NaN normal, zero normal, sideways, straight-down, **and a NaN limit** — each paired with a flat-normal control.

**Test 31 — the shipped body really has that shape** (source probe; the two halves are only worth anything together — `SC-§32`).
- each seam consumed **exactly once**; **one** trace call serves all samples;
- ⛔ **nothing re-derived** — `CalcBounds` / `TryGetPlacementFootprintRadius` / `GetStaticMeshComponent` = **0** inside the gate, each with a positive control;
- ⛔ **no slope arithmetic left in the gate** — `FMath::Acos(` = 0 inside it, **1** file-wide;
- ⭐ **THE GATE CHAIN IS UNREORDERED** — `CheckPrecedes` on `Slope` → `Obstacle` → `Clearance` → `Units`;
- ⭐ **the two orderings**: `SetActorScale3D(` < `TryGetPlacementFootprintRadius(` (`TASK-815`'s, re-asserted) **and the NEW one this task depends on** — `TryGetPlacementFootprintRadius(` < `IsGroundSlopePlaceable(`. ⛔ Written the other way the trace would validate **this** frame's click against **last** frame's size: an intermittent refusal with a green suite;
- ⛔ the fence **re-measured after this diff**: `max`-not-sum = 1, sum = 0, `StepPlacementFootprintScale(` = 2, `MakePlacementFootprintScale3D(` = 3.

### ⭐⭐ `SC-§41` — THE NEEDLE TRAP, HIT AND DISARMED, WITH THE CONTROL EXECUTED
`IsGroundSlopePlaceable(PlacementLocation` is a **SUBSTRING** of the new call `IsGroundSlopePlaceable(PlacementLocation, FootprintRadius)`. A needle **without the closing paren** would count the new call as if it were the old one and test 31(d) would be **a lie that reads green**.
⭐ **The `)` is the discriminator. I greped it before shipping it, with a same-role positive control, and the pair is asserted in the test itself:**

| needle | count on code lines | meaning |
|---|---|---|
| `IsGroundSlopePlaceable(PlacementLocation)` | **0** | the old point-only shape is genuinely gone |
| `IsGroundSlopePlaceable(PlacementLocation` | **1** | ⭐ **the probe is not blind** — the zero above is separation |

`SC-§39`'s other two blind spots also handled: every "zero occurrences" row is paired with a positive control, and no new code line carries a trailing `//` that could manufacture a false hit.

---

## Census — `TL-§5c` / `TL-§5b`

**Scoped pattern:** `^IMPLEMENT_[A-Z_]*AUTOMATION_TEST\(` over tracked `Source/**/Tests/*.cpp`.

- **HEAD (`1aa0fee`): `425` declared across `31` files.**
- **Working tree now: `431` declared across `31` files.** ⛔ **Declared. ⛔ NOT a pass count.**
- **My delta: `+3`, all in `SiegePlacementTest.cpp` (28 → 31).**

⚠️ **RECONCILIATION — the dispatch's relayed `427 / 31` does not match either number, and here is why, so nobody goes hunting.** The `+6` tree delta is **three** files:

| file | HEAD | tree | delta | whose |
|---|---|---|---|---|
| `SiegeAssistantSelectionTest.cpp` | 38 | 39 | **+1** | ⛔ **not mine** (another lane, in flight) |
| `SiegeControlsHelpTest.cpp` | 16 | 18 | **+2** | ⛔ **not mine** — `TASK-870`'s, already PASSED by `qa/TASK-872.md` |
| `SiegePlacementTest.cpp` | 28 | 31 | **+3** | ✅ **mine** |

⇒ `qa/TASK-872.md`'s **`427` was CORRECT WHEN WRITTEN** (`425 + 2`, before the Assistant lane's `+1` landed) and has since been overtaken. ⭐ **This is `TL-§5b`'s "stale before the ink dries" exactly, ⛔ not a missing test. Nobody should go looking for one.**

⚠️ The bare `^IMPLEMENT_` trap reads **`432 / 32`** on this tree (`TL-§5b` cl. 2a — an absence is a property of a tree, never a repeal).

⛔ **`TL-§5d`: no new test file.** I extended a tracked one, so nothing here is invisible to `HEAD`.

---

## ⛔ WHAT I DID NOT RUN (`TL-§5c` cl. 5)

⛔ **I did not compile.** ⛔ **I did not run the suite** — the `431` above is a **declared** census, ⛔ never a pass count. ⛔ No editor, ⛔ no MCP (editor is up under an art task), ⛔ no Git beyond read-only `status`/`log`/`show` used to reconcile the census. ⛔ No PIE, so the balance table above is **geometry, not observation**.

---

## 🔍 For `TASK-914` — where to press hardest

1. ⚖️ **The corner-distance ruling** — `(±R, ±R)` vs distance `R`. Both readings are defensible; my argument is above and it is one line to flip. **Attack it.**
2. ⚠️ **The message-shuffle** (behaviour change 2). I judged it inherent to tightening gate 4 and in fence; disagree if you read the fence differently.
3. ⚠️ **The skipped-corner degrade.** It is the one place the new gate declines to bite. I believe it is the house law; check that it cannot re-open the defect (my argument: a flank adjacent to a crown is always within the ±500 bracket, so a steep corner never *misses*).
4. ⚠️ **The brace-matching extractor** in test 31 is new instrument surface. It is self-checked four ways (non-empty, the one trace call, `ImpactNormal` present, and that it did **not** swallow `HasObstacleClearance`) — but it is new, and `SC-§39.1` binds tool **authorship**.
5. ⚠️ **The NaN fail-open finding** (§0). If you think that is out of fence, say so — I judged it in-fence (it *is* the slope gate) and in the gate's declared direction.
