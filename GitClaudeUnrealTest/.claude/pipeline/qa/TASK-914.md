# QA Report — TASK-914
Verdict: **PASS**

**Blockers: 0 · WARN: 6 · NIT: 5**

---

## ⛔ SC-§29 COVERAGE LEDGER — THIS GATE COVERS **`TASK-871` AND NOTHING ELSE**

| task | subject | covered here? |
|---|---|---|
| **`TASK-871`** | `SiegePlayerController.{h,cpp}` (the four-corner footprint slope probe) + `Tests/SiegePlacementTest.cpp` (+3 tests, 28→31) | ✅ **YES — this gate, and this gate ALONE, is the only review that work will get.** |
| `TASK-870` | the controls-help registry | ⛔ **NO** — `qa/TASK-872.md`, PASSED. ⛔ Out of scope here. |
| `TASK-874` | `Tests/SiegeAssistantSelectionTest.cpp` | ⛔ **NO** — `TASK-889`, has not returned. ⛔ Its `+1` is in the tree and is **UNGATED**. |
| `TASK-844` | the next writer of `SiegePlayerController.{h,cpp}` | ⛔ **NO** — not in the tree at my instant. |

⛔ **`qa/TASK-872.md` does NOT cover the slope trace and must never be cited as if it did.**

### 🔧 CONDITIONAL-PATH RELEASE FOR `TASK-873` — stated by name, as the row requires

`TASK-873` item (1a) holds **two** paths conditional on *"`TASK-871` HAS LANDED **AND** `TASK-914` HAS RETURNED PASS."*

✅ **BOTH CONDITIONS ARE NOW MET. THE CONDITION IS RELEASED FOR EXACTLY THESE THREE PATHS:**

```
Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h      (871 ✅914)
Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp    (871 ✅914)
Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp (871 ✅914 — the +3 rows)
```

⛔⛔ **AND THE FENCE THAT SURVIVES THIS PASS UNCHANGED: `Tests/SiegeAssistantSelectionTest.cpp` IS STILL UNGATED** (`TASK-874` → `TASK-889`, not returned) **and MUST NOT ENTER EITHER COMMIT.** My PASS releases the placement pair, ⛔ **not** the Assistant lane, and ⛔ **not** the `+1` sitting dirty beside it.

⛔ **Reconcile against `git status --porcelain` at your own instant** (`§25b` cl. R) — ⛔ not against this paragraph. I have no Git.

---

## ⛔ WHAT I DID NOT RUN (`TL-§5c` cl. 5) — ABOVE THE VERDICT, AS SPECIFIED

- ⛔ **I did not compile.** ⛔ **I did not run the suite.** The `431` below is a **DECLARED** census, ⛔ never a pass count.
- ⛔ No editor, ⛔ no MCP (editor UP, PID 11892, art task running), ⛔ no Git, ⛔ no PIE, ⛔ no edit to any source file.
- ⇒ ⛔⛔ **NOBODY HAS COMPILED `SiegePlayerController.{h,cpp}` SINCE THIS DIFF.** ⭐ **THE COMPILE DUTY IS TRANSFERRED BY NAME to whichever host build adopts `TASK-873`'s now-released conditional half.** That build is the **first** event that will discover a syntax or link error in this change. I have read for compile hazards (§4 below) and found none, but a read is ⛔ not a compile.
- ⛔ The balance table in §3 is **GEOMETRY AND CHANNEL SEMANTICS, ⛔ NOT OBSERVATION.** No terrain was traced by anyone.

---

## 1. ⭐⭐ THE FINDING THAT IS NOT IN THE ROW — **CONFIRMED REAL, AT TWO INDEPENDENT ENGINE SITES**

**Claim:** the shipped slope gate **FAILED OPEN** on a NaN surface normal.

### ✅ MEASURED AT SOURCE — and it is worse than the handoff says, because **BOTH** clamps launder

| site | measured text | NaN behaviour |
|---|---|---|
| `GenericPlatformMath.h:534` | `Acos(double V) { return acos( (V<-1.0) ? -1.0 : ((V<1.0) ? V : 1.0) ); }` | `NaN < -1.0` = **false**; `NaN < 1.0` = **false** ⇒ falls through to **`1.0`** |
| `UnrealMathUtility.h:592-595` | `Clamp(X, Min, Max) { return Max(Min(X, MaxValue), MinValue); }` | `Min(NaN, 1.0)` = `(A<=B)?A:B` → **`1.0`**; `Max(1.0, -1.0)` → **`1.0`** |

⇒ `acos(1.0) = 0` ⇒ ⛔⛔ **A NaN NORMAL READ AS *PERFECTLY FLAT* AND WAS ADMITTED.** The shipped expression `FMath::Acos(FMath::Clamp(NormalZ, -1.0, 1.0))` laundered it **twice over** — the explicit clamp written for float safety and `Acos`'s own internal clamp reach the identical wrong answer independently, so removing either one would not have saved it.

✅ **THE REPAIR IS CORRECT AND SITS IN THE RIGHT PLACE** — `SiegePlayerController.cpp:5468-5478`, an `IsFinite` test on `NormalZ` **BEFORE** the clamp, returning `180.f`. `180` is `acos`'s own range ceiling, ⛔ not an invented sentinel; every finite limit refuses it; it is legible in a log. Tested at `SiegePlacementTest.cpp:3296-3305`, both as a refusal **and** as an explicit `SlopeDegrees == 180` read, each paired with a flat-normal positive control.

