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

### FAB-005 — Stylized RTS Buildings & Props Pack (M7.5 building-fleet upgrade)
- status: requested
- requested-by: art-director (TASK-206)
- for: Building-fleet quality upgrade via the Stage-2 conform + same-path `SM_` swap lane (TASK-208 first wave). Research-identified roster mapping — near-exact Siegebound coverage: **Town Center / Fortress → SM_Castle tier** (Fortress also a castle-crumble stage-mesh candidate, TASK-157 chain), **Barracks → SM_Barracks**, **Watch Tower → SM_ArrowTower tier** (donor-adapt candidates for SM_BombTower / SM_BallistaTower), **wall segments → SM_Wall** (closes the Wall gap — the TRELLIS retry-listed asset), **Blacksmith + props → arena/base dressing** (TASK-178-style), **construction meshes → build-up/placement states**. Roster-replacing conforms KEEP the two-slot `[TeamRegion, <AssetName>PBR]` law; pure environment props are exempt (M6.5 precedent, M7.5 manager decision 6).
- fab-link: https://www.fab.com/listings/a4b43ae5-e442-4d51-93f2-fea8d77e9f37
- license-note: MUST permit game use — expected Fab Standard License (royalty-free use in shipped interactive products). Jonathan verifies the EXACT license tier shown on the listing AND the price at checkout in the Epic Launcher — agents cannot browse Fab (it blocks bots), so tier/price are unverifiable agent-side. Budget context: ~$100–200 total approved across FAB-005 + FAB-006.
- fulfilled-note: (pending Jonathan — TASK-207: Epic Launcher → drop at `Content/Fab/StylizedRTSBuildings/`, READ-ONLY donor quarantine)
- integration: TASK-208 (planned — inventory + conform plan + first-wave swaps)

### FAB-006 — Rigged stylized medieval unit pack (M7.5 humanoid-unit upgrade — ONE pack, Jonathan picks in the launcher)
- status: requested
- requested-by: art-director (TASK-206)
- for: Humanoid unit quality — AI mesh generation's weakest area. Donor skeletal meshes for the unit roster (Footman/Knight tier first), conformed via Stage-2 + retarget into the existing `SK_<CardID>` / `A_<CardID>_<Action>` same-path contract (TASK-209; IK_/RTG_ law, CONVENTIONS "Meshy second engine (M7.5)"). Researched candidates — Jonathan chooses ONE in the launcher (exact listings there; agents cannot browse Fab):
  1. **TAB Medieval Knights** — RECOMMENDED PICK. Justification: rigged to the **Epic skeleton** — the cleanest UE 5.8 retarget path of the four (near-zero IK Retargeter friction into our SiegeBiped chain), solid stylized-medieval fit vs the §6 bar; tradeoff: knights-focused, narrower roster coverage than a full RTS unit set.
  2. **Toon RTS Units** — customizable low-poly fantasy units; BEST roster breadth (multiple unit archetypes) and strong stylized fit; tradeoff: custom rig → full retarget-chain setup per the TASK-203 spike pattern.
  3. **Animated Stylized Knight** — 74 animations included (richest clip library — useful donor clips for the TASK-205 fleet lane); tradeoff: single-character coverage.
  4. **Stylized Warrior Pack** (modular parts) — mix-and-match parts could cover several roster units from one purchase; tradeoff: rig/skeleton type unverified agent-side — Jonathan checks rigging claims on the listing before buying.
- fab-link: exact search terms in the Epic Launcher / fab.com: "TAB Medieval Knights", "Toon RTS Units", "Animated Stylized Knight", "Stylized Warrior Pack" (stylized + rigged + medieval/fantasy filters)
- license-note: MUST permit game use — expected Fab Standard License (royalty-free in shipped interactive products). Jonathan verifies the EXACT license tier AND price on the chosen listing at checkout (agents cannot browse Fab / prices unverifiable agent-side). Shared budget with FAB-005: ~$100–200 total approved.
- fulfilled-note: (pending Jonathan — TASK-207: Epic Launcher → drop at `Content/Fab/<UnitPack>/` (name per chosen pack, e.g. `Content/Fab/TABMedievalKnights/`), READ-ONLY donor quarantine)
- integration: TASK-209 (planned — unit conform + retarget wave)
- **SCOPE FENCE (added 2026-07-26): this pack is HUMANOID-ONLY and does NOT cover the Cavalry quadruped — see FAB-007.**

### FAB-007 — Rigged horse / mount / QUADRUPED source (Cavalry — TASK-233's missing source)
- status: **🛑 PARKED — ON HOLD BY JONATHAN (2026-07-27): *"Hold off on Cavalry's quadruped source for now."*** ← was: `requested`.
  **The request stays AUTHORED and complete below — it is not withdrawn, not rejected, and must not be deleted or re-researched.** It is
  simply NOT to be purchased, NOT to be raised at the TASK-207 checkout, and NOT to be acted on by any agent until Jonathan lifts the hold.
  **Do NOT dispatch its integration task (`TASK-233`) — that task is PARKED in lockstep on the board.** Nothing is blocked by this hold:
  Cavalry ships and plays today (`aa00826`), the mesh reads bright and correct, and its procedural anims remain live. The only cost of the
  hold is that Cavalry's gait stays stiff.
