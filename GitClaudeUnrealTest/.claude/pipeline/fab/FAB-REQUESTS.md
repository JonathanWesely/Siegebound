# Fab / Marketplace Request Lane — FAB-REQUESTS.md

Created 2026-07-07 (TASK-082..088 chain; Jonathan-approved plan). Owned by the **manager**; entries AUTHORED by the **art-director**; APPROVED + FULFILLED by **Jonathan** (human-only — agents cannot browse, buy, or download Fab/marketplace content; acquisition requires his Epic Launcher).

## Protocol

1. **requested** — art-director (or manager) adds a `FAB-###` entry below (IDs increment from the highest existing) when a task would benefit from a marketplace pack. Note the request in the task handoff + a one-liner in the 🎨 Art Slack thread.
2. **approved** — Jonathan says yes (in Claude Code or Slack; the orchestrator records it here). A Slack post alone is never acquisition authorization — Jonathan's word is.
3. **fulfilled** — Jonathan downloads the pack into the project via the Epic Launcher. Packs land in `Content/Fab/<Pack>/` — **READ-ONLY donor quarantine** (CONVENTIONS "Textured mesh law": soft-reference or duplicate into /Game/, never edit in place). Jonathan (or the orchestrator on his word) records the landing path here.
4. **integrated** — a board task conforms the asset (reuse the Stage-2 pipeline: `refine_trellis_glb.py` on the Fab mesh → budgets/UVMap/two-slot split/axis contract) and swaps it in via the same-path `SM_<AssetName>` overwrite. Record the task ID here.

Statuses flow strictly `requested → approved → fulfilled → integrated` (or `rejected`, with Jonathan's reason). This file is the source of truth for the lane; Slack mirrors it.

## Rules

- License note is MANDATORY at request time — the pack must permit game use.
- No agent ever fetches marketplace content by any other route (no web downloads, no asset-store scraping).
- Fab meshes obey the full "Textured mesh law" after conform: two-slot `[TeamRegion, <AssetName>PBR]`, Nanite OFF, collision per law, same-path swap only.

## Entry template

```
### FAB-000 — <pack / asset name>
- status: requested | approved | fulfilled | integrated | rejected
- requested-by: art-director (TASK-###)
- for: <SM_/T_ asset(s) it should replace or supply>
- fab-link: <URL or exact search terms>
- license-note: <confirms game-use permission>
- fulfilled-note: <Jonathan: landing path under Content/Fab/<Pack>/>
- integration: <task ID that conformed + swapped it>
```

## Requests

### FAB-001 — Stylized tree pack (M4.5 arena trees)
- status: approved
- requested-by: manager (Jonathan's 2026-07-08 directive — his directive IS the approval; TASK-092 consumes)
- for: SM_Tree_01 / SM_Tree_02 (Content/Meshes/) — arena obstacle trees. Conform (TASK-092): duplicate donor meshes into /Game/, rename per CONVENTIONS, ≤4k tris each, Nanite OFF, TRUNK-ONLY collision (canopy collision stripped/none), origin trunk-base ground-center; §6 stylized bar (chunky silhouette, no flat single-color). Environment assets are NEUTRAL — the two-slot TeamRegion law does NOT apply (deviation recorded in CONVENTIONS "Arena terrain & environment").
- fab-link: Jonathan's choice — any pack (stylized/low-poly trees fitting §6, e.g. "stylized fantasy trees low poly")
- license-note: Jonathan to confirm game-use license at download
- fulfilled-note: (pending Jonathan — drop under Content/Fab/<Pack>/, see Content/Fab/README_DROP_ZONE.md)
- integration: TASK-092 (amended 2026-07-08 to Fab-conform)

### FAB-002 — Stylized rock pack (M4.5 arena obstacles — NEW scope)
- status: approved
- requested-by: manager (Jonathan's 2026-07-08 directive)
- for: SM_Rock_01 (+ SM_Rock_02.. if the pack has variants) (Content/Meshes/) — mirror-symmetric arena obstacles under the same law as trees (navmesh + placement blocker, projectile blocker, base-footprint-only collision). Conform in TASK-092.
- fab-link: Jonathan's choice — any pack (stylized/low-poly rocks/boulders fitting §6)
- license-note: Jonathan to confirm game-use license at download
- fulfilled-note: (pending Jonathan)
- integration: TASK-092 (amended 2026-07-08 to Fab-conform)

### FAB-003 — Stylized grass / ground material pack (M4.5 battlefield surface)
- status: approved
- requested-by: manager (Jonathan's 2026-07-08 directive)
- for: M_ArenaGround (Content/Materials/) + its textures (T_ArenaGrass_* if texture-based) — the grass battlefield surface. Donor materials/textures are duplicated/conformed INTO M_ArenaGround (target name stays the law). Optional grass-blade foliage meshes from the pack may be evaluated but are M7-tier unless trivially within the §6 perf budget. Conform in TASK-091.
- fab-link: Jonathan's choice — any pack (stylized grass ground material / hand-painted terrain textures)
- license-note: Jonathan to confirm game-use license at download
- fulfilled-note: (pending Jonathan)
- integration: TASK-091 (amended 2026-07-08 to Fab-conform)

### FAB-004 — Hill / terrain meshes (M4.5 high ground)
- status: approved
- requested-by: manager (Jonathan's 2026-07-08 directive)
- for: SM_ArenaTerrain (unified ground) OR flat base + SM_Hill_01(+variants) placed instances — whichever the pack supports (conform decision at TASK-091). MUST end up WALKABLE: complex-collision-as-simple, faces ≤35°, near-flat placeable crowns, flat-pad law respected (castle/gold-node pads + lanes at Z=0), exact X=0 mirror symmetry. Conform in TASK-091.
- fab-link: Jonathan's choice — any pack (stylized hills/terrain meshes, walkable mounds)
- license-note: Jonathan to confirm game-use license at download
- fulfilled-note: (pending Jonathan)
- integration: TASK-091 (amended 2026-07-08 to Fab-conform)