⚖️ **IN FENCE.** The row's item (5) asked for the shape assertion; this *is* the slope gate, it is in the gate's declared fail-closed direction, and it was **declared, not smuggled**. ⭐ It was found by someone reading the **arithmetic** rather than the **specification**, which is the only way it could have been found — no behavioural test written against terrain will ever produce a NaN normal.

### ⚖️ PROPOSED LAW — for the manager to board, because this generalises well past this gate

> **A clamp written for RANGE safety silently launders NaN into the SAFE value.**
> Both `FMath::Clamp` and `FMath::Acos`'s internal clamp are ordered-comparison chains. **Every comparison against NaN is false**, so NaN falls through to the chain's **final** branch — and by construction that branch is the *benign* end of the range. ⇒ **any gate whose verdict passes through a clamp fails in whichever direction the clamp's last branch points, and the author never chose that direction.** Here it pointed OPEN, on a gate whose entire declared contract is to fail CLOSED.
> **The rule:** test the non-finite input **explicitly, before the clamp** — ⛔ never rely on comparison semantics to refuse it. *"A NaN would also refuse, but only through comparison semantics that nobody reading this should have to know"* (`SiegePlayerController.cpp:5475-5476`) is the correct instinct, written into the code, by the person who found it.

---

## 2. ⛔ THE TWO CORRECTIONS TO THE ROW — **BOTH VERIFIED INDEPENDENTLY, BOTH STAND**

### ✅ CORRECTION 1 — the limit is **20°**, and ⛔ NO DERIVATION CREPT IN

- `SiegePlayerController.h:2099-2100` — `UPROPERTY(EditDefaultsOnly, Category="Siegebound|Placement", meta=(ClampMin="0", ClampMax="90")) float MaxPlacementSlopeDegrees = 20.f;`
- ⛔ **Not `32.005°`, ⛔ not `44°`, ⛔ not derived from `AgentMaxSlope`.** I grepped the whole diff surface for `AgentMaxSlope`, `WalkableFloorAngle` and `32.005` — **zero occurrences** in `SiegePlayerController.{h,cpp}` and in the three new tests.
- ⭐ **The tests read it by REFLECTION, not transcription** — `SiegePlacementTest.cpp:3163` `TryReadShippedFloat(TEXT("MaxPlacementSlopeDegrees"), SlopeLimit)`, with a hard `AddError` + early return if the property is not a reflected float (`:3166-3170`). Every angle in test 30 is then **derived from that value** (`SlopeLimit * 0.5f` crown, `min(SlopeLimit * 2, (SlopeLimit + 90) * 0.5)` flank, `:3210-3211`), with a self-check that the derived pair genuinely straddles the limit (`:3212-3213`). ⇒ a retune of the tunable keeps every row meaningful instead of quietly making it vacuous. **This is `SC-§37` honoured, not merely cited.**
- ⚠️ The **only** transcription of `20.f` in the suite is the pre-existing tunable-registry table at `SiegePlacementTest.cpp:731`, which exists precisely to catch an unannounced retune. ⛔ Not this diff's, and correct in kind.

### ✅ CORRECTION 2 — the arena's hill flanks are **~27–27.5°**, ⛔ not 40°

Measured against `CONVENTIONS.md:184-190` (M6.6 climbable-hill law), ⛔ not relayed:

| mesh | base | **crown** | height | **max face angle** |
|---|---|---|---|---|
| `SM_Hill_01` knoll | r 700 | **r 220** | 250 | **~27.5°** |
| `SM_Hill_02` hill | r 1100 | **r 320** | 400 | **~27°** |
| `SM_Hill_03` ridge | 2200×1700 | **1400×350** | 350 | **~27.5°** |

Plus `CONVENTIONS.md:190`: *"every hill face angle ≤ **30°** … crown near-flat ≤ **8°**"*.

⇒ ✅ **THE FINDING SURVIVES INTACT — 27.5° clears the 20° limit by 7.5°, so hill flanks refuse today and refuse after.** The `40°` in `qa/TASK-816.md:98` is illustrative and is now **corrected on the record**. The handoff's crown table (r220 / r320 / short-axis 175) matches `CONVENTIONS` **exactly**; I re-derived the thresholds and they are right: `R > CrownRadius/√2` ⇒ 155.6 / 226.3 / 123.7 at ×1.0, and 103.7 / 150.9 / 82.5 at ×1.5.

### ✅ ITEM (0) — PERFORMED AND REPORTED THOUGH IT MATCHED (`SC-§40` cl. 9). **CONFIRMED.**

Located **by symbol** (`SC-§38`), ⛔ not by the row's line numbers:

