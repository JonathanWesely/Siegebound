# QA Report — TASK-250 (Scatter-on-hills + OverrideMaterial, branch C++)

Verdict: **PASS**

Reviewer: qa-reviewer, 2026-07-22. Files: `ScatterConfig.h`, `BattlefieldScatter.{h,cpp}` on the
`m7.6-arena10x` checkout. Reviewed against the W1-PREP board spec, CONVENTIONS "W1-PREP additions",
handoffs/TASK-250.md, and the installed UE 5.8 engine source (API claim re-verified). Compile
rides the pre-W1 bounce per spec. Board untouched.

Blockers: 0 · Warnings: 0 · Nits: 3

## Diagnosis — independently VERIFIED from the code (the board's mechanism correctly reversed)

- Cause 1 confirmed at BattlefieldScatter.cpp:486-493: the real-geometry blocker profile sets
  `SetCollisionResponseToAllChannels(ECR_Ignore)` + blocks only Pawn/Visibility/Camera — hills
  IGNORE `ECC_WorldStatic` by the scatter-channel law (the in-code comment even states why), so
  `GroundZAt`'s WorldStatic trace (:730) passes through every hill.
- Cause 2 confirmed at :729: `FCollisionQueryParams(..., /*bTraceComplex=*/false, this)` — the
  scatter actor is ignored wholesale, and every hill HISM is its component.
- The board's assumed mechanism is genuinely wrong: `PlacedPoints` (:246) is a LOCAL per-layer
  array — MinSpacing/footprint rejection is same-layer only; hills exert no cross-layer
  exclusion. Pre-fix, hill-XY candidates grounded at floor Z inside the mound. Diagnosis is
  evidence-solid and correctly recorded.

## 1. Two-pass GenerateScatter — VERIFIED

- **Split correctness:** pass 1 = every `bAllowOnHills=false` layer (:149-155), pass 2 = the
  opted-in layers (:156-162); `HillSurfaceComponents.Reset()` precedes pass 1 (:148).
- **Circular case SAFE:** a (misconfigured) hills layer with `bAllowOnHills=true` moves to pass 2
  AND is excluded from surface registration (`bHillSurfaceProvider` requires `!bAllowOnHills`,
  :226) — it can never trace against itself; no recursion, no self-stacking. Pass-2 layers never
  register, so two opted-in blockers can't stack on each other either. Degrades to
  hills-on-pass-1-rocks at worst.
- **All-default DA byte-identical — the regression trap CLEARED:** with no layer opted in, pass 1
  visits layers in exact config order and pass 2 matches nothing → layer processing order
  identical to the old single loop. Per-attempt Stream draws are unchanged (X :255,
  SampleBiasedY :256, mesh :268, scale :274, yaw :321 — same order); `ResolveHillAwareGroundZ`
  draws NO randoms and is short-circuited by `Layer.bAllowOnHills &&` (:330/:374); the
  OverrideMaterial branch short-circuits on `IsNull()` (:449); traces never touch the stream.
  RNG consumption is genuinely byte-identical for existing seeds.
- **Play-Again/re-seed intact:** the registry is reset + rebuilt every generate; comp reuse via
  `ResolveComponentForMesh` + `AddUnique` keeps it consistent across re-seeds; `ClearScatter`
  untouched; the opted-in seed-order change is correctly documented as the TASK-140-precedent
  intentional class.

## 2. ResolveHillAwareGroundZ — VERIFIED

- **Engine API claim RE-VERIFIED against installed 5.8 source:**
  `InstancedStaticMeshComponent.h:564` declares the `LineTraceComponent` override (HISM inherits
  it), and the implementation (`InstancedStaticMesh.cpp:5442-5444`) traces
  `InstancePhysicsBodies->LineTrace(...)` — per-instance bodies, direct body query (which is
  exactly why it can see WorldStatic-ignoring hills without touching any channel contract).
  Signature/args at :772 match. QueryOnly bodies exist (hills block Pawn — physics state is
  created; pass-1 instances get query bodies at AddInstance on the registered comp).
- **Highest-hit-wins correct:** per component a top-down trace's closest hit IS that component's
  highest surface; across components the loop keeps `ImpactPoint.Z > BestZ` (init FloorZ) — max
  over overlapping hills, and at-or-below-floor skirt hits are correctly discarded (:770-777).
- **Slope math correct:** `acos(clamp(Normal.Z, -1.0, 1.0))` in double → degrees (:792);
  downward-facing normals (cave lips) yield >90° and auto-reject; the gate compares against
  `max(MaxSlopeDeg, 0)`. Bonus correctness property: `bTraceComplex=false` means the slope gate
  reads the SIMPLE collision hull — the same geometry units actually climb — so prop placement
  agrees with the walked surface.
- **Over-slope ⇒ outright rejection (no floor fallback):** returns false; the caller `continue`s
  (primary :330-333) or skips just the twin (:374-377). Correct — a floor fallback would re-bury
  the prop, the exact defect being fixed. The header doc states the caller contract explicitly.
- **Mirror-twin independent grounding:** twin re-traces at (−X, Y) with the same resolve; an
  over-slope face skips only the twin, mirroring the keep-clear twin-skip shape. ✓
- **Trace ceiling ample for TASK-251 scaling:** start Z = 50,000 uu (500 m); the CONVENTIONS
  W1-PREP law caps hill crowns under the nav volume Z (±1,200 uu today) — even an uncapped 3×+
  crown sits orders of magnitude below the ceiling.

## 3. Keep-clear / corridor / traversability — laws UNCHANGED, verified

