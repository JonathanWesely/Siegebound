# Handoff — TASK-226 — Environment life: emissive pulse on OUR glow materials (art-director)

**Date:** 2026-07-19 01:22–01:26 (overnight batch) · **Status: DONE (pulse shipped); WPO sway SKIPPED per Ruling C (recorded below).**

## What shipped

Subtle sine emissive pulse spliced into BOTH of our glow masters via `MaterialEditingLibrary` over the TASK-221
python remote-execution lane (same recipe as TASK-225-B; live editor, no restart):

- `/Game/Materials/M_GoldGlow` and `/Game/Materials/M_CrystalGlow`
- Graph: `Time → Sine(period 10 s = 0.1 Hz) → ×0.12 → +1.0 → Multiply` spliced BETWEEN the existing emissive
  chain and the EmissiveColor pin — the original emissive graph is fully intact upstream (input A of the new
  Multiply); pulse factor swings 0.88–1.12 (±12%, inside the board's ±10–15% band).
- The splice Multiply carries `desc="TASK226 pulse mult"` (idempotency guard + findable in the graph editor).
- NO parameters added → NO MI changes needed; MI_CrystalGlow / GoldNode wiring untouched.

## Verification (all PASS)

- **Structural readback:** post-splice, EmissiveColor input on both materials is the desc-tagged Multiply;
  both `recompile_material` calls clean.
- **No shader errors:** editor log clean for the window (no "Failed to compile", no LogShaderCompilers errors);
  MapCheck 0/0 after the final reload.
- **Breathing proven quantitatively:** 4 timed captures of the placed `GoldNode_Blue` (world-time read back per
  shot) rank EXACTLY as the sine predicts — predicted factors 0.889 / 0.924 / 0.942 / 1.116 → measured glow
  means 174.57 / 175.10 / 175.33 / 177.21 (perfect rank correlation). Per-pixel min-vs-max diff: GoldNode 291k
  pixels changed (emissive spill on the ground breathes too); CrystalTower ~4k pixels changed >15 at the crystal
  top, glow-region ratio 1.029. Post-tonemap delta is deliberately subtle (tasteful, not a disco) — the linear
  ±12% compresses in the tonemapper's shoulder.
- **Glow integrity eyeballed:** crystal capture shows the height-masked cyan top + dark stone body intact;
  GoldNode warm glow intact.

## Evidence (durable)

`Tools/ArtPipeline/Cache/_TASK226_report/`:
- `TASK226_gold_phase{A,B,C,D}.png`, `TASK226_crystal_phase{B,C,D}.png` (timed captures)
- `AB_TASK226_pulse_min_vs_max.png` (2×2 labeled strip: both subjects at sine-min vs sine-max)

CrystalTower is card-summoned (not placed in L_Arena) — its captures used a transient preview spawn, deleted
afterwards.

## WPO tree-sway — SKIPPED + recorded (Ruling C)

No lawful edit path exists tonight: the scatter/vegetation meshes are Fab donors (`Content/Fab/` READ-ONLY law)
and `DA_BattlefieldScatter` is M7.6-branch-owned (no repoints on main). Deferred; revisit post-merge or via
duplicated-donor materials under /Game/ as an M7.6-branch task.

## Editor / dirty state (for TASK-228)

- **Saved: ONLY `M_GoldGlow` + `M_CrystalGlow`.** Pre-save dirty readback confirmed the recompile dirtied ONLY
  those 2 content packages (no MIs, no meshes). Transient actors (SceneCapture + preview CrystalTower) deleted;
  L_Arena reloaded from disk (map never saved); final readback: **zero dirty maps, zero dirty content**.
  Editor RUNNING, L_Arena loaded clean.
- **Git/SCC disclosure (read-only status, no mutations):** `Content/Materials/M_{GoldGlow,CrystalGlow}.uasset`
  show ` M` modified-UNSTAGED. Combined with TASK-225-B: 5 `Content/Textures/T_<CardID>_D.uasset` also ` M`
  unstaged; plus the part-A/223 raw-file changes listed in handoffs/TASK-225.md §Part B.
- Runtime perf: no new textures/params, one extra scalar multiply chain per material — no perf-bar impact.
