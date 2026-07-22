# TASK-251 — Hill size variance: DA scale range + nav-Z cap — artist handoff

**Agent:** art-director · **Date:** 2026-07-22 overnight · **Status:** ready-for-integration (same verify runs as TASK-249)

## DA values set (`/Game/Data/DA_BattlefieldScatter`, Hill layer — SAVED)

| Field | Before | After |
|---|---|---|
| `ScaleRange` | 0.9 – 1.3 | **0.4 – 2.5** |
| `InstanceCount` | 6 | **8** |

Everything else on the Hill layer unchanged (WholeField bias, MinSpacing 2000, bBlocking, bAllowOnHills=false — hills never on hills). The radius-aware MinSpacing law (`MinSpacing+Ri+Rj`) auto-scales separation for giants — no spacing edit needed.

## Nav-Z cap math (the recorded constraint)

Measured donor heights (StaticMeshTools bounds, local Z): SM_Hill_01 = **250.0**, SM_Hill_02 = **400.0** (tallest), SM_Hill_03 = **350.0** uu.

- NavMeshBoundsVolume Z = ±1,200 (CONVENTIONS W1-PREP law).
- Absolute ceiling: 1,200 / 400 = **3.0×** (crown touches the volume top — unusable).
- Adopted cap: **2.5×** → tallest crown = **1,000 uu**, leaving a **200 uu** margin under the volume for navmesh cell height + agent headroom.
- The manager-ruled ≈0.4–2.5 target lands exactly at the margin-safe cap → **no nav-volume bump, no nav rebuild owed.**
- Climbability: uniform scale, angles scale-invariant — the ≤30° face law holds at every size (verified visually: giants and minis show identical slope character).

## Count reasoning (giants rare, spread visible)

`FScatterLayer` has a single uniform ScaleRange (no bands field), so rarity is driven by count + draw luck: at 8 hills/seed, expected ~1–2 instances above 2.1× — giants read as landmarks, not a mountain range, while 8 samples make the small-mid-large spread visible every seed. 6→8 is the "modest bump" the spec allows; corridor/keep-clear laws police placement as before.

## Verification (shared runs with TASK-249 — seeds 471742529 / 53151321 / 1677445121)

- **Hill fill 8/8 on all three seeds** — no fill collapse from giant footprints + edge clamp + keep-clear (the QA NIT-1 watch): every other layer also 100%.
- **Traversability CONFIRMED (Blue→Red path, 0 culls) on all three seeds** with giants present — the castle-to-castle guarantee holds at 2.5×.
- **Size spread reads dramatic:** seed-2 wide shot has a 0.4-class bump, mid hills, and 2.5-class giants in one frame; closeup pair `after_seed2_bighill_closeup.png` (giant) vs `after_seed2_smallhill.png` (mini). Seed-1 overhead shows the spread with props on flanks.
- Elevated prop placement follows scale automatically (TASK-250 traces live instance bodies) — grass/trees observed on giant flanks and crowns.
- Residual: direct navmesh spot-check on a 2.5× crown not performed (no nav-debug MCP surface) — the 0-cull traversability pass on every seed is the indirect confirmation; Jonathan's W1 climb is the human check.

## Captures

Same folder as TASK-249: `Tools\ArtPipeline\Cache\HillGrass\w1prep\` — see `after_seed2_overhead.png` (spread), `after_seed2_bighill_closeup.png`, `after_seed2_smallhill.png`, `after_seed1_overhead.png`.

## Integration

Rides the same `Content/Data/DA_BattlefieldScatter.uasset` save as TASK-249's wire (one file, both tasks) — branch-lane fold per the board.
