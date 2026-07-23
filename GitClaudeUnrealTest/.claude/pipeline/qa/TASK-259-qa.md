# QA Report — TASK-259 (floating-units fix: Archer/Ogre skeletal Z-offset)

Verdict: **PASS**

Reviewer: qa-reviewer, 2026-07-23. W1 hard blocker on the TASK-258 build commit. One-file C++
change: `SummonedUnit.cpp::ResolveSkeletalVisual` (guarded `SetRelativeLocation(VisualMeshBase
RelativeLocation)` after `SetSkeletalMeshAsset`). Reviewed against DIAG-floating-units.md
(SESSION-2 measured table), handoffs/TASK-259-programmer.md, and the code on the branch. No compile
(rides TASK-258). Board untouched.

Blockers: 0 · Warnings: 0 · Nits: 1

## 1. Cache-ordering — VERIFIED (the load-bearing subtlety)

- The cache write is `BeginPlay:154-158`: `if (VisualMesh && !bVisualMeshBaseCached) { … = VisualMesh
  ->GetRelativeLocation(); bVisualMeshBaseCached = true; }`. `VisualMesh` is a constructor default
  subobject → never null → the cache is always populated by the end of BeginPlay.
- `ResolveSkeletalVisual` has **exactly one caller**, `LoadStatsAndStart` (call at :970;
  grep-confirmed single site). `LoadStatsAndStart` has **exactly two callers**:
  - `BeginPlay:182` — same function, sequentially AFTER the :154-158 cache. ✓ (covers the deferred
    `SpawnActorDeferred → InitUnit(pre-BeginPlay) → FinishSpawning → BeginPlay → cache →
    LoadStatsAndStart` path).
  - `InitUnit:457` — gated by `if (HasActorBegunPlay())` at :455, so BeginPlay (and its cache) has
    already run. ✓ (covers the plain `SpawnActor + InitUnit` path; the pre-BeginPlay InitUnit branch
    does NOT call LoadStatsAndStart — it defers to BeginPlay).
  Independently confirmed: no third caller of either function exists in the TU.
- **`bVisualMeshBaseCached` correctly gates the write** — if (hypothetically) the cache were ever
  unpopulated, the fix is skipped and the component keeps its pre-fix Z (no regression, never pins a
  garbage ZeroVector). Defense-in-depth, not load-bearing given the paths above. The cache holds
  `VisualMesh->GetRelativeLocation()`, which is the static VisualMesh's BP-authored offset — the same
  cache the TASK-020 lunge already consumes (:1819/1832/1861), so the value is proven-correct in an
  existing shipping path.

## 2. No-op for the 9 — VERIFIED, and it CANNOT shift a correct unit

- The fix writes the static VisualMesh's authored offset onto SkeletalVisualMesh. **Structural
  argument (stronger than the table):** every rigged SK is imported feet-at-pivot (CONVENTIONS
  feet-center origin law; DIAG bounds-measured Footman 89.85/89.83, Archer 90.00/89.95, Ogre
  143.93/144.09 — all mesh-bottom ≈ Z=0), and the static SM is likewise feet-at-pivot. Both meshes
  therefore require the SAME grounding offset = −CapsuleHalfHeight = the static VisualMesh.Z. So for
  ANY correctly-grounded rigged unit, SkeletalVisualMesh.Z already equals VisualMesh.Z → the write is
  the value the component already holds → genuinely byte-identical.
- The value SOURCE distinction the dispatch flagged (static base vs the BP-authored skeletal value)
  is real — they are two independently-authored component transforms — but the DIAG SESSION-2 live
  MCP reads measured them EQUAL for the 9 (−90/−90; −145 is the Ogre, one of the two fixed), so the
  no-op is confirmed empirically as well as structurally.
- **Cannot shift a correct unit:** the only way overriding to VisualMesh.Z could break a correct unit
  is if that unit were correctly grounded with SkeletalVisualMesh.Z ≠ VisualMesh.Z — which requires a
  non-feet-at-pivot SK, contradicting the measured/law invariant above. Even the handoff's
  "impossible different-authored-Z" case resolves correctly: VisualMesh.Z is the verified grounding
  offset for every unit, so the override lands them right regardless. A same-value
  `SetRelativeLocation` has no dependent-component side effect (HPBarWidget attaches to the capsule,
  not the mesh; UE early-outs an unchanged transform).

## 3. Correctness for Archer / Ogre — VERIFIED

Archer: SkeletalVisualMesh 0 → −90 (its VisualMesh.Z, = −CapsuleHalfHeight 90). Ogre: 0 → −145
(VisualMesh.Z, = −HalfHeight 145). With SK feet at pivot Z=0 and the component pinned at
−HalfHeight below capsule center, feet land at capsule bottom = grounded. Magnitudes match the
measured +90/+145 float exactly. Correct.

## 4. Scope + null-safety — VERIFIED

- Only `ResolveSkeletalVisual` changed; no BP, SK asset, static-mesh, spawn, or movement path
  touched. The static-fallback path (no `SK_<CardID>`) early-returns at :249 BEFORE the fix — a
  un-rigged unit is byte-for-byte unchanged.
- `SkeletalVisualMesh` non-null is guaranteed by the top-of-function early-return
  (`if (bUsingSkeletalVisual || !SkeletalVisualMesh || CardID.IsNone()) return;`, :234); the fix
  sits inside that guaranteed-non-null region — no redundant re-check, correct.
- **Secondary Ogre-near-hill lift correctly left as a WATCH, not half-fixed:** the handoff's
  assessment is sound — that residual is a CAPSULE-position issue (nav-projection Z-extent 1000 +
  AdjustIfPossibleButAlwaysSpawn/LiftZ), orthogonal to this mesh-component offset; the player
  single-unit path (the observed symptom) spawns at the cursor's verbatim Visibility `Hit.ImpactPoint`
  and never goes through the ±1000 snap-up, so it mainly affects bot/Barracks open-field spawns.
  Bundling it would widen the branch touched-files/merge gate for a smaller, non-symptom contributor.
  Leaving it as the board WATCH (fix later ONLY if W1 still reads bad; DIAG Options D/E named) is the
  correct tight-fix discipline.

## 5. Lane note — consistent by inspection

`SummonedUnit.cpp` is a main-lane file (the SkeletalVisualMesh machinery is M7/TASK-159, on main); it
appears in NO M7.6 branch task's file set (the branch touched ScatterConfig / BattlefieldScatter /
SiegeBotController / GoldNode / MinerUnit). So the handoff's "main == branch, 0 diff, clean lane,
joins the merge-gate touched set" claim is fully consistent with everything reviewed. (git proper is
not a QA tool here; the claim is corroborated structurally, not contradicted.)

## Findings

- [NIT] The no-op guarantee for the 8 rigged units beyond the bounds-measured Footman/Archer/Ogre
  rests on the DIAG SESSION-2 table's live-MCP value-equality reads (−90/−90) plus the feet-at-pivot
  pipeline law, not a per-unit re-measure this task (correctly so — the editor was Jonathan's and
  compile-pending). TASK-258's compile + PIE will visually confirm all rigged units ground correctly;
  worth a one-line spot-check across the roster in that pass, per the DIAG's own "recommend a quick
  sweep of the other 8."

## Notes for TASK-258 / build-master

- Compile the fix with the mines batch; PIE verify: Archer + Ogre spawn grounded (feet at ground),
  the other rigged units unchanged, un-rigged units unchanged. The float is deterministic per the
  DIAG, so a single spawn of each proves it.
- Carry the Ogre-near-hill residual on the W1 WATCH list, not as a task, unless it still reads badly.
