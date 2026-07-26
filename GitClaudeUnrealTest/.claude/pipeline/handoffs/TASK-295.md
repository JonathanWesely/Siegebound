# TASK-295 handoff — Decorative backdrop ground apron (build-master)

**Branch:** `m7.6-arena10x` · **Date:** 2026-07-25 · HEAD at start `b5dcb8f` (TASK-294 hash-record atop `238a2da`) · NO compile.
Worked SYNCHRONOUSLY: ONE deterministic editor-python script that spawned the apron + set material/flags + SAVED `L_Arena`
atomically, save persisted to disk (git-verified umap modified) BEFORE any screenshot/PIE. MCP client wedged → drove
everything via the raw-HTTP MCP fallback (`.claude/tmp/mcpc.py`, dual-channel SSE) + `ProgrammaticToolset.execute_tool_script`.

## Problem (Jonathan's screenshot)
From the RTS/angled gameplay cam the playable field ends at the nav boundary and there is a VOID horizon band between the
field edge and the vista ring, so the vista rocks/hills read as "floating." Fill the gap with decorative ground — VISUAL ONLY,
behind the nav barrier — while keeping the playable/walkable/nav area EXACTLY unchanged.

## Field ground I matched (read pass — authoritative live values)
- Actor **`ArenaGround`** (`StaticMeshActor_1`): mesh **`/Engine/BasicShapes/Cube.Cube`**, override material
  **`/Game/Materials/Instances/MI_BattlefieldGround`** (the M6.5 tri-planar grass instance), loc (0,0,−50), scale (560,250,1)
  → **top surface Z=0**, bounds **X±28000 / Y±12500** — i.e. the field ground ENDS EXACTLY at the nav boundary; beyond it is void.
- Boundary walls `ArenaBoundary_{N,S,E,W}` at X±27500 / Y±12500, tops Z=1800 (opaque grey `BasicShapeMaterial` — the pre-existing
  barrier that occludes the gap from a LOW eye-level cam; the void is seen from the higher/angled RTS cam that looks over it).

## What I added — ONE decorative apron (`BackdropApron`, `StaticMeshActor_33`)
- SAME mesh **`/Engine/BasicShapes/Cube`** + SAME material instance **`MI_BattlefieldGround`** (tri-planar projects on world
  position → grass tiles CONTINUOUSLY across the field/apron boundary, no UV seam, exact color match).
- Transform: loc **(0,0,−55)**, scale **(800,640,1)** → world half-extents **X±40000 / Y±32000**, **top surface Z=−5**.
  - Top is **5 uu below** the field top (Z=0): the apron UNDERLAPS the field, the opaque field body (Z 0→−100) hides the apron
    in the overlap, and the 5 uu step at the field edge is sub-pixel from gameplay height → **no z-fighting, no visible seam**.
  - Extent reaches **past the vista-ring bases** (ring origins ~X 30.5–36.5k / Y 15–22.5k) so the apron's own far edge lands
    UNDER the tall ring meshes on every side → the apron edge is hidden behind the ring (no visible apron edge). No new art asset
    needed (reused the engine Cube + existing MI).
- ⚠ FLAGS (readback-verified, all = false): `CastShadow`, `bVisibleInRayTracing`, `bCanEverAffectNavigation`,
  **`bAffectDistanceFieldLighting`**, **`bAffectDynamicIndirectLighting`**. The two DF/GI flags are the load-bearing guard against
  re-triggering the TASK-292/292c LWC precision NaN at far coords — confirmed held (LWC clean below). The apron still RECEIVES
  lighting → renders lit green grass (verified in screenshots), the flags only stop it CONTRIBUTING to DF/GI.
- Collision: `collisionProfileName` SET to **`NoCollision`**; the derived `collisionEnabled` enum reads `QueryAndPhysics` because
  it is NOT writable via the toolset `set_properties` (the KNOWN TASK-293 BodyInstance limitation — set returns true, value
  unchanged). This is a non-issue: `bCanEverAffectNavigation=false` (verified) is the load-bearing nav guard, the apron sits
  entirely BELOW the field surface and outside the walkable rect (units can't reach it), and the traversability PIE proves the
  walkable area is unchanged. Ordered collision→material→flags LAST so any reconstruct couldn't revert the flags (none occurred).

## VERIFY (all AFTER save)
- **Save persisted:** `AssetTools.save_assets(["/Game/Maps/L_Arena"])` → True; `git status` showed `Content/Maps/L_Arena.umap`
  modified on disk (LFS pointer changed) BEFORE any screenshot/PIE.
- **Gap-filled proof — screenshots (SENT, unstaged in `handoffs/`):**
  - `TASK-295-eye-posX-ingap.png` / `TASK-295-eye-posY-ingap.png` — eye-level IN the former gap: apron grass runs unbroken right
    up to and under the vista rock/hill bases — **no void, vista fully grounded** (±Y is the widest former gap; +X the long side).
  - `TASK-295-oblique-posY.png` — the grass runs continuously field → over the boundary → up to the vista hills, no sky-gap strip.
  - `TASK-295-topdown.png` — the apron forms a rectangular grass skirt around the field, reaching the vista ring on all sides.
  - `TASK-295-rts-posY.png` / `-rts-negY.png` / `-rts-posX.png` — RTS-angle (looks over the boundary wall): continuous grass, no void.
  - `TASK-295-posY-gap.png` / `-negY-gap.png` / `-posX-gap.png` / `-corner.png` — low eye-level shots; here the OPAQUE grey
    boundary wall fills frame (pre-existing element), grass continuous below/beyond it. (Kept for completeness; the angled/grazing
    shots above are the definitive gap proof.)
- **Playable area UNCHANGED — LWC clean + traversability, ONE fresh PIE (seed 710563009):** polled for a FRESH confirmation —
  `[2026.07.25-23.36.29] Traversability CONFIRMED — Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s))`. LWC precision
  family ALL ZERO across the whole session log: `InverseFast` NaN = **0**, `OriginX<=OriginMax` ensure = **0**, `non-invertible`
  = **0** → the apron's DF-off/GI-off flags held (no far-coord NaN). 7 scatter layers on target; StopPIE clean. Nav bounds +
  playable ground + scatter + vistas + POIs + gold props all untouched.

## Diff / hygiene
Branch-owned diff = **`Content/Maps/L_Arena.umap` only** (+ board + this handoff). **No new mesh/material asset created**
(reused engine Cube + existing `MI_BattlefieldGround`). NO code / DA / fleet / gameplay source touched.
`DeckBuilderWidget.{cpp,h}` + `WBP_DeckBuilder.uasset` (parked) left MODIFIED-but-UNSTAGED. Screenshots + `.claude/tmp/`
untracked. LFS handles the umap. NO push.

## Follow-ups for the manager (report-only; non-fatal)
1. `collisionEnabled` on the apron reads `QueryAndPhysics` despite `collisionProfileName=NoCollision` — the toolset can't write
   the derived enum (TASK-293 limitation). Harmless here (nav-off + below-surface + out of reach). If a future pass wants the
   apron truly NoCollision on the enum, it needs a C++/commandlet `SetCollisionEnabled(NoCollision)` (out of build-master's
   editor-python lane) — same known gap as the vista dressing.
2. The grey `ArenaBoundary_*` walls (opaque `BasicShapeMaterial`, top Z=1800) are visible from a low eye-level cam and read as a
   flat grey band. Pre-existing, unrelated to this task — if Jonathan dislikes them a future pass could hide/retexture them.
3. `InputMode:UIOnly` HUD-focus error (pre-existing, unrelated to dressing) still stands — HUD/deck-builder owner's item.