| claim | my own measurement | verdict |
|---|---|---|
| ONE `LineTraceSingleByChannel` at the cursor | `cpp:5029-5031`, `SamplePoint ± 500 Z`, `ECC_Visibility`, `bTraceComplex=false` | ✅ |
| ⛔ **no radius parameter at all** | pre-diff signature took `(const FVector& Point)` only — the tests' own `SC-§41` control proves the old shape is gone (§5) | ✅ |
| **one** caller, passing `PlacementLocation` alone | exactly one call site, `cpp:2687`, inside `UpdatePlacementGhost` | ✅ |
| ×1.5 wheel | `PlacementFootprintMax = 1.5f` / `Min = 1.0f`, both reflected | ✅ |
| **SCALED** bounds | `cpp:5529` `GhostMesh->CalcBounds(GhostMesh->GetComponentTransform())` (`STACK-§6`) | ✅ |
| *"one line above the gate"* | it is **8 lines** above (`:2679` → `:2687`) | ✅ in substance — a dated hint, ⛔ not a defect |

---

## 3. ⭐ THE FIX — VERIFIED, ⛔ NOT ACCEPTED

### ✅ (a) THE FOOTPRINT IS **NOT** RE-DERIVED — and the `TASK-815` structural agreement **STAYED STRUCTURAL**

Read at `cpp:2678-2706`. One measurement, **three** consumers:

```
float FootprintRadius = 0.f;
const bool bFootprintKnown = bPendingIsBuilding && TryGetPlacementFootprintRadius(FootprintRadius);   // :2679  — the ONE read
... IsGroundSlopePlaceable(PlacementLocation, FootprintRadius)                                        // :2687  — NEW consumer
... HasBuildingClearance(PlacementLocation, FootprintRadius)                                          // :2697
... HasUnitClearance(PlacementLocation, FootprintRadius)                                              // :2702
```

⛔ **ZERO `CalcBounds(` / `TryGetPlacementFootprintRadius(` / `GetStaticMeshComponent(` inside the gate body** (`cpp:4972-5072`) — I read all 100 lines, not just the test's claim. The test asserts each zero **with a positive control** proving the same token IS findable elsewhere in the file (`:3388-3394`), which is `SC-§39` done properly.

⭐ **RE-MEASURED THAT `TASK-815`'s AGREEMENT IS UNDISTURBED:** `MakePlacementFootprintScale3D(` occurs **exactly 3 times on code lines** — `cpp:2362` (spawn transform), `cpp:2605` (ghost), `cpp:5338` (definition). ✅ **One pure static, two consumers, no third.** A fourth would be a consumer sizing something nobody reviewed. `StepPlacementFootprintScale(` = **2** (`:2862`, `:5301`) ⇒ ⛔ **no second clamp on the wheel's range** (`R-4c` / `HIGH-§1` booby trap). ✅

### ✅ (b) THE TWO ORDERING FACTS — **BOTH UNDISTURBED, BOTH RE-MEASURED AT SOURCE**