- requested-by: manager (2026-07-26, from the TASK-319-verify in-engine finding; the art-director raised the gap mid-batch and routed it here rather than write this file concurrently)
- for: **`SK_Cavalry` locomotion** — a rigged QUADRUPED skeleton + gait clip set (`idle / walk / attack / death`) that `A_Cavalry_{Idle,Walk,Attack,Death}` can be retargeted from, closing the last un-animated unit in the fleet (TASK-233). **NOTE THE SHAPE OF THE ASK: this is NOT primarily a mesh request.** The Meshy-remastered `SM_Cavalry` / `SK_Cavalry` (commit `aa00826`) look GOOD — bright and saturated, and the historical "rider-fit rig warps the horse" defect is **GONE** (rigged bind-pose bounds match the static mesh to the centimetre). What is missing is a quadruped RIG + CLIPS. A pack that also ships a rigged mount MESH is welcome (it would swap in same-path per the Fleet-remaster overwrite law, never delete+recreate), but the load-bearing deliverable is the skeleton + clips.
- evidence-of-need (in-engine, TASK-319-verify, `aa00826`): under `*_Walk` the Cavalry body is **near-rigid while biped leg bones flail** — head / `spine_02` / `upperarm_r` travel **4.2 / 4.2 / 5.5 cm** vs Pikeman's **12.7 / 12.0 / 14.3**, while `foot_r` swings **75.1 vs 49.6**. Rig weighting confirms why: `pelvis` + both `thigh` bones own **50.4%** of all weight (healthy fleet units ≈32%), `thigh_l` alone spans Z 0.1→173.9, drift 12.8% of height vs 8.3%. Meshy has NO quadruped clip library (recorded at TASK-233), so no in-house lane closes this.
- **WHY FAB-006 DOES NOT COVER IT (the reason this entry exists):** FAB-006 is explicitly a **HUMANOID** unit pack, and before today FAB-REQUESTS.md had **no horse/mount/quadruped entry at all** — so TASK-233 was unsourced while appearing to be covered. Caveat kept from TASK-233: candidate #1 *TAB Medieval Knights* was flagged as *possibly* including mounted units. Jonathan verifies that on the listing at checkout — **if the chosen FAB-006 pack does ship a rigged mount, close FAB-007 as `rejected — covered by FAB-006` at zero extra cost.**
- **→ PURCHASE-SCOPE IMPACT ON A PENDING JONATHAN DECISION (TASK-207) — ⚠️ SUPERSEDED BY THE 2026-07-27 HOLD: he has ALREADY chosen option (b), defer. Do NOT re-raise this at the TASK-207 checkout; FAB-005 + FAB-006 proceed on their own merits.** Historical text: TASK-207 was scoped to **FAB-005 + FAB-006 only** (~$100–200 approved). **Neither fixes Cavalry.** FAB-007 is a THIRD line item on that same checkout — Jonathan decides at TASK-207 whether to (a) add it to the purchase, (b) defer it (Cavalry keeps its current stiff-but-non-deforming gait; nothing is blocked), or (c) confirm the FAB-006 pick covers mounts and reject FAB-007. Non-blocking either way — Cavalry is playable today.
- fab-link: Jonathan's choice in the Epic Launcher / fab.com — search terms: "rigged horse", "stylized horse animated", "horse animset", "quadruped animation pack", "medieval mounted knight rigged", "animalia horse". Prefer: stylized/low-poly fit vs the §6 bar, **rig + gait clips included**, and (bonus) an Epic-skeleton-adjacent or clean-hierarchy rig for the least retarget friction. Agents cannot browse Fab (it blocks bots) — exact listings, tiers and prices are Jonathan-side only.
- license-note: MUST permit game use — expected Fab Standard License (royalty-free in shipped interactive products). Jonathan verifies the EXACT license tier AND price on the chosen listing at checkout.
- fulfilled-note: (pending Jonathan — drop at `Content/Fab/<MountPack>/`, READ-ONLY donor quarantine)
- integration: **TASK-233** (Cavalry quadruped anim source — the existing backlog task; its `blocked-by: source decision` line resolves to this entry once fulfilled). Conform + retarget follows the sanctioned Blender lane `Tools/ArtPipeline/retarget_meshy_to_siegebiped.py`-equivalent for a quadruped chain, NOT the UE IK-Retargeter export door (UE-5.8 root-only export defect, CONVENTIONS TASK-229 ruling); output OVERWRITES `/Game/Characters/Anims/A_Cavalry_*` same-path, on an explicit PIE pass gate only.
