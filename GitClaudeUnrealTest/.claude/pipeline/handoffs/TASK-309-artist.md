# TASK-309 — Building/tower colour audit (art-director)

**Status: ready-for-integration** (audit only — NOTHING was rebuilt, reimported or mutated).
Date: 2026-07-26. Scope: `ArrowTower, Wall, BombTower, BallistaTower, Barracks, DeepMine, CrystalTower, GoldNode, Castle`.

## VERDICT — Jonathan is RIGHT for 8 of 9. Castle is the exception, and it is severe.

The buildings do **not** broadly share the fleet's wash-out defect. Exactly one mesh fails:
**`SM_Castle` — the most important building in the game — is the worst-looking asset in the project**, worse
than any of the 11 fleet units were *before* the remaster.

## FLAGGED LIST

| # | Building | Verdict | mean linear albedo (raw / UV-normalised) | one-line reason |
|---|---|---|---|---|
| 1 | **Castle** | **REBUILD** 🚨 | **0.0078 / 0.0653** | Never de-lit at all (pre-TASK-193 bake, no `albedo_delight` block); renders as a near-black silhouette under real `L_Arena` sun vs a warm tan sandstone concept. |
| 2 | ArrowTower | OK | 0.0408 / 0.1996 | Grey stone reads faithfully; chroma retention 0.31x sits at the accepted-unit floor (Cleric 0.28x). |
| 3 | Wall | OK ✅ | 0.0810 / 0.2451 | Best of the set — chroma retention **1.02x** (no loss at all) and albedo at the Footman baseline. |
| 4 | BombTower | OK | 0.0523 / 0.2314 | Absolute chroma is the lowest of the nine, but the *concept* is also the lowest-chroma of the nine — retention 0.51x is 3rd best. Grey is correct here. |
| 5 | BallistaTower | OK ✅ | 0.0314 / 0.1531 | Warm brown timber reads strongly (highest in-engine saturation, 0.537); low albedo is genuine dark timber, the "Knight is correctly dark" case. |
| 6 | Barracks | OK | 0.0576 / 0.2724 | Above the Footman baseline; warm timber + gold banner read. Roof is the TeamRegion, so the concept's terracotta was always going to be overpainted. |
| 7 | DeepMine | OK (watch) | 0.0368 / 0.1596 | Faithful dark-rock arch, warm timber reads; retention 0.35x = exactly Knight's accepted value. 2nd-dimmest — watch it in a real match. |
| 8 | CrystalTower | OK ✅ | *(n/a — textures unused)* | Ships on `MI_CrystalGlow`, not its baked PBR. **Cyan glow reads** (EmissiveColor `0.05/0.6/1.0`, strength 12). |
| 9 | GoldNode | OK for this audit ⚠️ | *(n/a — textures unused)* | Ships on `M_GoldGlow`, not its baked PBR. **Warm-yellow emissive reads — it OVER-reads:** 86.3% of the mesh is blown past luma 0.85, form destroyed. Separate issue, see below. |

**Rebuild count: 1 (Castle).** Everything else is OK.

## Method — how the numbers were produced (measure, don't eyeball)

Primary metric = the fleet batch's own: mean linear albedo of the shipped `T_<Building>_D.png`, computed with the
exact formula from `Tools/ArtPipeline/refine_trellis_glb.py` (`linear = srgb_to_linear(rgb); metric = linear.mean()`).
**Validated:** the script reproduces every recorded `albedo_delight.mean_linear_after` to 4 decimal places
(Knight 0.1639 = 0.1639, Cleric 0.3614 = 0.3614, Footman 0.1639 = 0.1639). Baseline = **Footman 0.1639**; rebuilt fleet 0.14–0.36.

⚠️ **Confound the orchestrator must know:** building UV coverage is **12–33%** vs units' **59–73%**. The raw all-pixel
mean is therefore *not* comparable between buildings and units — buildings are penalised by empty UV gutter, not by
being dark. Hence the second, fair column (mean over covered texels only; Footman baseline = **0.2536**, fleet 0.242–0.520).

Secondary metric = **chroma retention vs the approved concept** (shipped flat-lit render ÷ concept render, both masked to
the subject). Calibrated against units Jonathan already signed off: **the accepted floor is Cleric 0.28x / Knight 0.35x.**
Only Castle (0.20x) falls below it. This is what separates "genuinely dark by design" (Knight, BallistaTower) from
"lost the art direction" (Castle).

Third = in-engine truth: `CaptureAssetImage` on all 9 shipped meshes with their real materials, plus a real-lighting
`CaptureViewport` of `Castle_0` in `L_Arena`.

## Two findings that change how the results read — do not skip these

**1. The blue on the roofs is the TeamRegion, not wash-out.** The Blender flat-lit previews show ArrowTower's and
Barracks' roofs as bleached near-white, which looks damning. In-engine those faces are slot `TeamRegion` painted with
`MI_TeamColor_Blue`. The concepts' terracotta/orange roofs were deliberately traded for the team-colour identifier.
**Do not commission a rebuild to "restore the red roof".**

