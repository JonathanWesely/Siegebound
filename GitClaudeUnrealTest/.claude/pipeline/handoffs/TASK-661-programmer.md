# TASK-661 — [F1-2b] ⚙️ ACastle furnishing anchors for the F1 readability set (gameplay-programmer handoff)

**Status: ready-for-qa (TASK-658 reviews; orchestrator flips the board).** Date: 2026-08-27.
**Files touched: `Source/GitClaudeUnrealTest/Siegebound/Castle.h` + `Castle.cpp` — NOTHING ELSE.** No compile run (TASK-659 owns the one — QUIET-MODULE), no editor/MCP, no git, no TASKBOARD edit, `BattlefieldScatter.cpp` untouched (the :1313 rider stays PARKED per F1-R4 as adjudicated).
Laws honoured: F1-R3 (no seal touched — every number below places VISUALS only) · GH-R9 (zero collision/nav delta, enforced code-side) · M8 (declaration §4) · the ATorch null-safe soft-resolve law · the RELAYED-DIAGNOSIS law (every anchor recomputed from `handoffs/TASK-656-buildmaster.md` §1/§2, not from dispatch transcriptions) · the trailing-defaulted-parameter law (no new default anywhere — §6) · the compile-trap laws (audited §7).

---

## 1. THE MECHANISM (what changed, where)

The torch/commander furnishing precedent, mirrored shape-for-shape:

