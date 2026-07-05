# TASK-065 Handoff — Unit blockout meshes wave 1: SM_Cavalry, SM_Pikeman, SM_Sapper, SM_MilitiaMob (art-director)

Status: **COMPLETE — all four modeled, exported, imported, materialed, collision-generated, verified in-editor, and saved.** Blender MCP (port 9876) + UE5 MCP both live this session. **Editor was FREE (PIE not running)** when I imported — no playtest disruption; I did not boot/close/bounce the editor. No Git run (build-master owns commit at TASK-069). No TASKBOARD edit (orchestrator/manager owns the board).

Family style: SM_Footman lineage (GDD §6) — chunky/stylized, oversized hands/weapons, 1-seg angle-limited bevel (50° limit so box corners chamfer but round facets stay), **blockout tier — premium art DEFERRED to M7**, NO textures (team material carries all color). Distinct silhouettes verified via Blender Workbench ortho renders (front + 3/4) AND in-editor asset thumbnails.

## Deliverables (all verified in-editor)

| Mesh | Asset path | FBX source | UE bounds X×Y×Z (units) | Height | minZ | Tris (UE LOD0) | Nanite |
|------|-----------|-----------|--------------------------|--------|------|----------------|--------|
| SM_Cavalry    | `/Game/Meshes/SM_Cavalry`    | `Content/RawAssets/Cavalry.fbx`    | 75.5 × 254.6 × 208.0 | 208 (~200 ✓) | ~0 | 804 | Off |
| SM_Pikeman    | `/Game/Meshes/SM_Pikeman`    | `Content/RawAssets/Pikeman.fbx`    | 76.8 × 60.3 × 190.0  | 190 (~185 ✓) | ~0 | 532 | Off |
| SM_Sapper     | `/Game/Meshes/SM_Sapper`     | `Content/RawAssets/Sapper.fbx`     | 74.0 × 98.0 × 169.0  | 169 (~170 ✓) | ~0 | 620 | Off |
| SM_MilitiaMob | `/Game/Meshes/SM_MilitiaMob` | `Content/RawAssets/MilitiaMob.fbx` | 74.5 × 34.3 × 149.0  | 149 (~150 ✓) | ~0 | 452 | Off |

- All tri counts FAR under the 8k budget (max 804). UE LOD0 counts match the Blender triangulation exactly (FACE smoothing + clean UVs → no seam-split inflation).
- **minZ ≈ 0 on all four → feet-center origin.** Each mesh sits ON the floor when placed at Z=0. Object origin is at (0,0,0) = feet center of the footprint.
- **Height ladder reads correctly at a glance:** MilitiaMob (149) < Sapper (169) < Pikeman (190) < Cavalry (208). Cavalry is also by far the longest footprint (254.6 in Y) = the "fast heavy" mounted read.

