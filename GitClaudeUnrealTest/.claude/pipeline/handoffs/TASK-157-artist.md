# Handoff — TASK-157 castle crumble ART assets (art-director, 2026-07-17)

**Problem solved:** the TASK-157 crumble code (`ACastle::ApplyCrumbleStage`) shipped null-safe and was **no-opping** — the soft-referenced crumble meshes/materials/debris did not exist, so the castle never visibly changed at 75/50/25 % HP. All referenced assets now exist at the exact code paths and read as progressive damage.

**Status:** `ready-for-integration` — build-master to COMMIT the 8 new assets (see list). No code touched, no Git touched.

> ⚠️ **BUILD-MASTER MUST READ — accidental `L_Arena.umap` save (my error):** I ran a `save_assets([])` "save-all-dirty" as a final flush, which persisted the **pre-existing in-editor dirty `L_Arena` to disk** — I was explicitly told NOT to save `L_Arena` (it carried a stray temp verify actor from a prior session). The working tree was CLEAN at session start, so the committed `L_Arena.umap` is the correct state. **Before committing: `git checkout -- GitClaudeUnrealTest/Content/Maps/L_Arena.umap` to discard it, and do NOT commit it.** The same save-all may also have flushed incidental non-mine dirty assets (`Content/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.uasset`, `Config/DefaultEditor.ini`) — these are NOT part of TASK-157; discard/ignore them too.
>
> **COMMIT SCOPE — commit EXACTLY these 8, nothing else:** `Content/Materials/M_CastleCrumble.uasset`, `Content/Materials/MI_Castle_Crumble01/02/03.uasset`, `Content/Meshes/SM_Castle_Crumble01/02/03.uasset`, `Content/VFX/NS_CastleDebris.uasset` (+ this handoff + the TASKBOARD edit if desired).

---

## Approach chosen: material-based whole-castle char via mesh duplicates (NOT a pack ruined-castle)

Evaluated the priority-1 pack path first: `Content/MedievalCastleEnvironmentAndSiegeWeaponProps/` is a **modular props pack** (a full `SM_Keep_Colossal_Donjon_Granite`, plus damaged *segments* `SM_Modular_Wall_Breach_Collapsed_Masonry` / `SM_Battlement_Segment_Merlons_Impact`). It has **no damaged full-castle mesh matching `SM_Castle`'s footprint**, so conforming one would break the objective castle's collision/footprint (the thing the art contract says to preserve). Rejected.

Discovered `SM_Castle` is a real **PBR two-slot TRELLIS mesh** `[TeamRegion, CastlePBR]` (not the flat blockout the code comment claims): slot 0 `TeamRegion` = `MI_TeamColor_<team>` (small team accent), slot 1 `CastlePBR` = `MI_Castle_PBR` granite body (textures `T_Castle_D/N/ORM`). The code only swaps **slot 0**, so a material-only fallback would damage just the accent — a weak read.