- **`Castle.h`** — three `EditDefaultsOnly` castle-LOCAL anchor arrays (`GateBannerAnchors`, `TramplePathAnchors`, `ToeRockAnchors`), four soft mesh refs (`GateBannerMeshAsset`, `TramplePathMeshAsset`, `ToeRockMeshAsset01/02`), a `Transient` `UPROPERTY` tracking array (`SpawnedDiscoverabilityMeshes`), three one-shot log guards, and two private members: `SpawnDiscoverabilityFurnishings()` + `SpawnDiscoverabilityMesh(UStaticMesh*, const FTransform&, bool bCastShadow)`.
- **`Castle.cpp`** — an F1 constants block in the anonymous namespace (every 656 measurement as a NAMED constant with its derivation beside it — the GH-R7 house style), anchor construction in `ACastle::ACastle()` (arithmetic over the constants, never bare literals), the sibling spawn called from inside `SpawnCastleFurnishings()`, teardown appended to `DestroyCastleFurnishings()`, and the two new function definitions.
- **Lifecycle = the torch lifecycle with ZERO new call sites:** the sibling is invoked from inside `SpawnCastleFurnishings()`, so it automatically rides BeginPlay, BOTH Play-Again edges (`ApplyDestroyedState` true→destroy / false→respawn), and the EndPlay belt. Idempotency comes from the existing clear-first (`SpawnCastleFurnishings` opens with `DestroyCastleFurnishings`).
- **Components, not actors** (declared deviation D3, §5): the spawns are `UStaticMeshComponent`s attached to `CastleMesh` with the anchor AS the relative transform. Castle-local ⇒ symmetric on BOTH castles by construction (Red's mouth banners signpost the hero's attack approach, per the ruling). Components live and die with the actor, so the WR-§4 orphan hazard cannot arise structurally — teardown still runs on the same edges as the torches so a fallen castle sheds its signage.
- The existing torch furnishing summary log line is **byte-identical** (it is TASK-569's grep surface); the F1 set logs its OWN one-line summary per pass.

## 2. ANCHOR TABLES (castle-mesh-local; FRotator is (Pitch, Yaw, Roll) — only yaw is ever non-zero)

Source of every figure: `handoffs/TASK-656-buildmaster.md` §1/§2. Frame: origin GROUND-CENTRE (WR-§0), z 0 = arena grass, gate mouth on local −Y, +X = the d1 face a straight run from the Blue spawn hits.

### 2a. GateBannerAnchors (2) — ruling item (a), the mouth markers

| # | x | y | z | yaw | derivation |
|---|---|---|---|---|---|
| 0 | −1400 | −3675 | 0 | −90 | x = −(1470 − 70): channel half-width (656: mouth x −1470..+1470) minus the flank-knoll inset; y = −3650 − 25: the step line (656: step band y −3650..−3600) minus the onto-flat-approach offset; yaw −90 ⇒ +X (the banner face, 657 pivot contract) due SOUTH at the field |
| 1 | +1400 | −3675 | 0 | −90 | mirror of #0 |

### 2b. TramplePathAnchors (13, in walk order) — ruling item (b), spawn line → toe-ring wrap → the mouth

z = 2 + 0.25 × chain index (the ruling's "+2 over measured support" plus a monotonic anti-coplanar stagger — no two segments coplanar whatever segment length 657 ships; max lift 5.0 at index 12). Yaw aims +X along the direction of travel (tileable-along-X contract).

| chain i | x | y | z | yaw | leg |
|---|---|---|---|---|---|
| 0 | 3699.5 | 0 | 2.00 | −90 | Leg 1 SOUTH, the hero's stop lane: x = 3657.5 (seal face, 656-measured live==manifest) + 42 (hero capsule radius) = 3699.5 — 656 measured the VID-001 stop centre at exactly this x; first segment starts AT the measured stop point (+3699.5, 0), under the hero's own feet |
| 1 | 3699.5 | −600 | 2.25 | −90 | y = −600 × i (pitch 600) |
| 2 | 3699.5 | −1200 | 2.50 | −90 | |
| 3 | 3699.5 | −1800 | 2.75 | −90 | |
| 4 | 3699.5 | −2400 | 3.00 | −90 | |
| 5 | 3699.5 | −3000 | 3.25 | −90 | |
| 6 | 3699.5 | −3700 | 3.50 | −135 | SE corner: on the wrap lane y −3700 (SOUTH of the 656 measured-clear bound −3692.5, and the proven 16/16 route battery's own southmost station line); yaw −135 = the diagonal between the two legs' headings |
| 7 | 3099.5 | −3700 | 3.75 | 180 | Leg 2 WEST along the wrap: x = 3699.5 − 600 × (k+1) |
| 8 | 2499.5 | −3700 | 4.00 | 180 | |
| 9 | 1899.5 | −3700 | 4.25 | 180 | |
| 10 | 1299.5 | −3700 | 4.50 | 180 | |
| 11 | 699.5 | −3700 | 4.75 | 180 | |
| 12 | 0 | −3700 | 5.00 | +90 | TURN-IN: x 0 = the channel centre lane 656 walked 16/16 EMPTY; yaw +90 ⇒ +X due north through the mouth — the chain's last segment is the arrow at the ramp foot; any overrun past y −3650 vanishes under/into the step mass |

Wrap-fence check: every anchor with i ≥ 6 sits at y = −3700 ≤ −3692.5 ✓. Leg-1 anchors run OUTSIDE the seal face (x 3699.5 > 3657.5) on the grass the hero himself walked.

### 2c. ToeRockAnchors (10) — ruling item (c), the seal-line picket

x = 3650 (the 656 rim crest, measured x ≈ 3645..3655), z = 95 (the measured rim TOP — deviation D2, §5), y = −1000 + 200 × i (the full measured rim span y −1000..+800 at the ruling's ≈200 spacing, ends inclusive ⇒ count = 1800 ÷ 200 + 1 = 10). Yaw = fmod(137.5 × i, 360) — golden-angle jitter, deterministic (baked into the CDO array once ⇒ identical on both castles and both machines).

| i | y | yaw | mesh (parity pick) |
|---|---|---|---|
| 0 | −1000 | 0.0 | ToeRock01 |
| 1 | −800 | 137.5 | ToeRock02 (falls back to 01 if 02 unresolved) |
| 2 | −600 | 275.0 | 01 |
| 3 | −400 | 52.5 | 02→01 |
| 4 | −200 | 190.0 | 01 |
| 5 | 0 | 327.5 | 02→01 |
| 6 | +200 | 105.0 | 01 |
| 7 | +400 | 242.5 | 02→01 |
| 8 | +600 | 20.0 | 01 |
| 9 | +800 | 157.5 | 02→01 |

## 3. SOFT-RESOLVE / NULL-SAFETY POSTURE (the ATorch two-null-paths law, per family)

Pinned paths, exactly the board's names block (MIs deliberately NOT referenced code-side — the meshes carry their materials from 657's import; the 661 names block soft-references SM_ paths only):
`/Game/Meshes/SM_Castle_GateBanner` · `/Game/Meshes/SM_Castle_TramplePath` · `/Game/Meshes/SM_Castle_ToeRock01` · `/Game/Meshes/SM_Castle_ToeRock02`

- **CLEARED (`IsNull`)** ⇒ silent designer opt-out — family absent, no log.
- **SET BUT UNRESOLVABLE** ⇒ family skipped with ONE log line at `Log` verbosity (one-shot guard bool per family, never per Play Again; `Log` not `Warning` — the ResolveTorchClass reasoning: this is the EXPECTED state until TASK-657 lands, and 657 may land after this code by the ruling's design). **No crash on any missing asset, ever.**
- **ToeRock02 is OPTIONAL by the 657 names block:** its absence alone is never logged — odd anchors quietly fall back to 01 (and vice versa); the family logs only when at least one of the pair is SET and NEITHER resolves. The picket is never thinned by a missing optional.
- No C++ fallback mesh exists on purpose (unlike the torch CLASS fallback): there is no such thing as a placeholder banner worth shipping.

## 4. COLLISION ENFORCEMENT + M8 DECLARATION

**Enforcement is a SINGLE site every spawn routes through** — `ACastle::SpawnDiscoverabilityMesh` (`Castle.cpp:1032` region):
```
SetCollisionProfileName(NoCollision_ProfileName);   // profile first
SetCollisionEnabled(ECollisionEnabled::NoCollision); // the ruling's named call, verbatim
SetGenerateOverlapEvents(false);
SetCanEverAffectNavigation(false);                   // navmesh record byte-identical too
```
Belt (657's `ucx: null` assets) AND braces (these lines hold against even a mis-authored import). The three family call sites — `Castle.cpp:916` (banners), `:940` (path), `:973` (rocks) — are the ONLY callers; grep `SpawnDiscoverabilityMesh(` = 3 call sites + 1 definition + 1 declaration.

**M8 declaration (the ruling's expected posture, confirmed true at the artifact):** no replicated property, no new replicated class, no new relevancy tier — cosmetic furnishing on the existing spawn path. The components are created LOCALLY on every machine by the deliberately-unguarded `SpawnCastleFurnishings` lane (the Tier-C local-projection posture the class doc declares for the torches, verbatim); no `UPROPERTY(Replicated)`, no `SetIsReplicated`, no new actor class exists at all. Both machines build identical sets from the same CDO defaults by construction. The predicted false-condition ("if that turns out false, STOP and declare") did not arise.

## 5. DECLARED DEVIATIONS (SC-§15)

- **D1 — banner anchors (±1400, −3675) vs the ruling's literal "(±1470, ≈ −3650)".** ±1470 IS the channel edge where the flank knolls begin (245–468 uu, 656 §1) — a pole base AT the line risks standing in knoll toe; −3650 is the step band's own start. 70 uu in + 25 uu south puts each pivot on flat approach ground the 656 full-width mouth-line down-traces (x −1500..+1500) measured clear. Documented at the constants (`GateBannerEdgeInsetX`).
- **D2 — toe-rock z = 95, not 0.** The ruling's "x ≈ 3650" line is the rim CREST whose local ground is the measured rim top 95–96; a ground-contact pivot at z 0 there buries a rock ~95 uu deep. z 95 stands each rock ON the lip the hero jumped at — silhouette exactly where the refusal happens; the rim's measured 92–101 spread sinks/floats a base ≤ 6 uu (natural for fieldstone; EditDefaultsOnly retunable).
- **D3 — `UStaticMeshComponent`s, not spawned actors.** The ruling's mechanism ("the ACastle-spawned-furnishing pattern: C++ castle-local anchors, zero level edit") and its own enforcement wording ("on every spawned **component**") are both satisfied; components additionally make the WR-§4 orphan hazard structurally impossible and need no new actor class (the fence is Castle.{h,cpp} only). Anchor semantics, Tier-C posture, and teardown edges are byte-equivalent to the torch pattern.
- **D4 — dispatch-transcription discrepancy, for the record (RELAYED-DIAGNOSIS law):** the dispatch relayed "the spawn at castle-local ≈ (+3957, 0)"; 656 §1 pins PlayerStart world (−23800, 0, 100) ⇒ castle-local (+1200, 0). Neither figure is load-bearing here: the chain starts at the 656-MEASURED stop lane (+3699.5, 0) — which is also the ruling's own named start "local ≈ (+3699, 0)". No anchor derives from the +3957 figure.
- **D5 — path shadow off.** `bCastShadow` false for the ribbon only (a 2-uu flat ribbon's shadow is acne fuel under the low moonlight); banners and rocks cast (their shadow IS silhouette). Explicit at every call site — no default parameter.

## 6. QA SCRUTINY LIST (TASK-658)

1. **(b2) anchor recompute** against 656 §1/§2 — the three tables in §2 above carry every derivation; check the arithmetic, especially `HeroStopLaneX = 3657.5 + 42` and the wrap fence (all wrap-leg y = −3700 ≤ −3692.5).
2. **FRotator argument order** — (Pitch, Yaw, Roll) at every `FTransform(FRotator(0.f, <yaw>, 0.f), …)`; the torch anchors are the in-file precedent.
3. **Collision enforcement** — single helper site (§4), three enumerated callers, nothing spawns a mesh around the helper. Also `SetCanEverAffectNavigation(false)` (zero NAV delta, not just zero collision).
4. **No rider** — `BattlefieldScatter.cpp` untouched; diff is Castle.{h,cpp} only (F1-R4 parked, F1-R3 clean: no seal/hull/manifest figure written).
5. **Trailing-defaulted-parameter law** — `SpawnDiscoverabilityMesh(..., bool bCastShadow)` has NO default; call sites 916/940/973 all pass it explicitly.
6. **Null-safety matrix** — per-family cleared vs unresolvable vs resolved; the ToeRock either-of-two logic (`PickedRockMesh` can never be null inside its branch); one-shot guards never per-Play-Again.
7. **Lifecycle** — sibling called INSIDE `SpawnCastleFurnishings` (after the commander block, before the torch summary log, which is byte-identical); teardown appended to `DestroyCastleFurnishings`; no new call sites anywhere else (grep `SpawnDiscoverabilityFurnishings` = 1 call + 1 def + 1 decl).
8. **Mobility/attach** — Movable child under the Static level-placed root (the legal direction); `SetupAttachment` before `RegisterComponent`; `NewObject` with auto-unique name (Play-Again name-collision hazard avoided).
9. **Editor-world cleanliness** — components are Transient and created only on the BeginPlay/ApplyDestroyedState lanes (never OnConstruction), so no editor-world or `L_Arena` dirt vector exists.

## 7. COMPILE-TRAP AUDIT (self-run; TASK-659 owns the actual compile)

- No `*/` inside any added comment text (grep-verified over the diff; the one arithmetic that needed a division sign in a doc comment uses `÷`).
- Every `UE_LOG` format string is a `TEXT()` literal.
- No shadowed locals (`SouthSegIndex`/`WestSegIndex`/`RockIndex`/`RockAnchorIndex`/`TrampleChainIndex` all distinct in scope).
- Complete-type includes: everything the new code touches (`UStaticMeshComponent`, `UStaticMesh`, `UCollisionProfile`, `FSoftObjectPath`) was already included in Castle.cpp; the header uses existing forward decls only.
- `constexpr` arithmetic exact: `ToeRockCount = (800 − (−1000)) ÷ 200 + 1 = 10` (1800/200 is exact in float).

## 8. OPEN QUESTIONS FOR QA / DOWNSTREAM

- **Q1 (658):** is D1's ±1400/−3675 inset acceptable as an SC-§15 departure, or does the ruling's "±1470 line" bind literally? The inset is defended on 656's measured flank-knoll geometry; moving back is a two-line defaults change.
- **Q2 (659, live):** the ribbon/rock meshes' real dimensions arrive with 657 — the 600 pitch and the crest-top rock line were authored to be length/size-robust (§2b note, D2), but the before/after capture pair at 656's M1/M2 transforms is where the read is judged; re-pose is EditDefaultsOnly if Jonathan's eye objects (no recompile).
- **Q3 (657 fyi, via orchestrator):** banner readability from the spawn approach is asset height's job (the pair is ~4.4–6.3 k uu from the stop lane); nothing code-side constrains pole height.