Absolute FBX paths:
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\{Cavalry,Pikeman,Sapper,MilitiaMob}.fbx`

## MilitiaMob choice — CUSTOM mesh (NOT reused SM_Footman)
I modeled a **distinct custom mesh** rather than reusing SM_Footman scaled. It is a small (149u), thin, un-armored, slightly-hunched peasant with a crude short knobby club raised in one hand — clearly the smallest/weakest silhouette in the roster (thin 34.3u depth vs Sapper's 98u). **TASK-062 should set `BP_Unit_MilitiaMob` VisualMesh = `/Game/Meshes/SM_MilitiaMob`** (do NOT point it at SM_Footman).

## Material (each mesh)
- Exactly **ONE** material slot, slot name **`MI_TeamColor_Blue`** (from the Blender material slot name in the FBX), bound to **`/Game/Materials/Instances/MI_TeamColor_Blue`** (slot 0). Verified via `get_material` — returns `/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue` on all four.
- Imported with `import_materials=False` / `import_textures=False` → the importer created **NO** new material or texture assets; `/Game/Meshes/` gained only the four `SM_` meshes, nothing else.
- Per the M3 team-color contract: BeginPlay recolors to `MI_TeamColor_<Team>` from the actor's `Team`; the Blue instance authored here is the design-time placeholder and the Red bot units will recolor at spawn.

## Silhouette / read (blockout intent)
- **SM_Cavalry** — mounted charger: a horse (elongated body along the facing axis, 4 legs, forward muzzle, tail) with a rider seated on top and a **long lance couched forward** (tip well ahead of the horse's nose). Biggest + longest footprint → "fast heavy." In-editor thumbnail confirms the mount+rider+lance read.
- **SM_Pikeman** — anti-tank reach: normal-tall soldier holding a **tall near-vertical pike with a pointed spearhead towering above the head**, gripped by two oversized hands. The long thin weapon line above the head = "reach."
- **SM_Sapper** — expendable demolisher: hunched figure **hugging a large round bomb at the belly (front) with a lit fuse + spark on top**. The big round mass in front is the dominant read. In-editor thumbnail confirms the bomb imported round and the figure reads as a bomb-carrier.
- **SM_MilitiaMob** — weak swarm: small, thin, un-armored, hunched peasant with a small raised club. Distinctly the smallest and thinnest → "weak / spam unit."

## Orientation, origin, integration notes
- Modeled facing **Blender −Y**, exported with `axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale=True` (metric, 1 m = 100 UE units) — **identical convention to SM_Footman / SM_Knight** (handoffs TASK-014, TASK-037). Result: each imports **facing +Y in UE asset space**.
- **Front of each figure = the −Y (min-Y) side** in UE asset space: Cavalry muzzle+lance, Pikeman pike lean, Sapper bomb+fuse, Militia club all sit toward −Y.
- Per the established fix, the per-unit BP should give **VisualMesh a relative yaw of −90°** so the figure faces the actor's forward +X (same fix `BP_Unit_Footman`/Knight/Archer use). Weapon sides above are stated in **asset space BEFORE** that −90 yaw.
- Feet-center origin means the team vertical gradient auto-fits bounds (darkest at feet, brightest at head) with no tuning.
- Capsule-friendly: gameplay collision is the pawn capsule (sized per unit at integration), NOT this mesh collision. Cavalry's 254.6u Y length is visual overhang (horse + lance) — size the capsule to the rider/horse torso, not the lance tip.

## Collision
- Each mesh: importer auto-collision replaced with **4-hull simple convex collision** (`generate_convex_collisions hull_count=4, max_hull_verts=12`) for sane placement-ghost / level use. Matches the TASK-037 "any simple collision" intent. Hulls include weapon/mount overhang; if a BP enables mesh collision and the overhang is undesirable, swap to a body-only capsule at integration (no re-export needed).

## Warnings / cleanliness
- **Zero import warnings / errors.** UE output log for all four: `Triangulating` → `Building static mesh [0.02s]` → saved → `AssetCheck: Validating asset` with no Warning/Error lines. FACE smoothing groups written on export (no missing-smoothing warning); UV channel 0 present via smart-project (no missing-UV warning).
- No auto-import artifacts: no `/Game/RawAssets/` content folder was created (only the raw `.fbx` files live under `Content/RawAssets/`).
- All four `.uasset` saved to disk (`Content/Meshes/SM_{Cavalry,Pikeman,Sapper,MilitiaMob}.uasset`) alongside their FBX in `Content/RawAssets/`. **Left uncommitted for build-master to stage/commit at TASK-069** (UE Git provider may auto-stage — left as-is). Not placed in any level. Nanite Off on all (blockout low-poly).

## Downstream contract (character-exact, per CONVENTIONS §Per-card visual assets)
`SM_Cavalry`, `SM_Pikeman`, `SM_Sapper`, `SM_MilitiaMob` are the `SM_<CardID>` visual meshes resolved by string for placement ghosts and referenced by the per-card unit BPs (TASK-062: `BP_Unit_Cavalry` / `BP_Unit_Pikeman` / `BP_Unit_Sapper` / `BP_Unit_MilitiaMob`, VisualMesh component, −90 yaw). M7 replaces these blockouts with skeletal meshes at the same path contract — treat none of this as final art.
