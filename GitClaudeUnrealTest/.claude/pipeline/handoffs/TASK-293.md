# TASK-293 handoff — Vista-ring GAP FILL (build-master, RE-DO)

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-25 · HEAD at start `95a1b39` · editor PID 7440 · NO compile.
Re-do of a stalled attempt that persisted nothing (prior in-editor placement lost to an editor crash). This run did the
work SYNCHRONOUSLY as ONE deterministic editor-python placement script and SAVED `L_Arena` to disk immediately
(git-verified modified before screenshots/PIE).

## Problem (Jonathan's screenshot)
The TASK-291 16-instance vista ring was too SPARSE — ~48° angular sky-gaps on the long (±Y) sides, and through the gaps
the ground plane ended and dropped to sky/void at the horizon.

## What was placed — 62 fill instances (`VistaFill_01..62`), one seeded one-shot
Deterministic layout: fixed LCG seed `20260725`, rounded-rectangle ring projection (so nothing lands inside the play box)
+ per-instance radial/yaw jitter. All OUTSIDE nav bounds `NavMeshBounds_Arena` (X±28000 / Y±12500) and outside the
ArenaBoundary walls; none inside the field/corridor/keep-clears.

**3 radial depth bands (layered ridgelines, silhouettes overlap):**
| Band | Rounded-rect half-extents (uu) | Count | Content |
|------|-------------------------------|-------|---------|
| FRONT (tree-line + low ridges) | RX 31000 · RY 17000 | 20 | trees (scale 7–11×) + low hills (6–9×) |
| MID (main ridge, interleaves existing 16) | RX 34000 · RY 20500 | 24 | peak rocks (11–14×) / green ridges (9–13×) / hills (7–10×) |
| BACK (tall peaks behind gaps) | RX 36500 · RY 22500 | 18 | peak rocks (12–15×) + green ridges (10–13×) |

**Composition by family (existing assets only, no new art):**
- Rocks **34** — peaks `SM_Vista_01`/`SM_Vista_04` (20) + green ridges `SM_Vista_02`/`SM_Vista_03` (14)
- Hills **13** — `SM_Hill_01`/`SM_Hill_02`/`SM_Hill_03`
- Trees **15** — `SM-Mobile_Tree_1..12` (`/Game/Tree_Pack_1/Meches/Mobile_Tree_1/`)

Spawn result: **62/62 spawned, 0 spawn failures**. z=0 for all (matches the existing ring's base level).

## ⚠ Vista flags — SET + readback-verified on ALL 62
Every new instance's `StaticMeshComponent0` got the full vista flag set, matching the proven existing `Vista_01..16`:
`CastShadow=false`, `bVisibleInRayTracing=false`, `bCanEverAffectNavigation=false`,
`bAffectDistanceFieldLighting=false`, `bAffectDynamicIndirectLighting=false`.
**`verified_off = 62/62`** (independent readback pass, 0 bad). `set_properties` returned true on all 62.
- Gotcha recorded: the toolset `set_properties` `values` param is a **JSON-encoded STRING**, not an object — passing an
  object is a silent no-op (flags stay at the mesh default `true`). Fixed by `values=json.dumps({...})`.
- Collision: left at the mesh default (the `SM_Vista`/`SM_Hill`/tree meshes carry **no baked collision geometry** — same as
  the existing 16 vistas, which read `QueryAndPhysics` profile but never collide) + `bCanEverAffectNavigation=false`.
  A `BodyInstance.collisionEnabled=NoCollision` set was attempted but is a no-op via this tool AND triggers a component
  reconstruct that reverts the 5 flags — so I matched the PROVEN existing-vista config (nav-off is the load-bearing guard;
  traversability confirmed unregressed below).

## VERIFY (after SAVE)
**Save:** `AssetTools.save_assets(["/Game/Maps/L_Arena"])` → `True`; `git status` showed `Content/Maps/L_Arena.umap`
modified on disk (122KB→521KB) BEFORE any screenshot/PIE.

**Screenshots (editor viewport captures — SENT, paths below):**
- Top-down full perimeter: `.claude/pipeline/handoffs/TASK-293-topdown.png` — the ring is now a CONTINUOUS closed loop of
  hills+rocks+trees 360° around the play rectangle; no angular sky-gap.
- Oblique aerial of the +Y (former-gap) side: `.claude/pipeline/handoffs/TASK-293-oblique-posY-gap.png`.
- Low/outward at former 66° gap: `.claude/pipeline/handoffs/TASK-293-low-gap66.png` — horizon fully backed by ridgeline; sky
  only above the silhouette, no drop-off void.
- Low/outward at former 294° gap (mirror): `.claude/pipeline/handoffs/TASK-293-low-gap294.png` — same, continuous.
- (Definitive gameplay-cam check is Jonathan's; these are left UNSTAGED, not committed, to keep the diff L_Arena-only.)

**LWC clean + traversability — ONE PIE generate (seed `901016449`):**
- `InverseFast` NaN (Matrix.h:468) = **0 matches**; `OriginX<=OriginMax` ensure (DoubleFloat.cpp:19) = **0**;
  `non-invertible` = **0** — across the WHOLE session log. The DF-off flags on all 62 kept the LWC precision family GONE
  (no flag missed). The TASK-292/292c closure holds with the added instances.
- `LogSiegeTerrain: Traversability CONFIRMED — Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s))` on the
  current run (frame 779). 7 scatter layers on target. StopPIE clean.

## Diff / hygiene
Branch-owned diff = **`Content/Maps/L_Arena.umap` only (+ board + this handoff)**. NO code / DA / fleet / gameplay source
touched. `DeckBuilderWidget.cpp/.h` + `WBP_DeckBuilder.uasset` (parked) left MODIFIED-but-UNSTAGED. Screenshots + `.claude/tmp/`
left untracked. LFS handles the umap. NO push.

## Follow-ups for the manager (report-only; non-fatal)
1. From the high oblique (130 m up) a thin sky saddle is visible between two MID hill crests on the +Y side; NOT visible from
   the z=1800 low shots (gameplay height). If a future pixel-check finds a residual sliver, a couple more BACK-band peaks at
   that azimuth close it — existing assets suffice (no wider mesh needed).
2. `InputMode:UIOnly` HUD-focus error (pre-existing, unrelated to dressing) still stands — HUD/deck-builder owner's item.
