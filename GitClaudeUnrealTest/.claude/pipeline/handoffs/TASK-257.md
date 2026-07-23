# TASK-257 — Editor: TeamFill lights + castle nodes DELETED, M_GoldGlow GlowIntensity (handoff)

**Author:** art-director · **Date:** 2026-07-22 · **Branch:** `m7.6-arena10x` (verified checked out before any edit; L_Arena + M_GoldGlow are branch-owned per the M7.6 ownership extension)
**Editor:** UnrealEditor PID 37288 (fresh launch 2026-07-22 13:04, NOT the overnight 12060 lineage), MCP up, PIE was NOT running (Jonathan not mid-PIE — no coordination pause needed).
**Status: done-pending-DA-handover** — pre-compile parts (1) and (2) landed + saved; part (3) is sequenced INSIDE TASK-258's window (see §for-258).

## 1. L_Arena deletions (saved, not-dirty)

Deleted from `/Game/Maps/L_Arena` (labels verified via get_label BEFORE deletion — no guessing):

| Actor (level name) | Label | Location | Why |
|---|---|---|---|
| `RectLight_0` | `TeamFill_Cool_Blue` | (−12,500, 0, 1,600) | Jonathan directive: team mood lighting removed outright |
| `RectLight_1` | `TeamFill_Warm_Red` | (+12,500, 0, 1,600) | same |
| `GoldNode_0` | `GoldNode_Blue` | (−24,200, 0, 0) | castle-node economy dies; never-ship-8-nodes law |
| `GoldNode_1` | `GoldNode_Red` | (+24,200, 0, 0) | same |

- Post-delete verification: `find_actors` for `TeamFill`, `GoldNode`, AND `RectLight` all return **zero** matches in L_Arena.
- Deleting the GoldNode actors BEFORE the class-change compile also removes the two instances serializing the removed `Team` property (TASK-253 deviation #5 noted they'd drop it silently — now moot).
- **Save discipline:** `save_assets(["/Game/Maps/L_Arena"])` ONLY — the sanctioned branch-lane L_Arena edit. No other asset was saved; `is_dirty(L_Arena) == false` confirmed.

## 2. M_GoldGlow `GlowIntensity` (saved, not-dirty)

Authored exactly per the TASK-253 coordination flag — a **NEW multiplicative scale, default 1.0, inserted INTO the existing emissive chain**, NOT a rename/replacement of any existing constant:

- Original chain (untouched, byte-identical): `EmissiveColor ← Multiply_2 (A: Multiply_0 [color × intensity], B: Add_0 [sine-pulse chain])`.
- New wiring: `EmissiveColor ← Multiply_3 (A: Multiply_2 [entire original chain], B: ScalarParameter "GlowIntensity" = 1.0)`.
- Node objects: `MaterialExpressionMultiply_3` + `MaterialExpressionScalarParameter_0` (ParameterName `GlowIntensity`, DefaultValue 1.0). Stock nodes only — Custom-HLSL ban held.
- Recompiled clean (the recompile tool raises on shader failure — none).
- **Neutrality verified two ways:** (a) ×1.0 is a mathematical identity on the full chain — undriven renders byte-identical, and a code-driven `GlowIntensity=1.0` (full mine) is ALSO byte-identical to the authored look, which is the whole point of the 253 flag; (b) asset thumbnails captured before/after the edit (`mat_before.png` / `mat_after.png` in the Cache path below) — emissive sphere identical in hue and brightness.
- Code contract now live once compiled: TASK-253's lazy MID drives `GlowIntensity` = Lerp(0.05, 1.0, Reserve/Initial). The glows-regardless-of-team law holds (intensity modulation only).

## 3. Visual sanity captures (durable)

`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\ArtPipeline\Cache\TASK-257\`

| File | Shows |
|---|---|
| `before_overview.png` / `after_overview.png` | Same camera (0, −38,000, 16,000): warm+cool team-fill ground pools clearly visible before → uniform neutral field lighting after |
| `before_blue_node.png` / `after_blue_node.png` | Blue castle apron: glowing GoldNode_Blue labeled before → gone after (annotation pass found **0** GoldNode-class actors) |
| `before_red_node.png` / `after_red_node.png` | Mirror proof on the Red side (GoldNode_Red labeled before, 0 after) |
| `mat_before.png` / `mat_after.png` | M_GoldGlow thumbnail — GlowIntensity neutrality |

## §for-258 — DA population handover (REQUIRED, inside TASK-258's window)

The `Scatter|Mines` fields on `USiegeScatterConfig` (TASK-255) are **NOT COMPILED YET** — they do not exist in the editor's reflection data today, so `DA_BattlefieldScatter` cannot be populated in this session. This is the sequencing the board already encodes. **Immediately after TASK-258's compile + editor relaunch, populate the DA's `Scatter|Mines` block:**

| Field | Value |
|---|---|
| `MineCountPerSide` | 3 |
| `MineMinSpacing` | 3000 |
| `MineClearanceRadius` | 600 |
| `MineGoldReserve` | 300 |
| `MineEdgeMargin` | 600 |
| `MineMaxSlopeDeg` | 30 |
| `MineClass` | null (⇒ AGoldNode) |

Notes for the executor (build-master window; art can be re-dispatched into it if preferred):
- These are also the C++ defaults (TASK-255 authored them as such), so a fresh reflection load should already show these values — populating = CONFIRM each field explicitly, override any stale serialized value, and **save DA_BattlefieldScatter not-dirty**. The old `GoldNodeKeepClearRadius` serialized value drops silently on load (property removed) — expected, not an error.
- Then flip TASK-257 to `done` on the board.

## Integration notes
- No Blueprint/actor wiring was touched (build-master's lane). No Git operations performed (branch check was read-only).
- L_Arena now has NO gold nodes by design until `PlaceMines` spawns the 6 mirrored mines at runtime post-TASK-258 — a PIE run before the compile lands would legitimately show zero mines and zero miner income; don't flag it as a bug.
- The two TeamFill RectLights are gone entirely (not disabled) — the arena reads neutrally lit; if Jonathan wants mood lighting back later it's a new-feature ask, not a revert.
