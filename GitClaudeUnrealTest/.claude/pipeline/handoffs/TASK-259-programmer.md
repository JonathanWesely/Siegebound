# TASK-259 — Floating-units fix (Archer/Ogre) — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-07-23 · **Branch:** m7.6-arena10x · **Status:** ready-for-qa
**Ref:** `.claude/pipeline/handoffs/DIAG-floating-units.md` (SESSION-2 — the measured diagnosis this fixes)
**No compile performed** (per instruction — the finishing integration TASK-258 compiles the mines batch + this fix together).

---

## What changed

One file: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp`, function `ASummonedUnit::ResolveSkeletalVisual()`.

Immediately AFTER the `SkeletalVisualMesh->SetSkeletalMeshAsset(SkeletalAsset)` swap, pin the skeletal
component to the known-good static-mesh base offset:

```cpp
if (bVisualMeshBaseCached)
{
    SkeletalVisualMesh->SetRelativeLocation(VisualMeshBaseRelativeLocation);
}
```

(plus a block comment documenting the root cause and the cache-ordering guarantee). This is the DIAG SESSION-2
"Fix A" — the permanent one. No BP, no SK asset, no static VisualMesh path, no spawn/movement code touched.

### Why it fixes it
`ASummonedUnit` creates `SkeletalVisualMesh` at RelativeLocation (0,0,0) in the constructor. TASK-159
hand-authored the `−CapsuleHalfHeight` Z into every unit BP that existed then; `ResolveSkeletalVisual` only
swapped the SK asset and trusted the BP for the Z. Archer/Ogre were first-imported later (TASK-242/243, zero BP
changes), so their `SkeletalVisualMesh` kept Z=0 → the mesh rendered one capsule-half-height above the correctly
grounded capsule (Archer +90, Ogre +145). Pinning the component to the cached static-mesh base corrects both and
removes the dependency on per-BP authoring for all future first-imports.

---

## Cache-ordering verification (the one subtlety — CONFIRMED VALID)

`VisualMeshBaseRelativeLocation` / `bVisualMeshBaseCached` are populated in `BeginPlay` (SummonedUnit.cpp:154-157):

```cpp
if (VisualMesh && !bVisualMeshBaseCached)
{
    VisualMeshBaseRelativeLocation = VisualMesh->GetRelativeLocation();
    bVisualMeshBaseCached = true;
}
```

`VisualMesh` is a constructor-created default subobject (never null), so the cache is always populated after
BeginPlay. Traced every call path to `ResolveSkeletalVisual` (grep-confirmed it is called from exactly ONE site):

- `ResolveSkeletalVisual()` ← called only from `LoadStatsAndStart()` (SummonedUnit.cpp:950).
- `LoadStatsAndStart()` ← called from:
  - `BeginPlay()` line 182 — AFTER the cache block at 154-157. ✓ (covers the deferred-spawn path:
    SpawnActorDeferred→InitUnit(pre-BeginPlay, bStatsLoaded=false)→FinishSpawning→BeginPlay→cache→LoadStatsAndStart).
  - `InitUnit()` line 437 — guarded by `if (HasActorBegunPlay())`, i.e. BeginPlay already ran → cache already
    populated. ✓ (covers the plain SpawnActor+InitUnit path.)

**Conclusion:** in every path that can reach the swap, the cache is populated first, so
`VisualMeshBaseRelativeLocation` is always valid at the fix site. The value it holds is the static VisualMesh's
authored RelativeLocation, which the DIAG measured correct on every BP (−90 Footman/Archer, −145 Ogre, −90 the
rest). The `bVisualMeshBaseCached` guard is defense-in-depth against a hypothetical future call path — never pins
a garbage ZeroVector.

**Null-guard:** `SkeletalVisualMesh` is already guaranteed non-null at the fix site by the early-return at the
top of the function (`if (bUsingSkeletalVisual || !SkeletalVisualMesh || CardID.IsNone()) return;`,
SummonedUnit.cpp:234). No redundant re-check added; the `SetRelativeLocation` call is inside that guaranteed
region. The optional-component concern the task raised is satisfied.

---

## No-op-for-9 confirmation

The fix writes `VisualMeshBaseRelativeLocation` (= the static VisualMesh's authored RelativeLocation) onto
`SkeletalVisualMesh`. For any unit whose BP already authored `SkeletalVisualMesh.Z == VisualMesh.Z`, this writes
the value the component already holds → byte-identical outcome, no visual change. The DIAG SESSION-2 live-MCP
table measured exactly that for the 9 correct units (static VisualMesh Z and SkeletalVisualMesh Z both −90, or
both −145 for the tall unit) — only Archer/Ogre differ (SkeletalVisualMesh Z=0 vs VisualMesh Z=−90/−145), and
those are the two the fix corrects. I relied on the DIAG's authoritative live measurements rather than re-reading
the editor (editor state is Jonathan's / the compile-pending gate; no MCP sets or PIE during this change). Even
in the impossible case where some unit had intentionally authored a DIFFERENT SkeletalVisualMesh.Z, overriding to
the static VisualMesh's offset is still correct — that offset is the verified grounding value for every unit.

---

## Secondary-lift assessment (Ogre near-hill — ASSESSED, NOT fixed here, per instruction)

DIAG SESSION-1 Rank-2 / the board's WATCH note: a smaller, separate contributor to Ogre's "worse near a hill" is
the spawn path lifting the big Ogre CAPSULE (not the mesh) via nav-projection Z-extents and
`AdjustIfPossibleButAlwaysSpawn`+LiftZ near elevation (SiegePlayerController.cpp:2179-2185; bot/Barracks
NavProjectionExtent.Z=1000 in SiegeBotController/Barracks). This fix removes the dominant, deterministic +145
MESH offset — the primary and by far the larger of the two.

**My assessment:** worth a follow-up ONLY if it still reads badly at W1 playtest — do not pre-empt it. Rationale:
(a) it's a capsule-position issue, orthogonal to this mesh-component fix, so bundling it here would violate the
"keep the fix tight" instruction and widen the branch touched-files/merge gate; (b) the player single-unit path
(the observed symptom) places at the cursor's Visibility `Hit.ImpactPoint` verbatim, which is a true ground hit —
it does NOT go through the ±1000 nav-projection snap-up, so the residual mainly affects bot/Barracks open-field
spawns and is a much smaller magnitude than the +145 this fix removes. If W1 shows a residual Ogre lift near
hills, the clean follow-up is a gameplay-programmer task: either shrink `NavProjectionExtent.Z` (1000→~90) on
SiegeBotController + Barracks to match the player's tight Z=50, OR add a downward `ECC_WorldStatic` ground
re-trace to set the final spawn Z after projection (DIAG Options D/E). Recommend leaving it as the board's WATCH
item, not registering a task yet.

---

## Lane / divergence note

- `SummonedUnit.cpp` AND `SummonedUnit.h` are **byte-identical between `main` and `m7.6-arena10x`**
  (`git diff --stat main...HEAD` empty; 0 diff lines) and had no prior working-tree modifications. This is a
  clean lane: the fix applies identically to both, no branch divergence to reconcile. It JOINS the branch's
  touched-files set for the merge gate (as the board's `names` field notes) and rides the TASK-258 W1-build commit.
- parallel-safe coordination: TASK-259 is the only in-flight SummonedUnit.cpp toucher (TASK-254 is MinerUnit.cpp,
  disjoint). No ordering conflict.

---

## What QA should scrutinize
1. The `bVisualMeshBaseCached` guard + the single-call-path ordering argument above (the only correctness-critical
   subtlety). Confirm no other caller of `ResolveSkeletalVisual`/`LoadStatsAndStart` bypasses the BeginPlay cache.
2. No-op-for-9: confirm the fix writes the same value the correct units already hold (relies on the DIAG table).
3. Include/shadow scans: no new includes needed (`SetRelativeLocation` is on USceneComponent, already available;
   `USkeletalMeshComponent` complete type already included at the top of the TU). No new locals shadow anything.
4. Static-fallback path untouched: a unit with no `SK_<CardID>` still early-returns before the swap (line 249) and
   the new code, keeping byte-for-byte the static-mesh behavior.

## Files touched
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (ResolveSkeletalVisual — the fix)
- `.claude/pipeline/TASKBOARD.md` (TASK-259 status → ready-for-qa)
- `.claude/pipeline/handoffs/TASK-259-programmer.md` (this note)

## Assets referenced
None new. Reads existing `/Game/Characters/SK_<CardID>` (unchanged). No BP, no SK asset, no material touched.