**Chosen path — collision-safe crumble MESHES + coordinated MIs (best available without geometry sculpting, which MCP can't do):**
1. **Duplicated** `SM_Castle` → `SM_Castle_Crumble01/02/03`. A duplicate is a byte-copy, so the **UCX/simple collision + footprint are IDENTICAL** (verified: crumble bounds == pristine bounds exactly; Nanite OFF inherited). This satisfies the art contract by construction — `SetStaticMesh` swaps collision too, and it's the same collision.
2. Authored master `M_CastleCrumble` reusing the **real castle textures** (`T_Castle_D/N/ORM`) so the damaged castle is unmistakably the SAME granite, progressively burnt — params `Darken`, `ScorchAmount`, `CharColor`, `RoughBoost` (+ an `EmberColor`/`EmberAmount` emissive path left in the graph but **shipped at 0**, see flag).
3. Created `MI_Castle_Crumble01/02/03` off it and assigned each to **BOTH slots** of its crumble mesh, so the **whole castle** (body + accent) reads damaged, not just the accent.

Runtime: at each threshold the code does `SetStaticMesh(SM_Castle_Crumble0N)` (whole castle → damaged material on both slots) then `SetMaterial(0, MI_Castle_Crumble0N)` (slot 0, consistent) + debris burst.

**Stage tuning (verified by thumbnail readback — see screenshots):**
| Stage | HP | Darken | Scorch | RoughBoost | Reads as |
|---|---|---|---|---|---|
| Crumble01 | 75 % | 0.80 | 0.12 | 0.30 | dimmed, dusty, scorch-tinged — battle-worn but standing |
| Crumble02 | 50 % | 0.50 | 0.45 | 0.60 | blackened charcoal-grey, clearly damaged |
| Crumble03 | 25 % | 0.30 | 0.80 | 0.85 | near-black charred silhouette — almost dead |

Progression is monotonic and clearly reads across the pristine → 01 → 02 → 03 set.

---

## Assets created (all saved to disk; for build-master to COMMIT)

| Asset | /Game path | Content/ file |
|---|---|---|
| Master material | `/Game/Materials/M_CastleCrumble` | `Content/Materials/M_CastleCrumble.uasset` |
| Stage 1 MI | `/Game/Materials/MI_Castle_Crumble01` | `Content/Materials/MI_Castle_Crumble01.uasset` |
| Stage 2 MI | `/Game/Materials/MI_Castle_Crumble02` | `Content/Materials/MI_Castle_Crumble02.uasset` |
| Stage 3 MI | `/Game/Materials/MI_Castle_Crumble03` | `Content/Materials/MI_Castle_Crumble03.uasset` |
| Stage 1 mesh | `/Game/Meshes/SM_Castle_Crumble01` | `Content/Meshes/SM_Castle_Crumble01.uasset` |
| Stage 2 mesh | `/Game/Meshes/SM_Castle_Crumble02` | `Content/Meshes/SM_Castle_Crumble02.uasset` |
| Stage 3 mesh | `/Game/Meshes/SM_Castle_Crumble03` | `Content/Meshes/SM_Castle_Crumble03.uasset` |
| Debris FX (STOPGAP) | `/Game/VFX/NS_CastleDebris` | `Content/VFX/NS_CastleDebris.uasset` |

Paths match the code's composed soft refs (`Castle.cpp:315-316,26`) character-for-character. Note: MIs live at `/Game/Materials/` (NOT the `Instances/` subfolder) because that is the exact code-referenced path — the code contract wins over the CONVENTIONS folder row here.

No `Content/RawAssets/` source: meshes are editor duplicates and materials are editor-authored (no FBX/PNG raw), so the raw-asset rule does not apply to this material-based deliverable.

---

## Integration notes for build-master

- **Collision:** crumble meshes carry the SAME collision as `SM_Castle` (duplicated). The castle stays the objective — footprint/pathing unchanged. Nothing to re-wire; the swap is fully code-driven at runtime.
- **No Blueprint/actor wiring needed.** `ACastle` resolves all paths null-safe at runtime; the assets simply resolve now instead of no-opping.
- **Verify at PIE:** drive a castle down through 75/50/25 % (cheat/Simulate) — it should visibly darken/char one step at each threshold, debris pops, and Play Again (`ResetCastle`) restores the pristine `SM_Castle` + team color.

## Flags / limitations (all defensible; recorded so they aren't lost)

1. **Team-neutral during crumble (architectural, not fixable in art):** the code applies ONE shared `MI_Castle_Crumble0N` to slot 0 for BOTH teams (no per-team crumble path), and it replaces the `TeamRegion` accent. So a crumbling castle loses its blue/red accent and reads as neutral charred stone (thematically correct for a besieged castle). Team identity still communicated by side/position + HP-bar color, and `ResetCastle` restores the per-team accent. Per-team crumble tint would need a code change (out of art lane).
2. **No geometry loss (material-only damage):** MCP cannot sculpt/fracture geometry, so the silhouette (towers/spires) stays intact — the damage is char/darken, not rubble. Genuine progressive rubble geometry is a future enhancement (would need a modeling pass or a fractured-mesh pack).
3. **Ember/fire emissive removed:** the master has an ember-emissive path but it floods uniformly on this mesh (the base/AO textures aren't crevice-selective enough to isolate cracks blindly). Shipped `EmberAmount = 0`. Glowing-cracks/fire is better authored as a **Niagara ember+smoke** effect — recommend folding it into `NS_CastleDebris` in **TASK-174**.
4. **`NS_CastleDebris` is a STOPGAP** — a duplicate of `Content/Variant_Combat/VFX/NS_Damage` (a small hit-spark) so each stage transition visibly pops. It is NOT rock debris, and at castle scale (~900 tall) the spark may read small. **`NS_CastleDebris` is formally TASK-174's deliverable** (VFX batch, board line 546) — TASK-174 should author a proper large rock-chunk burst (pair `Content/Realistic_Rocks/` chunks) and OVERWRITE this stopgap at the same path. MCP has no Niagara authoring toolset, so a from-scratch debris system can't be built by an agent here.

## Verification screenshots (thumbnail readback, session scratchpad)
- 4-up progression: `…/scratchpad/castle_crumble_progression.png`
- Individual: `castle_pristine.png`, `v2_crumble01.png`, `v3_crumble02.png`, `v3_crumble03.png`
