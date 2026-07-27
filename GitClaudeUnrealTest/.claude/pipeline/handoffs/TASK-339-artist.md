# TASK-339 — [CASTLE-crumble-fix] DONE: `M_CastleCrumble` ORM sampler `LinearColor → Masks`, master resaved, compile CLEAN, all three crumble MIs render again

**Status: complete.** Date: 2026-07-27 · art-director. **Saved: `Content/Materials/M_CastleCrumble.uasset` ONLY. No Git (TASK-338 commits it). `L_Arena` never saved. No texture touched (`T_Castle_ORM` stays `TC_Masks`).**

## The one-enum change (before → after)

| | Node | Param | Texture (node default) | SamplerType |
|---|---|---|---|---|
| BEFORE | `MaterialExpressionTextureSampleParameter2D_2` | `ORM` | `/Game/Textures/T_Castle_ORM` | `SAMPLERTYPE_LinearColor` |
| AFTER | same node — nothing else touched | `ORM` (unchanged) | unchanged | **`SAMPLERTYPE_Masks`** |

Readback-verified before and after via ObjectTools (`ParameterName`/`Texture`/`SamplerType`). Zero graph-topology change, zero parameter change (the other two samplers `BaseColor`=SAMPLERTYPE_Color / `Normal`=SAMPLERTYPE_Normal untouched; all scalar/vector params untouched). Exactly the `M_AssetPBR` combination: Masks compression, linear channels, `.rgb` → O/R/M semantics preserved.

## Compile evidence (WEDGE-WATCH honored — no wedge)

- **Timeline proven from the editor log:** `set_properties` (the enum) at `22.45.03`, `MaterialTools.recompile` at `22.45.14` — the recompile tool RAISES on shader failure and returned success. Editor game thread stayed alive throughout (every subsequent MCP/remote-exec call answered normally; two full Simulate sessions ran after the fix).
- **`Failed to compile Material` grep = 0 hits post-fix.** Last failure line in the session log is `22.33.20` (pre-fix, the MI01 lines from the prior TASK-337 session). Nothing after the enum change — through recompile, three MI thumbnail renders, master save, two Simulate sessions and all four stage drives.
- **`Sampler type is Linear Color, should be Masks` grep:** last hit `22.31.24` (pre-fix MaterialEditorStats). Zero post-fix.
- **Master resaved with Simulate STOPPED** — `is_dirty` false after save; this also clears the standing "recompiles every editor launch until resaved" debt. On-disk diff: `git status` shows `Content/Materials/M_CastleCrumble.uasset` modified.

## MI render confirmation (not Default Material)

- Thumbnail renders post-fix: `MI_Castle_Crumble01/02/03` all render the castle atlas through the real material chain — MI01/MI02 match the known-good `MI_Castle_PBR` atlas pattern, and **MI03 renders visibly charred/darker than MI01/02** — three DIFFERENT renders, impossible under a Default-Material fallback (which renders all MIs identically).
- **In-Simulate acceptance (d):** on `L_Arena`, one castle driven to stage 1 at the SHIPPED MI values renders the scorch-dusted castle — NOT the grey sparkle-mottle. Capture: `TASK-339-stage1-realrender.png` (wall luma 157.3 vs Default-Material's flat 89.4 from the prior session — a completely different render). Full four-state sweep in the TASK-337 re-run handoff.

## For TASK-338 (integration/commit)

- `Content/Materials/M_CastleCrumble.uasset` is the ALWAYS-commit item per the re-adjudicated chain.
- The TASK-337 re-run (same session, appended to `handoffs/TASK-337-artist.md`) measured the band against the first real render: **shipped values FAIL S1/S2 (too bright)** → the conditional retune RAN → `MI_Castle_Crumble01.uasset` + `MI_Castle_Crumble02.uasset` are also saved and join the commit. MI03 untouched.
- SAMPLER-TYPE TRAP sweep reminder: the fallback is a `LogMaterial: Warning`, not an Error — grep `Failed to compile Material` explicitly on your fresh load.

Law: CONVENTIONS "Fleet Meshy remaster" → SAMPLER-TYPE TRAP (this task is its fix-pattern precedent) + "Material & Niagara lane laws" (graph stayed stock nodes).
