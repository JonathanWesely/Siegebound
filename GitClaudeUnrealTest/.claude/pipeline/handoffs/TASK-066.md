# TASK-066 Handoff — Unit blockout meshes wave 2: SM_Ogre, SM_Cleric, SM_Longbowman (art-director)

Status: **COMPLETE — all three modeled, exported, imported, materialed, collision-generated, verified in-editor (dims/tris/material/thumbnails), and saved.** Blender MCP (port 9876) + UE5 MCP both live this session. **Editor was FREE (checked `IsPIERunning` → false before import; Jonathan not mid-PIE)** — no playtest disruption; I did NOT boot/close/bounce the editor and left it UP. No Git run (build-master owns commit at TASK-069). No TASKBOARD edit (orchestrator/manager owns the board).

Family style: SM_Footman lineage (GDD §6) — chunky/stylized, oversized weapons, 1-seg angle-limited bevel (50° limit so box corners chamfer but round facets stay), **blockout tier — premium art DEFERRED to M7**, NO textures (team material carries all color). Distinct silhouettes verified via in-editor asset thumbnails (all read correctly at a glance, team-Blue applied).

## Deliverables (all verified in-editor)

| Mesh | Asset path | FBX source | UE bounds X×Y×Z (units) | Height | minZ | Tris (UE LOD0) | Nanite |
|------|-----------|-----------|--------------------------|--------|------|----------------|--------|
| SM_Ogre       | `/Game/Meshes/SM_Ogre`       | `Content/RawAssets/Ogre.fbx`       | 226.7 × 98.8 × 290.0 | 290 (~280 ✓, largest) | ~0 | 1572 | Off |
| SM_Cleric     | `/Game/Meshes/SM_Cleric`     | `Content/RawAssets/Cleric.fbx`     | 83.5 × 79.0 × 182.0  | 182 (~180 ✓) | ~0 | 920  | Off |
| SM_Longbowman | `/Game/Meshes/SM_Longbowman` | `Content/RawAssets/Longbowman.fbx` | 81.8 × 57.4 × 184.0  | 184 (~185 ✓) | ~0 | 1984 | Off |

- All tri counts FAR under the 8k budget (max 1984). UE LOD0 counts match the Blender triangulation **exactly** (FACE smoothing + clean smart-project UVs → no seam-split inflation).
- **minZ ≈ 0 on all three (measured −2e-6..−6e-6) → feet-center origin.** Each mesh sits ON the floor at Z=0; object origin (0,0,0) = feet center of the footprint.
- **Ogre is unambiguously the roster's largest unit:** 290 tall (vs next-tallest Pikeman 190, Cavalry 208) AND 226.7 wide (vs next-widest Knight 162). It towers and out-bulks everything — the "HUGE win-condition brute" read is met.
- Roster height ladder now clean at a glance: Archer 180 < **Cleric 182** < **Longbowman 184** < Pikeman 190 (Longbowman trimmed from a first 189.5 pass so it reads distinctly below Pikeman).

