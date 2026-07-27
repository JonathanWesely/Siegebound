# TASK-331 — [CASTLE-crumble] Re-derive SM_Castle_Crumble01/02/03 from the rebuilt castle + 75/50/25 % verify — build-master handoff

**Date:** 2026-07-27 · **Branch:** main · Same editor session as TASK-330 (`fcb1ec0`), own commit. Simulate STOPPED during derivation; verification in Simulate.
**Verdict: PASS with one ⚠️ MANAGER FLAG (stage visual differentiation — observation below; NOT re-tuned, per ruling).**

---

## Derivation (TASK-157 recipe against the REBUILT castle)

Same-path overwrite, never delete+recreate: each crumble asset was overwritten IN PLACE by reimporting the SAME `Content/RawAssets/Castle.fbx` that built the rebuilt `SM_Castle` (object identity preserved; `does_asset_exist` gated — overwrite only). Geometry/UVs/collision therefore identical to pristine BY CONSTRUCTION, then per stage: Nanite OFF, castle LOD chain (`lod_group=None`, LOD1 50 % @ 0.4 / LOD2 25 % @ 0.15), the same 11 manifest UCX boxes, and **`/Game/Materials/MI_Castle_Crumble0N` assigned to BOTH slots** (`ACastle::ApplyCrumbleStage` sets slot 0 only at runtime — slot 1 must be baked into the saved asset or the damaged castle renders half-pristine). MIs live at `/Game/Materials/` (code-path contract), NOT `Instances/`. `M_CastleCrumble` and the three MIs were **NOT re-authored** — they sample the same-path `T_Castle_*` and picked the new albedo up for free.

### Readbacks (MCP StaticMeshTools, per asset — all three identical)

| Check | Before (stale) | After (re-derived) |
|---|---|---|
| LOD count | 1 | **3** |
| Tris LOD0/1/2 | 40,000 / — / — | **20,000 / 10,000 / 5,000** |
| LOD0 verts | — | **24,333** (== rebuilt SM_Castle; no unweld, no 0-tri LOD — hard gate PASS on all three) |
| Bounds | old (min-Z 0.445) | **bit-identical to rebuilt SM_Castle**: min (−407.30075, −410.29462, 0.56111) / max (407.21713, 410.26984, 895.42725) — "crumble bounds == pristine bounds exactly" satisfied to full float precision |
| Nanite | off | **off** |
| Collision | — | **11 box hulls** (same manifest boxes as pristine ⇒ UCX footprint preserved; `Castle.cpp:317` "visual swap only" contract intact) |
| Slots | both → `MI_Castle_Crumble0N` | **both → `MI_Castle_Crumble0N`** (re-baked) |

## Simulate verify (Castle_Blue driven via world damage — no instigator ⇒ `TryGetInstigatorTeam` false ⇒ untyped 100 %)

| Step | HP | `CrumbleStage` | Live mesh | Live slot0 / slot1 |
|---|---|---|---|---|
| −520 | 1480 (74 %) | **1** | `SM_Castle_Crumble01` | `MI_Castle_Crumble01` / `MI_Castle_Crumble01` |
| −500 | 980 (49 %) | **2** | `SM_Castle_Crumble02` | `MI_Castle_Crumble02` / `MI_Castle_Crumble02` |
| −500 | 480 (24 %) | **3** | `SM_Castle_Crumble03` | `MI_Castle_Crumble03` / `MI_Castle_Crumble03` |
| `ResetCastle()` | 2000 | **0** | `SM_Castle` (pristine rebuilt) | `MI_TeamColor_Blue` / `MI_Castle_PBR` (per-team accent via `ApplyTeamVisuals`) |

- **Each stage fired ONCE, in order** — `LogGitClaudeUnrealTest` lines at 21:30:12 (stage 1) → 21:31:27 (stage 2) → 21:31:48 (stage 3), one line each, "mesh/material swap + debris".
- **Texture mapping on the new geometry is CLEAN** — close-up (`TASK-331-verify-crumble-stage3-closeup-uvcheck.png`): the char/speckle pattern follows walls, crenellations, tower cylinders and gatehouse arches with no scrambling, no UV smear, no stretching. The pre-TASK-331 scramble risk is closed.
- **Whole castle reads damaged** (both slots) — no half-crumbled roofs-vs-walls split.
- **Debris:** the burst code path ran on every stage (log lines above; `SpawnNiagara` → `/Game/VFX/NS_CastleDebris`, null-safe). The transient burst itself was not caught in the still captures (it spawns as a short-lived pooled component, not an actor) — no error, path exercised.
- **Restore:** readback table above + `TASK-331-verify-reset-restore.png` (bright pristine castle, blue accents, live-match Ogre in frame).
- **Message Log clean:** `Ensure condition failed|Accessed None|Fatal error|LogOutputDevice: Error` ⇒ 0 hits across the session.

## ⚠️ MANAGER FLAG — stage differentiation against the new bright base (spec item 4; observation only, NOT re-tuned)

Measured on the same lit castle-wall region, identical camera pose, exposure-consistent scenes (grass control 11.4/11.5/11.6):

| State | Wall luma (0–255) |
|---|---|
| Pristine (rebuilt) | **77.5** |
| Stage 1 (Darken 0.80 / Scorch 0.12) | **28.0** |
| Stage 2 (0.50 / 0.45) | **28.9** |
| Stage 3 (0.30 / 0.80) | **29.2** |

- **Stage 3 still reads as a near-dead charred silhouette — PASS.**
- **Stage 1 does NOT read "battle-worn but standing" any more** — with the new bright albedo, the pristine→stage-1 transition is a cliff (77.5 → 28.0) and stage 1 already reads nearly as charred as stage 3. The 1→2→3 progression is monotonic in DATA (correct MI per stage, shipped tuning intact: 01 `0.80/0.12/0.30`, 02 `0.50/0.45/0.60`, 03 `0.30/0.80/0.85`) but **visually flat between stages at gameplay distance** (28.0 / 28.9 / 29.2 — the sunlit close-up confirms it is material, not shadow). The shipped tuning was authored (TASK-157) against the old near-black texture; against the new base the scorch/char term dominates all three stages.
- **Per manager ruling I did NOT touch `M_CastleCrumble` or the MI parameters** — this is an art call. If ruled, the likely lever is re-spreading `Darken`/`ScorchAmount` across the stages (e.g. stage 1 toward a mild darken with low scorch) — a zero-geometry MI-parameter pass.

## Observations for the record
- The board note "`Castle_Red` carries yaw 180" did not reproduce — both castle actors read yaw 0.0 (also noted in the TASK-330 handoff).
- During the TASK-330 Simulate session the live bot match independently drove `Castle_0` through all three stages (log 21:23:2x) — the crumble chain also fires under real combat, not only under scripted damage.
- `L_Arena` never saved (left dirty in-editor). Editor left running; remote-exec flag in-memory only.

## Commit
`Content/Meshes/SM_Castle_Crumble01|02|03.uasset` + this handoff + 5 verify PNGs. Nothing foreign; NO push. Board status flips for TASK-330/331 follow in a docs-only commit (so both lines can carry their commit hashes).