- Field-edge clamp (:291), footprint-inflated keep-clear (:299), radius-aware MinSpacing (:306)
  all run BEFORE the ground resolve (:329) on the 2D candidate — identical for hill-placed
  instances.
- The collision/nav profile branch (:472-502) does not read `bAllowOnHills` — a hill-allowed
  BLOCKING layer keeps the full real-geometry profile (Pawn/Vis/Camera block,
  `bFillCollisionUnderneathForNavmesh`, nav-relevant), so elevated blockers still nav-carve,
  still participate in `ValidateTraversability`, and `CullCorridorBlockers` (|Y|-band,
  Z-agnostic) still reaches them. No new invisible corridor-block vector: the hill itself passed
  its own inflated corridor test, and every prop on it independently passes its own.

## 4. OverrideMaterial — verified; both adjudications resolved

- **Sync-load timing ADJUDICATED ACCEPTABLE:** `LoadSynchronous` at generation (:451/:551) sits
  beside the pre-existing donor-MESH `LoadSynchronous` calls (:200) — the file's established
  loading model, executed at match setup, not mid-play; one material is marginal against the
  mesh set, and Play-Again re-resolves are no-ops once resident. No hitch class is added that
  generation didn't already have.
- **Proxy application NOT wasteful/harmful:** one-time SetMaterial on an invisible component =
  zero render cost, keeps the pair uniform for debug un-hiding — and the CONVENTIONS wording
  ("applied to the layer's HISM + proxy components") REQUIRES it. Law-compliant, not a nit.
- Null-safe both sites: null short-circuit; failed resolve → warn + donor look (visual), silent
  no-op (proxy, documented as already-warned). All-slots via `GetNumMaterials()` with the
  single-slot `HillGround` donor evidence recorded; the shared-mesh first-layer-wins reuse
  behavior is documented as a DA config smell (NIT-3).

## 5. Hygiene — verified

- UPROPERTYs per file style: `bAllowOnHills` (Category Scatter, default false — pre-task
  behavior); `MaxPlacementSlopeDeg` (ClampMin 0 / ClampMax 89, `EditCondition="bAllowOnHills"`,
  default 35 with the ≤30°-law rationale); `OverrideMaterial` TSoftObjectPtr with the null=donor
  doc. `UMaterialInterface` fwd-decl in ScatterConfig.h + `Materials/MaterialInterface.h` include
  in the cpp (:13) — complete-type where needed.
- `HillSurfaceComponents` GC safety: raw-pointer secondary index over comps rooted via the
  `ScatterComponents` UPROPERTY + the actor outer chain — the VisualToProxy precedent, reset per
  generate, torn down with the actor. Sound.
- Shadow scan clean (new locals `bPlaceMirror`/`MirrorGroundZ`/`GroundZ`/loop `Hit` collide with
  nothing reflected or enclosing; the handoff's C4456-59 claim holds on read).

## 6. The six handoff scrutiny points — each addressed

1. **LineTraceComponent:** CONFIRMED — header :564 + impl :5442 per-instance bodies; signature
   and args match; agree with the usage.
2. **Rocks-not-opted-in corner:** AGREE acceptable/organic — grass on ≤35° boulder tops reads
   naturally, blocking laws unchanged, and the slope gate reads the same hull units walk on. A
   name-based hill filter would be brittle; the field-driven definition is right.
3. **Mirror restructure:** CONFIRMED equivalent for the non-hill path — `bPlaceMirror` reproduces
   the old single-`if` gate exactly (GroundZAt still called only after the keep-clear test
   passes; no stream impact since traces draw no randoms).
4. **Aliasing:** CONFIRMED safe — `FloorZ` is a by-value param, copied before `OutZ` (same
   variable at the call site) can be written; `OutZ = FloorZ` first line makes the no-hill path
   correct even so.
5. **Double/float discipline:** CONFIRMED — double clamp literals (:792), one explicit
   `static_cast<float>` on the LWC Z (:775), slope compare promotes safely.
6. **All-default no-behavior-change:** CONFIRMED including the full RNG-stream analysis (see §1)
   — the strongest regression risk is genuinely closed.

## Findings

- [NIT-1] Over-slope rejections consume placement attempts (24/instance): an opted-in layer over
  a heavily-hilled field may under-fill its InstanceCount. Bounded, visible in the per-layer
  "placed X of Y" log line; playtest-observable, no action needed now.
- [NIT-2] The one runtime assumption not provable from source alone: pass-2 component traces
  require pass-1 instance QUERY BODIES to exist same-frame after `AddInstance` (standard ISM
  behavior on a physics-state'd registered comp — expected true). TASK-249's wire step should
  visually confirm props actually sit ON hill surfaces on the first re-seeded scatter; if hills
  ever read empty again, this is the first thing to check.
- [NIT-3] Mesh shared across layers keeps the first layer's override (one-HISM-per-mesh law) —
  documented DA config smell; keep hill donors exclusive to the hills layer (they are today).
- [Observation] The bonus finding (SM_Hill donors already carry `MI_BattlefieldGround` on their
  single `HillGround` slot — the bare look is planar smear, not stone) usefully corrects the
  board's stale note and sharpens TASK-249's target. Good diagnostic work.

## For downstream

- **build-master (pre-W1 bounce):** compile is the only outstanding gate; one new engine include,
  no new modules.
- **TASK-249:** wire `OverrideMaterial=M_HillGrass` on the HILLS layer; donor slot layout
  recorded (single slot `HillGround`); verify props-on-hills visually (NIT-2).
- **TASK-251:** elevated placement follows any hill scale automatically (live instance bodies);
  the CONVENTIONS crown-under-nav-Z cap is the binding constraint, not the trace ceiling.