| ordering | assertion | source | verdict |
|---|---|---|---|
| the wheel polls **BEFORE** `UpdatePlacementGhost` | `SiegePlacementTest.cpp:2794` (`TASK-815`'s, test 27) | `cpp:785` `ApplyPlacementFootprintWheel();` → `cpp:788` `UpdatePlacementGhost();` | ✅ **INTACT** |
| the ghost is **SIZED** before its footprint is **MEASURED** | `SiegePlacementTest.cpp:2909` (`TASK-815`'s, test 28) | `cpp:2605` `SetActorScale3D(…)` → `cpp:2679` `TryGetPlacementFootprintRadius(…)` | ✅ **INTACT** |
| ⭐ **NEW, and this diff depends on it** — the footprint is measured **BEFORE** the slope probe | `SiegePlacementTest.cpp:3440` | `cpp:2679` → `cpp:2687` | ✅ **CORRECT AND ASSERTED** |

⭐ **The new row is the right instinct.** Written the other way, the probe would validate **this** frame's click against **last** frame's size — an intermittent refusal with a green suite, invisible to every behavioural test. `CheckPrecedes` (`:1694-1701`) fails loudly when **either** needle is absent (`INDEX_NONE` short-circuits the `TestTrue`), so it cannot go green on a renamed symbol.

### ✅ (c) FOUR PURE STATICS — headlessly testable, GC-safe, UE-correct

`cpp:5401-5501`, declared `public` in `SiegePlayerController.h:992/1026/1041/1059` (inside the `public:` block opened at `:207`, closed at `:1537` — so the test's direct `ASiegePlayerController::…` calls resolve). ⛔ Plain C++ statics, ⛔ not `UFUNCTION`s (no reflection is needed and none is claimed), ⛔ no defaulted parameter (`SC-§33`), ⛔ no world, ⛔ no member state, ⛔ no allocation. ✅ **Correct UE practice.**

### ✅ (d) SKIP-ON-MISS DID **NOT** BECOME REFUSE-ON-MISS — the asymmetry I was told to verify

Read at `cpp:5033-5059`:

- **CENTRE (`SampleIndex == 0`)** — miss ⇒ `return false`, same Verbose wording, same `Point.X/Y/Z` arguments. ✅ **Shipped fail-closed semantics preserved.**
- **CORNER** — miss ⇒ one Verbose line, then `continue`. ⛔ **Never a refusal.** ✅

⭐ **AND I PROVED THE PROPERTY RATHER THAN ACCEPTING IT: `IsGroundSlopePlaceable` CANNOT ADMIT ANYTHING IT REFUSES TODAY.** The centre sample is index 0, runs **first**, uses the **zero-vector** offset (so it traces the *identical* point), and its two refusal paths (miss, over-limit) are unchanged. Every corner can only add a `return false`. ⇒ the post-diff verdict is the **conjunction** of the shipped verdict with four new terms. **A conjunction can only ever get stricter.** ✅ This is the whole non-regression argument and it is airtight.

⭐ **The programmer's own defence of skip-on-miss also holds:** the ±500 Z bracket exceeds the shipped `Max terrain height 250` (`CONVENTIONS:153`), so a corner adjacent to a crown is always inside the bracket and a *steep* corner never misses. ⇒ **skip-on-miss cannot re-open the defect.** ✅ **UPHELD.** Making it refuse-on-miss would make every arena edge and every gap unbuildable with nothing in any log — the house null-safety law, correctly applied.

### ✅ (e) THE DEGRADE IS KEPT — measured, not promised

`NumPlacementSlopeSamples` (`cpp:5409-5418`) gates on `FMath::IsFinite(R) && R > 0.f`; unusable ⇒ `1`. `PlacementSlopeSampleOffset` (`cpp:5428`) uses the **identical** predicate to build `SafeRadius`, so the two agree about *"unusable"* **by construction rather than by coincidence** — and index 0 is the zero vector regardless. ⇒ ✅ **an unknown footprint collapses the probe to exactly one sample, at exactly the shipped point.** The call site passes the **bare `FootprintRadius`** (`:2687`), the same expression its `HasBuildingClearance` sibling gets (`:2697`), ⛔ not a second re-decided degrade. ✅

### ✅ (f) THE GATE CHAIN IS UNREORDERED — `TASK-914` (d)

`cpp:2687/2692/2697/2702` ⇒ **Slope → Obstacle → Clearance → Units**, `Units` still **LAST**, `first-failing-rule-wins` intact. `EffectiveBuildingClearance` (`cpp:5145-5156`) is still `FMath::Max(SafeBase, SafeRadius)` — **max, ⛔ not sum**, 1 occurrence; `SafeBase + SafeRadius` = **0**. ✅ Fence honoured and **re-measured after the diff**, which is what the row demanded.

---

## 4. COMPILE-HAZARD READ (no compile available — this is a read, ⛔ not a build)

✅ **UE 5.8 API surface is clean.** `LineTraceSingleByChannel`, `FCollisionQueryParams`, `SCENE_QUERY_STAT`, `ECC_Visibility`, `FMath::IsFinite/Clamp/Acos/RadiansToDegrees`, `FVector::ZeroVector/UpVector` — ⛔ **no deprecated or removed API in the diff.**
✅ Header/cpp signatures match on all five changed declarations. `static` correctly present in the header and absent in the definitions.
✅ `GhostActor` is `UPROPERTY(Transient) TObjectPtr<AStaticMeshActor>` (`h:3333-3334`) — GC-safe; `AddIgnoredActor(GhostActor)` from a `const` method resolves via `TObjectPtr::operator T*() const`.
✅ `TSet<FVector>` (test 29(f)) — `GetTypeHash(const TVector<T>&)` exists at `Math/Vector.h:2578`. ✅ Compiles.
✅ `TestEqual(FString::Printf(...), …)` — 30 existing precedents across 7 suite files.
✅ `BitsToFloat` / `MakeQuietNaN` / `Exact` / `Tolerance` / `TryReadShippedFloat` all live in `SiegePlacementTestFixture` (`:124/127/155/162`), pulled in by the `using` at `:3044` and `:3155`.
✅ Test 31's two `using namespace` directives are **not ambiguous**: `LoadProjectSource`/`CountOccurrencesInCode`/`ControllerSourcePath` exist only in `SiegePlacementUpgradeFixture` (`:929/945/988`); `ExtractControllerFunctionBody`/`CodeLinesOnly`/`CheckPrecedes` only in `SiegeDiscardAllFixture` (`:1639/1662/1694`). ⛔ No symbol is defined in both.
✅ Three new test class names and three new automation names are unique in the suite.
⚠️ Not a substitute for a build — see the transferred duty above.

---

## 5. ⭐ `SC-§41` — THE NEEDLE TRAP: **VERIFIED INDEPENDENTLY, AND THE DISCRIMINATOR IS REAL**

I ran the pair myself rather than reading the handoff's table:

| needle | my count on code lines | meaning |
|---|---|---|
| `IsGroundSlopePlaceable(PlacementLocation)` | **0** | the old point-only shape is genuinely gone |
| `IsGroundSlopePlaceable(PlacementLocation` | **1** | ⭐ the probe is **not blind** — the zero above is separation, not silence |
| `IsGroundSlopePlaceable(PlacementLocation, FootprintRadius)` | **1** (`cpp:2687`) | the one call site, correct shape |

✅ **The closing paren IS the whole discriminator, and both halves are asserted in the test itself** (`:3419-3423`), not merely greped once before shipping. This is the trap being *disarmed in the artefact*, which is the only version that survives the next edit. `FMath::Acos(` = **1** file-wide (`cpp:5484`) and **0** inside the gate body — verified by my own grep, matching `:3400-3403`.

---

## 6. CENSUS — `TL-§5b` / `TL-§5c` (⛔ DECLARED, ⛔ NEVER A PASS COUNT)

**Scoped pattern:** `^IMPLEMENT_[A-Z_]*AUTOMATION_TEST\(` over tracked `Source/**/Tests/*.cpp`.

- **Working tree at my instant: `431` declared across `31` files.** ✅ **Independently re-measured — matches the handoff exactly.**
- Per-file: `SiegePlacementTest.cpp` **31** (28 → 31, **+3**, this task's) · `SiegeControlsHelpTest.cpp` **18** (`TASK-870`, passed) · `SiegeAssistantSelectionTest.cpp` **39** (⛔ another lane, **UNGATED**).
- **HEAD `1aa0fee` = `425 / 31`.** ⇒ tree delta **+6**, decomposing as **+1 / +2 / +3** across three files. ✅ **The handoff's reconciliation is correct and I confirm it.**
- ⛔ **Last EXECUTED: `427 / 0` at `1aa0fee`** (`TL-§5c`). ⛔ **Nothing was executed by me or by `TASK-871`.**
- ⭐ **`qa/TASK-872.md`'s `427` was CORRECT WHEN WRITTEN (`425 + 2`) and was overtaken by the Assistant lane's `+1`.** ⛔ **It is NOT a missing test. Nobody should go hunting.**

### ⭐⭐ THE CORRECTION TO A NUMBER RELAYED A DOZEN TIMES — **TESTED AS THE RIGHT VARIABLE, AND THE DISPATCH IS RIGHT**

| needle | scope | result |
|---|---|---|
| `^IMPLEMENT_[A-Z_]*AUTOMATION_TEST\(` | `Source/**/Tests/*.cpp` | **431 / 31** |
| `^IMPLEMENT_` (bare) | `Source/**/Tests/*.cpp` | **431 / 31** ⭐ **IDENTICAL** |
| `^IMPLEMENT_` (bare) | `Source/**/*.cpp` | **432 / 32** |

⇒ ✅ **CONFIRMED: it is a SCOPE difference, ⛔ NOT a second needle.** Over `Tests/*.cpp` the two needles agree **exactly**. The 32nd file is `Source/GitClaudeUnrealTest/GitClaudeUnrealTest.cpp`, whose single hit is `IMPLEMENT_PRIMARY_GAME_MODULE` — ⛔ **not a test, and it can never be one.** The "bare-needle trap" framing has been mis-attributed for a dozen relays: **the variable that moves the number is the file glob, not the pattern.** (See NIT-1.)

---

## 7. ⚖️ THE FLAGGED DECISION — **RULED**

> **Corners at `(±R, ±R)` — the corners of the half-extent box — rather than at distance `R` on the diagonal.**

### ⚖️ **RULING: UPHELD. KEEP `(±R, ±R)`. ⛔ DO NOT FLIP IT.**

**Premise verified at source, ⛔ not accepted:** `PlacementFootprintRadiusFromBounds` (`cpp:5126-5143`) returns `FMath::Max(|ExtentX|, |ExtentY|)` — a **half-extent of the world-axis-aligned box**, ⛔ **not a circumradius**. The programmer's premise is exactly right.

**Four reasons, in order of weight:**

1. ⭐⭐ **The alternative is not "less conservative", it is WRONG.** Sampling at distance `R` on the diagonal puts the sample at `(R/√2, R/√2)` — **inside** the very box the value describes, missing the real corner by 29%. That is not a tuning choice; it is a sample that **does not touch the footprint it claims to measure.** The under-refusal it buys is precisely the defect this task exists to close, re-introduced at 71% strength.
2. ⚖️ **This gate's declared direction is CLOSED.** It is the only member of the placement family that already fails closed on a trace miss. Over-refusal is its declared direction; under-refusal is the thing it was boarded to delete. When a residual must land on one side, it lands on the declared side.
3. ⚖️ **The asymmetry against `HasUnitClearance` is justified, and the programmer's reason is the correct one.** The unit gate over-refusing costs playability against a **dense, mobile** hazard in a busy spawn box; steep terrain is **sparse and static**. Different hazard densities warrant different residual directions. ⛔ That is not inconsistency; it is the same principle applied to two different populations.
4. ⭐ **It is the reading that cannot drift.** Deriving the sample from *what the value IS* means a future change to `PlacementFootprintRadiusFromBounds` carries the probe with it. A diagonal-`R` reading would silently become wrong the day that function returned a circumradius instead.

**⚠️ THE DECLARED PRICE, RESTATED SO IT IS ON THE RECORD:** for a **round** or strongly **oblong** footprint the short-axis corners sit beyond the mesh — over-refusing by up to **~41%** of the radius. ✅ **Accepted, and it is genuinely the mirror of the residual `PlacementFootprintRadiusFromBounds` already declares in the other direction.**

**⭐ THE BETTER FIX, RECORDED AS A FOLLOW-ON — ⛔ NOT ASKED FOR HERE AND ⛔ NOT A CONDITION OF THIS PASS:** the ~41% residual exists only because `max(|X|,|Y|)` **collapses two half-extents into one scalar**. Carrying `FVector2D(ExtentX, ExtentY)` and sampling at `(±X, ±Y)` would make the probe exact for **every** rectangular footprint and delete the residual outright — but it changes a shipped signature consumed by three gates and is a `TASK-735`-sized change, ⛔ **not a one-line flip.** Board it if the over-refusal is ever felt in play; ⛔ do not do it inside this task's fence.

---

## Findings

### BLOCKER — none.

### WARN

- **[WARN] W-1 — `SiegePlayerController.cpp:5029-5031` — ⛔⛔ THE BALANCE REPORT MODELS THE PROBE AS A *TERRAIN* PROBE. THE SHIPPED TRACE IS `ECC_Visibility` AND IGNORES ONLY `GhostActor`.**
  The (2a) table classifies the world as *"flat arena slab / hill flanks / hill crowns"* and concludes *"the flat arena slab is **BIT-IDENTICAL** (0° everywhere)"*. ⛔ **That sentence is over-broad, and the board has already copied it verbatim.** The trace channel is `ECC_Visibility` and the only ignored actor is the ghost (`:5014-5017`) ⇒ **a corner sample lands on whatever blocking geometry occupies that XY**, which on this arena includes **other buildings** (the file's own comment at `:2720-2722` records *"Buildings root a BlockAll VisualMesh … so they answer the Visibility trace"*), **rocks and hills** (`CONVENTIONS:205` — the HISM blocks `ECC_Visibility`), and **the castle's own geometry** — inside which placement is *deliberately legal* since `TASK-349` retired the plinth keep-out.
  ⚖️ **Why this is a WARN and ⛔ not a blocker:** it is fail-closed, it cannot admit anything currently refused (§3(d)), and the **outcome is genuinely undecidable headlessly** — a straight-down trace onto a tall blocker hits either its **flat top** (⇒ ~0°, admits) or starts **inside** it (⇒ `bStartPenetrating`, normal typically `-Dir` = +Z, ⇒ admits; a degenerate zero normal would read 90° and refuse). ⛔ **Neither the programmer nor I can settle which, without PIE.**
  ⇒ **Owed:** the report must say *"any `ECC_Visibility` blocker within `R·√2`"*, ⛔ not *"terrain"*, and the castle-interior case needs 🧑 Jonathan's eye (see §8).

- **[WARN] W-2 — `SiegePlayerController.cpp:5494-5497` — A **FOURTH** BEHAVIOUR CHANGE, ⛔ ABSENT FROM THE HANDOFF'S *"all three, declared"* LIST.**
  `IsSurfaceNormalWithinSlopeLimit` now refuses when the **LIMIT** is non-finite. Pre-diff, the comparison was `SlopeDegrees > MaxPlacementSlopeDegrees`, and `x > NaN` is **false** ⇒ a NaN limit **silently disabled the whole gate**. This is a second, independent fail-open→fail-closed change, in the same family as the headline finding but at the *other* operand.
  ⚖️ **The code and the test are BOTH correct and BOTH declare it** (`h:1047-1050`, tested at `:3302-3303` with a flat-normal control). ⛔ **Only the handoff's own enumeration is incomplete** — and a list that says *"all three"* invites the next reader to stop counting. ⇒ **it is four.**

- **[WARN] W-3 — `SiegePlayerController.cpp:5026` — the corner brackets ±500 Z from the **CENTRE's** Z, and there is ⛔ no height-agreement term.**
  A corner over a flat ledge, roof or shelf up to 500 uu above or below the centre reads **0° and ADMITS**. ⛔ Not a regression (it admitted before too, from the centre alone) and ⛔ not in fence — but it **bounds what this fix buys** and is not stated anywhere: the gate now proves *"every corner stands on something flat"*, ⛔ **not** *"every corner stands on the same surface as the centre."* Worth one sentence in `TOWER-§7` so the next reader does not over-trust it.

- **[WARN] W-4 — `Tests/SiegePlacementTest.cpp:3184-3196` — the NEGATIVE CONTROL exercises the **ADJUDICATOR**, ⛔ not the **PROBE**.**
  Test 30(b)'s sample normals are **hand-supplied** `FVector::UpVector`s, so *"a flat pad at ×max is still admitted"* reduces to `WithinLimit(Up, 20) == true` — a claim independent of the radius. ⭐ **The author knows this and says so** (`:2961-2969`, `SC-§32`), and mitigates it properly with (b2) proving the offsets genuinely reached further at ×max than at ×min (`:3201-3204`) plus test 31's source probe proving the shipped loop has that shape.
  ⇒ ✅ **`TASK-914` (c)'s requirement is SATISFIED — but only by 30(b) + 30(b2) + 31 TOGETHER.** ⛔ **Do not let anyone cite 30(b) alone as "the negative control".** It is not one on its own.

- **[WARN] W-5 — `SiegePlayerController.cpp:5055-5057` — up to **4 extra Verbose lines per frame** during placement mode.**
  The corner-miss log fires per corner per frame. ⚠️ 🧑 **Jonathan runs `Log LogGitClaudeUnrealTest Verbose` for the climb instrument** — with placement mode open near an arena edge this now emits ~240 lines/second into the same log he is reading. ⛔ Not wrong (it mirrors the shipped centre-miss line and `UE_LOG` does not evaluate its arguments below the verbosity threshold), but the **volume** is new. Consider `VeryVerbose` for the corner-skip line specifically.

- **[WARN] W-6 — `SiegePlayerController.cpp:5063-5066` — *"byte-for-byte"* applies to the **MISS** path, ⛔ not to the **REFUSAL** path.**
  The slope-exceeded Verbose line gained `sample %d of %d`, the footprint radius, and now prints `SamplePoint` rather than `Point` (identical for the centre, different for corners). ⛔ Correct and better — but the handoff's *"the centre keeps its shipped fail-closed semantics **byte-for-byte**"* is true of `:5041-5043` and **not** of `:5063-5066`. Log-scrapers and any doc quoting the old string are affected.

### NIT

- **[NIT] N-1 — handoff §Census — the *"bare `^IMPLEMENT_` trap reads 432/32"*** line attributes to the **needle** what belongs to the **scope**. Measured both ways (§6): over `Tests/*.cpp` the two needles are **identical at 431/31**; `432/32` appears only when the glob widens to `Source/**` and picks up `IMPLEMENT_PRIMARY_GAME_MODULE`. ⭐ The number is right, the cause is not — and this mis-attribution has now been relayed a dozen times.
- **[NIT] N-2 — `CONVENTIONS.md:152-153` vs `:190` vs `SiegePlayerController.h:2097`** — three live statements of the crown law: *"≤10°, r200"*, *"≤ **8°**"*, and *"crowns (<=10°) stay legally placeable"*. ⛔ Pre-existing, ⛔ out of this fence, ⛔ harmless here (8 and 10 both clear 20) — but the balance argument leans on it, so one of the three should be struck.
- **[NIT] N-3 — `Tests/SiegePlacementTest.cpp:3336-3359`** — the brace matcher counts braces inside **strings and comments**. It works today (I read `cpp:4972-5072`: no brace in any comment or literal) and it is self-checked four ways so it fails **loudly** in both directions — under-run turns rows (a)/(b)/(c) RED, over-run trips the `HasObstacleClearance` check. ⭐ **Accepted as new instrument surface** (`SC-§39.1`), with the limit recorded: a future `{` in a comment inside that function silently re-bounds it.
- **[NIT] N-4 — `SiegePlayerController.cpp:5477`** — the `180.f` sentinel is refused by every limit ≤ 90, which `ClampMax="90"` enforces in the editor. A hand-edited `.uasset` carrying a limit **≥ 180** would re-admit a NaN normal. Unreachable in practice; recorded only because W-2 shows hand-edited limits are the exact threat model this file already reasons about.
- **[NIT] N-5 — handoff §(0)** — *"computed **9 lines** above the gate"*; measured, it is **8** (`:2679` → `:2687`). Trivial, and the handoff correctly flags the whole quantity as a dated hint.

---

## 8. 🧑 FOR JONATHAN — **DOES IT REACH HIM CLEARLY? MOSTLY. ⛔ IT NEEDS ONE ADDITION AND ONE PIXEL ROW.**

### ✅ WHAT IS STATED WELL ENOUGH, AND I AM ENDORSING IT

> **On a hill crown, wheeling a building larger can now turn a green ghost RED.**

✅ **That sentence reaches him.** It is concrete, player-facing, in his own vocabulary (*green ghost / red ghost / the wheel*), and it names the exact gesture. ⭐ **And the programmer did the right thing twice over: it tuned NOTHING, and it answered the balance question with ARITHMETIC rather than a shrug.** The flat slab is 0° everywhere so it is untouched; flanks at 27.5° already refused; ⇒ the change bites **on crowns alone**, when `R·√2 > CrownRadius`. That derivation is correct and I have re-checked every number in it.

⚠️ **The per-card `R` is correctly flagged as a DATED HINT, ⛔ not a fact** — it comes from the ghost's scaled bounds at runtime and needs the editor. ⭐ **Flagging it instead of guessing is right**, and `TOWER-§7`'s own record shows why: the WatchTower footprint was re-authored **2,700 → ~750 uu one day after** the rule quoting it was written (`cpp:5119-5121`).

### ⛔ THE ADDITION IT NEEDS — W-1, in one sentence he can act on

The report tells him *"crowns alone"*. That is true **of terrain**. It should also say:

> ⚠️ *"The probe traces on the same channel as the cursor, so a footprint corner can also land on **another building, a rock, or the castle's own geometry** — not just on the ground. Where that happens, a refusal that used to read **'Too close to buildings'** or **'Too close to obstacles'** may now read **'Too steep'**, and a few placements that squeeze past the clearance rules today may stop being allowed at all."*

⛔ **Without that sentence he will read "crowns alone", meet a "Too steep" on flat ground next to his own tower, and file it as a new bug.**

### ⛔ YES — IT NEEDS A PIXEL ROW. **TWO ROWS, AND THE SECOND IS THE IMPORTANT ONE.**

| # | what to do | what would be wrong |
|---|---|---|
| **P-1** | Place a **large** building on a **hill crown**, then **wheel it up to max**. | Expected: green → **RED** as it grows. ⭐ This is the feature. If it stays green at max on `SM_Hill_01`'s r220 crown, the probe is not reaching. |
| **P-2** ⭐⭐ | **INSIDE THE CASTLE**, place a building **hard against an interior wall** — then step it away one nudge at a time. | ⛔⛔ **THE ONE I CANNOT ANSWER HEADLESSLY.** If it refuses with **"Too steep"** anywhere it used to place, W-1 is live and the castle-interior placement `TASK-349` shipped has quietly narrowed. ⛔ **A "Too steep" beside a vertical wall is a FALSE MESSAGE either way** — the ground there is flat. |

⚖️ **P-2 is the row that decides whether W-1 is a documentation fix or a real balance change.** It costs him about thirty seconds and it is the only instrument that exists.

---

## 9. ⚖️ ITEM (4) — **ATTACKING THE RULING, AS INSTRUCTED. ⛔ NOT A RECOMMENDATION.**

> **Ruled:** the `§25b` cl. R reconciliation runs at **DISPATCH** and at **BOARDING**; the **manager** globs `handoffs/TASK-###-*.md` per named subject and writes the dated result into the gate row's `blocked-by:`; a gate row without that line is not dispatchable.

**I attack it on five points and I do not contradict it.**

**The ruling's predicate is EXISTENCE. Its subject is DELIVERY. Those are different propositions, and the gap between them is where every one of tonight's failures lives.** (1) **A handoff can exist and describe a refusal, a partial, or a blocker** — `TASK-874`'s own handoff correctly *reported rather than reached* on `Capture()`'s missing filter; a glob reads PRESENT for a file whose entire content is *"I could not do this."* (2) **A handoff can be STALE.** Had `TASK-871` been delivered, reverted and re-dispatched, the old note would sit on disk and the glob would say PRESENT while the diff was gone — ⛔ **this is `TL-§5d`'s tree-vs-HEAD problem wearing a dispatch-time costume**, and the mirror image bit *this very gate* tonight: `qa/TASK-872.md`'s `427` was **correct when written and wrong when read**. A file-existence check is exactly as time-blind as that census was. (3) **It measures the wrong artefact class.** The **diff** is the deliverable; the **handoff is its receipt** — and both are written by the same agent, so an agent that writes the note but never saves the source edit produces a PASSING predicate and a missing good. **No receipt-based check can detect an absent good.** (4) **The per-*task* glob does not match the per-*`names:`-entry* delivery.** `TASK-872` is the live instance: had `handoffs/TASK-871-programmer.md` existed covering only the tests and not the gate, the glob would read PRESENT and the reviewer would *still* have arrived at half an empty subject — ⛔ the same failure, one file later. (5) **The cost it imposes on me is not effort, it is a new false-confidence surface.** A `blocked-by:` line reading *"✅ VERIFIED PRESENT 2026-09-03"* is precisely the relayed measurement that `SC-§40` cl. 3 already obliges me to re-measure. If I trust it, the law has bought a **second source of truth** — the defect this gate family exists to prevent. If I re-measure it, the boarding-time check saved me nothing; its whole value is upstream, in not *dispatching* a doomed gate. ⇒ ⚖️ **The ruling is NECESSARY and CORRECT and I would not repeal it. It is not SUFFICIENT, and the cheap predicate was chosen knowingly — my only ask is that the row never be allowed to READ as sufficient.**

**⭐ ONE CONCRETE STRENGTHENING THAT COSTS THE MANAGER NOTHING EXTRA** (it is the same shell call `§25b` cl. R already mandates): pair the handoff glob with `git status --porcelain` and require **at least one file from the gate row's own `names:` list to appear dirty** (or newer than the gate row's base commit). That converts an **existence** predicate into a **delivery** predicate and closes failure modes (1), (2) and (4) in one line. ⛔ **It does not close (3)** — and the honest limit is worth writing down: **no dispatch-time predicate can detect a handoff that is present, recent, and describes work that was never written.** Only a reviewer reading the source can, which is what happened tonight in both directions — the gate found the empty subject, and this gate found a NaN defect that no specification mentioned.

---

## Notes for build-master (PASS)

1. ⭐ **`TASK-873`'s conditional half is RELEASED** — the three paths named in the ledger at the top of this report. ⛔ **Verify with `git status --porcelain` at your instant, ⛔ not from that list.**
2. ⛔⛔ **`Tests/SiegeAssistantSelectionTest.cpp` STAYS OUT** — `TASK-874` is ungated (`TASK-889` has not returned). Its `+1` is dirty in the tree **right beside** the files you are staging. ⛔ **This is the trap.**
3. ⛔ **YOU ARE THE FIRST COMPILE.** Nobody has built `SiegePlayerController.{h,cpp}` since this diff. Expect it to be clean (§4), but the build is the measurement.
4. ⛔ **`TL-§5c`: EXECUTE the suite, ⛔ do not census it.** `-ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended`; report **both** `Result={Success}` and `Result={Fail}` verbatim. **`431 declared / 31 files` is my DECLARED figure and is ⛔ NOT a pass count.** Expected new rows: three, all in `SiegePlacementTest.cpp`, named `Siegebound.Placement.TheSlopeProbe*`.
5. ⚠️ **Test 31 reads `SiegePlayerController.cpp` from disk.** If it goes red immediately after a commit, check the working tree matches HEAD before blaming the diff.
6. ⚠️ **`TASK-844` is the next writer of this header — SERIALISE.** A concurrent edit lands in files I have just gated.
7. 🧑 **Carry W-1's one-sentence correction and the two pixel rows (§8) to the manager** — the *"crowns alone"* line is on the board verbatim and is over-broad.

---

**Verdict: PASS — 0 blockers, 6 WARN, 5 NIT.**
**Gate covers `TASK-871` ALONE (`SC-§29`).**
