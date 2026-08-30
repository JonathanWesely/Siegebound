# TASK-736 — handoff (gameplay-programmer)

**Both TASK-730 WARNs repaired. ONE file. Test-only. ⛔ Zero shipped-code change, ⛔ zero behaviour change.**

- **File touched (SOLE):** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp`
- **Suite-count effect: ZERO. Final total = `171`.** (Declared loudly below — §4.)
- **⛔ Not compiled** — TASK-731 owns the only compile and this rides it. ⛔ No editor, no MCP, no Git.

---

## 1. REPAIR 1 — the second `152.4f` literal (`qa/TASK-730.md` W-1)

### BEFORE (`:421`)
```cpp
TestEqual(TEXT("PRECONDITION: the shipped elevation step is 152.4 uu (5 ft × 30.48 cm)"), StepUU, 152.4f, Tolerance);
```

### AFTER
Added to the `SiegeClimbableTowerTestFixture` namespace, directly mirroring `SiegeHighGroundTest.cpp:80-88`:
```cpp
constexpr float CentimetresPerFoot = 30.48f;
constexpr float FeetPerStep = 5.f;
constexpr float ExpectedStepUU = FeetPerStep * CentimetresPerFoot; // = 152.4 uu
```
and at the assertion site:
```cpp
// ⭐ The expectation is DERIVED (5 ft × 30.48 cm/ft), ⛔ never re-typed as 152.4f — see the
// fixture's note. Tolerance, not Exact: 5.f * 30.48f and 152.4f differ by ~4e-6 in float32,
// which is four orders of magnitude inside the band, while every wrong step in the trap
// list (150 / 152 / 5 / 500) blows straight through it.
TestEqual(TEXT("PRECONDITION: the shipped elevation step is 5 ft × 30.48 cm/ft (= 152.4 uu)"), StepUU, ExpectedStepUU, Tolerance);
```

**DERIVED, ⛔ not restated** — exactly how TASK-724 already solved this. The fixture comment carries both halves of the reasoning: 724's epistemic one (*"an expectation transcribed from the subject cannot disagree with a wrong subject"*) **and** the maintenance one QA named — `HeightBonusStepUU` is `EditDefaultsOnly` **precisely so Jonathan can retune it**, and a magic `152.4` buried in a *tower* test is a booby trap for the day he does.

**⛔ The assertion was KEPT.** It is a real precondition, it checks against the CDO, and it still fails loudly on a retune. Only its hardcoding is gone.

**Naming note:** the derived constant is `ExpectedStepUU`, ⛔ not `StepUU` — `StepUU` is already the local holding the reflectively-read CDO value in test 5, and shadowing it would have been the one way to make this repair a bug.

### ✅ Re-measured, ⛔ not asserted
```
grep -rn "152\.4f" Source/
  SummonedUnit.h:923                 float HeightBonusStepUU = 152.4f;   ← the ONE shipped literal
  SiegeClimbableTowerTest.cpp:140    (prose — "⛔ Deliberately NOT written as `152.4f`")
  SiegeClimbableTowerTest.cpp:439    (prose — "⛔ never re-typed as 152.4f")
  SiegeClimbableTowerTest.cpp:440    (prose — the float32 tolerance note)
  SiegeHighGroundTest.cpp:83         (prose — 724's original note)
```
⇒ **exactly ONE code literal codebase-wide. `HIGH-§1` is RESTORED.** The four remaining hits are comment prose, the same category QA already accepted for HighGroundTest's 8 occurrences. Two of them are mine and both are the *"do not write this literal"* note itself.

**Float32 check (why `Tolerance`, ⛔ not `Exact`):** `5.f * 30.48f` = 152.39999771…, `152.4f` = 152.39999389…, delta ≈ **3.8e-6** against a `1.e-4f` band — four orders of magnitude of headroom, while 150 / 152 / 5 / 500 all blow straight through. This is the same reason 724 uses `Tolerance` for its derived expectations.

---

## 2. REPAIR 2 — the redundant parity assertion (W-2) — ⭐ **ROUTE TAKEN: (A) MADE IT REAL**

### BEFORE (`:431-434`, `:449-454`)
```cpp
const float TargetZ = 0.f;
const float TowerPlatformRiseUU = TowerDefaults->GetPlatformHeightUU();
constexpr float HillCrestRiseUU = 1200.f;
...
const float HillMultiplier = ASummonedUnit::HeightAdvantageMultiplier(TargetZ + HillCrestRiseUU, TargetZ, StepUU, BonusPerStep);

TestEqual(TEXT("(a) The tower's platform rise and the hill's crest rise are the same height"),
    TowerPlatformRiseUU, HillCrestRiseUU, Tolerance);
TestEqual(TEXT("(a) ⭐ …and therefore the IDENTICAL damage multiplier — a hill and a tower can never disagree"),
    TowerMultiplier, HillMultiplier, Tolerance);
```
Both sides collapsed to the identical call `H(1200, 0)`.

### AFTER
```cpp
constexpr float BasinFloorZ = -500.f;
...
//   TOWER: target on the flat at 0,          attacker on the platform at  1,200.
//   HILL:  target on a basin floor at −500,  attacker on the crest    at    700.
const float HillMultiplier = ASummonedUnit::HeightAdvantageMultiplier(BasinFloorZ + HillCrestRiseUU, BasinFloorZ, StepUU, BonusPerStep);

TestEqual(TEXT("(a) The tower's platform rise and the hill's crest rise are the same height"),
    TowerPlatformRiseUU, HillCrestRiseUU, Tolerance);
TestEqual(TEXT("(a) ⭐ …and therefore the IDENTICAL damage multiplier, though the hill sits 500 uu lower in the world — a hill and a tower can never disagree"),
    TowerMultiplier, HillMultiplier, Tolerance);
```
plus a load-bearing comment block explaining that the basin is **not decoration**.

### ⭐ WHY ROUTE A AND ⛔ NOT DELETE-AND-COMMENT
The board offered delete-and-comment (724's R-6 practice, ruled house practice) **only if this file structurally cannot express a real pair**. It can:

- `HeightAdvantageMultiplier` is a free static taking `(AttackerZ, TargetZ, StepUU, BonusPerStep)` — the test can site either pair at any absolute Z with no world, no actor and no new surface.
- The two heights already came from **genuinely different sources**; the defect was only that both pairs sat at `TargetZ = 0`, which collapsed them to one call. QA's own suggested fix was exactly this (`BasinZ = -500.f`).
- ⇒ Deleting a claim I *can* honestly make would have thrown away real coverage. **Delete-and-comment is for a claim that cannot be made; this one can.** The practice was considered and correctly not needed.

### ⭐ IT IS NOT A DUPLICATE OF 724's TEST 7
724's test 7 compares two **typed** hypothetical heights. This assertion's left side is `TowerDefaults->GetPlatformHeightUU()` — **the shipped tunable read off the class under test**. So this is the tower-specific instance of the property, and it is this file's own claim to make.

### It can now actually fail (`SHIP-§9c`)
- **Absolute-Z reading** — `1 + 0.1 × AttackerZ/Step` scores tower ×1.787 vs hill ×1.459. **FAILS.** (Nothing else in this file caught that.)
- **Any tower special case keyed on a world height threshold** — the two pairs straddle it. **FAILS.**
- **A `PlatformHeightUU` retune** — still fails, as before (shared with the height equality above, which is unavoidable and harmless).
- **Against the shipped implementation it passes:** `max(0, 700 − (−500)) = 1200` ⇒ ×1.7874, identical to `max(0, 1200 − 0) = 1200`. ✅

### ⚠️ Declared: `(b)` and `(c)` are byte-unchanged in CODE
- **(b)** pins `TowerMultiplier == 1.78740157f`. `TowerMultiplier` is still `H(TargetZ + TowerPlatformRiseUU, TargetZ)` = `H(1200, 0)`. **Untouched.**
- **(c)** computes `H(TargetZ + HillCrestRiseUU − StepUU, TargetZ)` and compares the drop against `TowerMultiplier`. Both sit at `TargetZ = 0`, so it is still correct and still passes: `1.7874 − 1.6874 = 0.10 = BonusPerStep`. ✅

**⚠️ ONE PROSE FIX I MADE NECESSARY, DECLARED RATHER THAN SLIPPED IN:** `(c)`'s comment said *"Drop **the hill** by exactly one step"*, but after my edit "the hill" lives in the basin while `(c)`'s crest is sited in the tower's world. I reworded it to *"the crest"* and added two lines stating that `(c)`'s crest is deliberately in the **tower's** world (it is compared against `TowerMultiplier`, so it must share `TowerMultiplier`'s target) and that `(a)` having just proven the rises equal is what licenses that. **⛔ Code unchanged — comment only.** This is the M7.7 prose-drift lesson: an edit that invalidates its own neighbouring prose fixes it in the same edit.

---

## 3. Fences — all honoured, verified by mtime ⛔ not by claim

`find Source/ Docs/ Tools/ -newermt "-30 minutes"` returned exactly one source file: **`SiegeClimbableTowerTest.cpp`**.

⛔ **NOT touched:** `SummonedUnit.{h,cpp}` · `ClimbableTower.{h,cpp}` · `SiegeHighGroundTest.cpp` (**read only**, to mirror its pattern) · `WarMapWidget.*` · `cards.csv` · every shipped header · `Tools/Packaging/` · TASK-716/700/735.

⚠️ **For QA's awareness, ⛔ not my doing:** `git status` shows `SummonedUnit.{h,cpp}`, `WarMapWidget.*`, `SiegeWarMapTest.cpp`, `SiegeControlsHelpWidget.cpp`, `cards.csv` and `pipeline_manifest.json` as modified. **That is the rest of this batch's uncommitted work (722/723/724/729) plus the art-director's live TASK-728 chain writing under `Tools/ArtPipeline/`** — all of it predates or runs parallel to me. My own file is still **untracked** (`??`), because TASK-726 created it and nothing has committed yet, which is why it shows no `git diff` line.

⛔ No compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ no `Content/`.

---

## 4. ⚠️⚠️ SUITE-COUNT EFFECT — DECLARED EXPLICITLY

# THE FINAL SUITE TOTAL IS `171`. UNCHANGED.

**Re-measured now, in the working tree** — `^IMPLEMENT_.*_AUTOMATION_TEST(` at line start across `Siegebound/Tests/`:

```
WarMap 27 · AssistantSelection 28 · StuckStatics 20 · ControlsHelp 13 · AssistantGrammar 12 ·
DeckSlots 12 · HighGround 9 · Cloud 9 · AssistantGuard 9 · Account 7 · Settings 7 ·
KeyboardLayout 7 · ClimbableTower 5 · AssistantZoneA 5 · CastleTransform 1   = 171 in 15 files
```

⭐ **`ClimbableTower` is still 5.** I added and removed **zero** `IMPLEMENT_*` tests — repair 2 modified an assertion's operands, and **assertions are not tests**. Repair 1 modified an assertion's expected value and added three `constexpr` floats.

⇒ **TASK-731 asserts `171`, exactly as `qa/TASK-730.md` reconciled it (156 + 9 + 5 + 1 + 0).** ⛔ There is no drift for build-master to chase.

---

## 5. What QA should scrutinise

1. ⭐ **That `ExpectedStepUU` does not shadow test 5's local `StepUU`** — the CDO-read value must remain the *subject* and the derived constant the *expectation*. If those ever swapped, the precondition would assert the compiler's multiplication against itself and become exactly the vacuity 724 deleted at `SiegeHighGroundTest.cpp:201-204`. Check the argument order at the `TestEqual`.
2. **That `Tolerance` (⛔ not `Exact`) is right for the derived comparison** — my float32 delta figure is ~3.8e-6 against a 1e-4 band. Please re-derive rather than trust it.
3. ⭐ **That repair 2 is genuinely failable now** — the pairs are `(1200, 0)` and `(700, −500)`. Confirm they are not equal by construction under *any* reading, and confirm my ×1.459 figure for the refuted absolute-Z reading.
4. **That `(b)` and `(c)` still hold with the hill re-sited** — `(c)` in particular deliberately does **not** use the basin, and my comment says why. That is a judgement call worth a second eye.
5. **That the route-A choice was right.** If QA thinks the tower-side/hill-side distinction is too thin to be worth an assertion, delete-and-comment (724's R-6) is still available and I would take that ruling without argument.
6. **The `−500` in comments is U+2212, not ASCII hyphen** — consistent with the file's existing heavy Unicode (⛔ ⭐ × §). The code literal is plain ASCII `-500.f`.