Absolute FBX paths:
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\{Ogre,Cleric,Longbowman}.fbx`

## Longbowman choice — CUSTOM mesh (NOT reused SM_Archer)
Spec allowed reusing SM_Archer scaled if time-constrained; I was not, so I modeled a **distinct custom mesh** for a cleaner "long-range archer" read. It is a taller (184 vs Archer 180), leaner figure holding a **large near-full-height vertical longbow arc + straight bowstring on the extended left (−X) arm** (the dominant silhouette element — a longbow nearly as tall as the archer), with a **quiver + 3 arrow shafts/fletching over the right shoulder (+Y back)**, right hand drawing to the chin, and a pointed archer cap. The oversized vertical bow distinguishes it from SM_Archer's shorter segmented bow.
- **TASK-062 should set `BP_Unit_Longbowman` VisualMesh = `/Game/Meshes/SM_Longbowman`** (custom mesh exists — do NOT point it at SM_Archer).

## Material (each mesh)
- Exactly **ONE** material slot, slot name **`MI_TeamColor_Blue`** (from the Blender material-slot name in the FBX), bound on **slot 0** to **`/Game/Materials/Instances/MI_TeamColor_Blue`**. Verified via `get_material` — returns `/Game/Materials/Instances/MI_TeamColor_Blue.MI_TeamColor_Blue` on all three.
- Imported with `import_materials=False` / `import_textures=False` → the importer created **NO** new material or texture assets. `find_assets` confirms `/Game/Meshes/` gained ONLY the three `SM_` meshes and `/Game/RawAssets/` holds ZERO UE assets (just the raw `.fbx` on disk).
- Per the M3 team-color contract: BeginPlay recolors to `MI_TeamColor_<Team>` from the actor's `Team`; the Blue instance authored here is the design-time placeholder, so the bot's Red units recolor at spawn.

## Silhouette / read (blockout intent — thumbnails confirm)
- **SM_Ogre** — win-condition siege brute: massive rounded shoulders, big protruding belly, thick short legs, small head sunk between the shoulders (menace), and a **huge knobby club raised overhead in the right (+X) fist** (the club is the tallest element at ~290). Reads HUGE and brutish next to a Footman.
- **SM_Cleric** — support/healer: **floor-length flared robe (no visible legs)**, hooded/cowled head, and a **tall staff held at the +X side topped with a holy cross** (vertical + horizontal bar + centre boss). Clearly a caster/support, NOT a fighter.
- **SM_Longbowman** — long-range archer: tall lean figure, **big vertical longbow arc + string on the extended left arm**, quiver + arrows over the shoulder, pointed cap. Reads "archer / reach" and taller than the base Archer.

## Orientation, origin, integration notes
- Modeled facing **Blender −Y**, exported with `axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale=True` (metric, 1 m = 100 UE units), `mesh_smooth_type='FACE'` — **identical convention to SM_Footman / TASK-065 wave-1 / TASK-037**. Result: each imports **facing +Y in UE asset space**.
- **Front of each figure = the −Y (min-Y) side** in UE asset space (Ogre belly/brow, Cleric hood face + staff lean, Longbowman bow + cap all toward −Y). The staff/cross is on +X; the bow is on −X (asset space, BEFORE the BP yaw).
- Per the established fix, the per-unit BP should give **VisualMesh a relative yaw of −90°** so the figure faces the actor's forward +X (same fix `BP_Unit_Footman`/Knight/Archer/wave-1 use). Weapon sides above are stated in **asset space BEFORE** that −90 yaw.
- Feet-center origin means the team vertical gradient auto-fits bounds (darkest at feet, brightest at head) with no tuning.
- Capsule-friendly: gameplay collision is the pawn capsule (sized per unit at integration), NOT this mesh collision. **Ogre's 226.7u width and Longbowman's tall bow/quiver are visual overhang** — size the capsule to the Ogre torso and the Longbowman body, not the club/bow extents.

## Collision
- Each mesh: importer auto-collision replaced with **4-hull simple convex collision** (`generate_convex_collisions hull_count=4, max_hull_verts=12`) for sane placement-ghost / level use. Matches the TASK-065/037 recipe. Hulls include weapon/staff/bow overhang; if a BP enables mesh collision and the overhang is undesirable, swap to a body-only capsule at integration (no re-export needed).

## Warnings / cleanliness
- **Zero import warnings / errors.** Cross-checked the UE output log: no entry mentions Ogre/Cleric/Longbowman with a warning/error, and no smoothing-group / missing-UV / degenerate-mesh warnings fired. FACE smoothing groups written on export (no missing-smoothing warning); UV channel 0 present via smart-project (no missing-UV warning). (The only warnings in the log are pre-existing and unrelated — engine-boot DLL loads, SDK setup, an old IMC_Hero input-modifier warning, and WBP_MainMenu/BP_MenuGameMode compile warnings from other agents' menu tasks.)
- No auto-import artifacts: no `/Game/RawAssets/` content assets were created (only the raw `.fbx` files live under `Content/RawAssets/`).
- All three `.uasset` saved to disk (`Content/Meshes/SM_{Ogre,Cleric,Longbowman}.uasset`) alongside their FBX in `Content/RawAssets/`. **Left uncommitted for build-master to stage/commit at TASK-069** (UE Git provider may auto-stage — left as-is). Not placed in any level. Nanite Off on all (blockout low-poly).

## Downstream contract (character-exact, per CONVENTIONS §Per-card visual assets)
`SM_Ogre`, `SM_Cleric`, `SM_Longbowman` are the `SM_<CardID>` visual meshes resolved by string for placement ghosts and referenced by the per-card unit BPs (TASK-062: `BP_Unit_Ogre` / `BP_Unit_Cleric` / `BP_Unit_Longbowman`, VisualMesh component, −90 yaw). Longbowman uses its OWN custom mesh (not SM_Archer). M7 replaces these blockouts with skeletal meshes at the same path contract — treat none of this as final art.