**2. CrystalTower and GoldNode never use their baked textures.** Their main slots are `MI_CrystalGlow` and
`M_GoldGlow` — hand-authored procedural glow materials with no BaseColor/Normal/ORM at all. `T_CrystalTower_*` and
`T_GoldNode_*` are imported but **orphaned**. Any albedo number for these two is irrelevant to what ships, and a
texture rebuild for them would change nothing on screen.

## Castle — the case for rebuild (evidence)

- **Albedo 0.0078 raw / 0.0653 covered** — 21x / 3.9x below the Footman PASS baseline; the lowest of every asset
  measured, buildings and units, before or after the remaster.
- **It was never de-lit.** `Tools/ArtPipeline/Cache/Castle/refine_report.json` (dated 2026-07-07) has **no
  `albedo_delight` block at all** — it predates TASK-193, so the de-light stage never ran. All 8 other buildings have one.
- **Chroma retention 0.20x** — below the worst unit Jonathan has accepted (Cleric 0.28x).
- **Real-lighting proof:** under the `L_Arena` sun it renders as a near-black mass, *darker than the grass it stands on*
  → `.claude/pipeline/handoffs/TASK-309-audit-Castle-in-arena.png`.
- Incidental structural debt on the same asset: **`lod_count == 1`** (no LOD chain; all 8 other buildings have 4) and
  **40,000 tris** (2x the 20k towers).
- It does use its bad textures: `MI_Castle_PBR` → `T_Castle_D/N/ORM`, so this is what ships.

### If the manager decomposes a Castle rebuild
- Use the **BUILDING** pipeline variant, not the UNIT path (no rig, no skeleton, no `A_*` clips).
- Slots must stay exactly **`[TeamRegion, CastlePBR]`** — `TeamRegion` → `MI_TeamColor_Blue` is what tints the spires;
  losing it breaks team identification on both castles.
- Same-path overwrite of `/Game/Meshes/SM_Castle` (**never** delete+recreate) — `Castle_0` and `Castle_1` are placed
  in `L_Arena` and are the win-condition actors.
- Pin the proven recorded-dark delight override `{ao_divide_strength 1.0, ao_floor 0.25, gamma 0.55, gain 1.2}` in the
  manifest — the same values that took Footman 0.0498 → 0.1639.
- Add the missing LOD chain; Nanite OFF; collision per law.
- Bounds to preserve: ~814 x 819 x 898 uu, pivot at min-Z ≈ 0.44.

## Separate flag — GoldNode emissive (NOT a rebuild, a material tweak)

`M_GoldGlow`'s `GlowIntensity` is driven so hard that **86.3% of the mesh renders past luma 0.85** — it reads as a
featureless cream blob, losing the grey rock / gold crystal / orange-crack separation the concept has. Contrast
CrystalTower's comparable glow at 6.5% blown, which still reads as a crystal. This is a one-parameter scalar tune on an
existing material, not a pipeline rebuild, and it needs Jonathan's eye on the intended look before anyone changes it.

## Incidental finding — the Wizard still carries the pre-fix profile

Outside this task's scope but worth a task: **`T_Wizard_D` mean linear albedo 0.0968** on the old conservative profile
`ao0.6/floor0.35/gamma0.85/gain1.0`. It is the only *unit* still below the 0.1639 baseline (the 11 remastered units are
all 0.14–0.36 on the new profile). The Wizard shipped before the profile fix and was not in the fleet-11 list. Jonathan
approved its look at the time, so this is informational — his call whether it gets a re-bake.

## Systemic note

All 8 buildings that have a delight block ran the **conservative** profile `ao0.6/floor0.35/gamma0.85/gain1.0` — the
exact profile that was rejected for the Footman pilot (0.0498) and replaced by `ao1.0/floor0.25/gamma0.55/gain1.2`.
So the buildings do carry the same *process* defect by construction. The audit's finding is that the *outcome* is
nonetheless acceptable for 8 of them, because grey stone and dark timber are the correct art direction — low
saturation is faithful here in a way it was not for the units' cloth and skin.

## Evidence files

- `.claude/pipeline/handoffs/TASK-309-audit-Castle-in-arena.png` — Castle under real `L_Arena` lighting (the decisive shot)
- `.claude/pipeline/handoffs/TASK-309-audit-concept-vs-shipped-A.png` — Castle / ArrowTower / Barracks
- `.claude/pipeline/handoffs/TASK-309-audit-concept-vs-shipped-B.png` — BombTower / BallistaTower / Wall
- `.claude/pipeline/handoffs/TASK-309-audit-concept-vs-shipped-C.png` — DeepMine / CrystalTower / GoldNode

## Compliance

Read-only throughout: no mesh/texture/material mutation, no reimport, no Blender run, **no Meshy calls (zero credits)**,
no Git, no Blueprint, no `TASKBOARD.md` edit, nothing published or uploaded. PIE/Simulate was confirmed **not running**
before any capture and none was started; no package was saved and `L_Arena` was not saved. The viewport capture used an
explicit `captureTransform`, so the user's editor camera was left where he had it.
