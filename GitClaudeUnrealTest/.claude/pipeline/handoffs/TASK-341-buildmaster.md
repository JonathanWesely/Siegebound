# TASK-341 — [OGRE-remake-int] Same-path SM+SK import + LOD reapply + sampler sweep + Simulate verify + ONE commit — build-master handoff

**Date:** 2026-07-27 · **Branch:** main · **Editor:** exclusive session, relaunched twice (see incident) · **Verdict: PASS — imported, hard-gated, Simulate-verified, committed. NO push.**

Sources = TASK-340 staging (all sizes byte-matched the artist's table before import). Recipe executed verbatim from `handoffs/TASK-340-artist.md`.

---

## Session incident (recorded honestly; no data lost)

Mid-task, Jonathan opened a play session in the first relaunched editor ("my bad, I thought you were done"): a PIE on `L_MainMenu` started at 18:06:26 local, seconds before my texture step ran. Consequences and resolution:
- The three texture Interchange imports RAN during his PIE (imports don't carry the editor-scripting guard). Every guarded readback call correctly refused ("Editor is currently in a play mode" — misdiagnosed for ~10 min as a registry desync). The texture `save_packages` (18:08:08) succeeded and was **re-verified from a clean fresh session afterwards**: registry sees all three, flags correct, import source = the new PNGs, not dirty. Nothing trusted from the raced window without re-verification.
- Jonathan closed that editor (18:09:37), briefly opened his own, closed it again; orchestrator relayed both changes. Disk audit showed the only Content writes in the window were my three texture saves. I relaunched clean (18:11) and did ALL mesh work + verification in that one exclusive session.
- **Simulate/PIE was verified STOPPED before the SM and SK imports and every save in the clean session.** The texture imports are the one exception (his PIE), covered by the clean-session re-verification above.

## What was imported (same-path overwrite, never delete+recreate)

| Asset | Source | Result |
|---|---|---|
| `/Game/Textures/T_Ogre_D` | `RawAssets/Textures/Ogre/T_Ogre_D.png` (1,008,337 B) | 1024², **sRGB ON**, TC_DEFAULT, saved, import-source readback = new PNG |
| `/Game/Textures/T_Ogre_N` | `…/T_Ogre_N.png` (808,411 B) | 1024², LINEAR, TC_NORMALMAP, saved |
| `/Game/Textures/T_Ogre_ORM` | `…/T_Ogre_ORM.png` (972,488 B) | 1024², LINEAR, TC_MASKS, saved |
| `/Game/Meshes/SM_Ogre` | `RawAssets/Ogre.fbx` (615,228 B) | same-path reimport in live editor; Nanite OFF; `lod_group=LargeProp`; 4 convex hulls; slots `[0 TeamRegion → MI_TeamColor_Blue, 1 OgrePBR → MI_Ogre_PBR]`; referencers before == after == `[BP_Unit_Ogre]` |
| `/Game/Characters/SK_Ogre` | `RawAssets/Characters/Ogre.fbx` (852,460 B) | same-path reimport **bound to the EXISTING `SK_Footman_Skeleton`**; slots as above; physics_asset null (unchanged) |
| `/Game/Materials/Instances/MI_Ogre_PBR` | — | **untouched by design** — references the texture OBJECTS; `git diff` shows zero change |

- **Texture-skip trap honored:** the three textures were imported EXPLICITLY as their own step with size/timestamp readback (the `_ensure_textures_and_mi` early-return never ran).
- **`A_Ogre_{Idle,Walk,Attack,Death}` + ABP untouched** — all four verified bound to `SK_Footman_Skeleton` before AND after the SK overwrite.

## HARD LOD GATE (the SM_MilitiaMob signature) — PASS

| | SM_Ogre after | Expected |
|---|---|---|
| lod_count / lod_group | **4 / LargeProp** | 4 |
| tris per LOD | **15000 / 7500 / 3750 / 1874** | ≈14999 / 7500 / 3750 / 1874 (+1 tri = import triangulation noise; Castle precedent +5) |
| LOD0 verts | **17202** (render verts; ratio 1.15 vs tris — normal UV/normal split) | NOT ≈44,997 (tris×3 unweld) — **unweld signature ABSENT** |
| 0-triangle LODs | **none** | none |
| Bounds / min-Z | **261.25 × 164.86 × 289.48 / +0.012** | artist's exact recorded values |

SK_Ogre: **SKELETON GATE PASS** — bound skeleton readback `SK_Footman_Skeleton`; skeleton census of `/Game/Characters` IDENTICAL before/after (`SK_Footman_Skeleton` + the pre-existing hero-donor `Mannequins/Meshes/SK_Mannequin`) — **no stray skeleton created**. SK LOD chain regenerated per `Ogre.lod.json` via the TASK-297 recipe (`num_of_triangles_percentage` 0.5/0.2, screens 0.4/0.15, transient LODSettings restored to None): **`lod_count == 3`, verts 17395 / 10719 / 5836** (fractions 0.62/0.34 — consistent with the 0.5/0.2 triangle targets; regen was idempotent over the import-preserved chain). URO lives in C++ (`SummonedUnit.cpp:155-156`) — nothing set on the asset, and it was OBSERVED live (see below).

## SAMPLER-TYPE sweep — PASS

- Material referencers of `T_Ogre_{D,N,ORM}`: **exactly one — `MI_Ogre_PBR`** (parent `M_AssetPBR`). **No master node-defaults these textures** (the trap precondition is absent). Master loads clean.
- `Failed to compile Material` grep over the full fresh-session log (which cold-loaded the new textures + MI + master): **0 hits** — also 0 at end of session.

## Simulate verify on L_Arena (Simulate, never PIE-in-viewport) — observations

Method: two `BP_Unit_Ogre` placed in the editor world (TASK-310 method), Red's `Team=Red` set per-instance, Simulate started via MCP, live match running alongside (26 units). Readbacks on the live PIE-world actors:

- **(fence, ruling 7 — observed only, nothing authored):** `SkeletalVisualMesh` relative rotation **(0, −90, 0)** and relative Z **−145.01** on BOTH; capsule half-height READ **145.0** (not assumed). Feet float **2.15 cm** above ground (fleet band 2.1–2.4). In the tight capture the mesh presents its FACE on actor-forward — the facing contract holds visually.
- **(a) Brightness/concept:** the Ogre reads clearly BRIGHTER and cleaner than the shipped muddy grey-brown — compare `TASK-341-verify-Ogre-{tight,threequarter,wide,feet}.png` against the committed before-shots `TASK-316-verify-Ogre-*.png` (same Simulate/L_Arena lane; hero mark, −35° sun-azimuth camera). Horns, pink facial detail, tusks, pauldron, skull-belt, maul and tattered skirt all legible. Under the warm arena sun the mossy green of the flat-lit previews (`Cache/Ogre/previews/preview_front.png`) reads pale khaki-tan at distance — an observation for Jonathan's eye, not a conclusion. The deep hunch in all shots is the PRESERVED idle animation (identical in the before-shots), not an import artifact.
- **(b) Team recolour:** slot0 readback = `MI_TeamColor_Blue` on the Blue unit, `MI_TeamColor_Red` on the Red unit (BeginPlay recolor live on both). The BLUE band reads clearly in the captures; the RED band is applied but reads SUBTLE at gameplay angles in the hunched idle (`TASK-341-verify-OgreRed-{tight,closeup}.png`) — deep red on brown leather under warm sun. Flagged for the WATCH (below).
- **(c) Anims tick live:** bone-delta sampling — with the camera off them the pose is frozen (that is `OnlyTickPoseWhenRendered` + URO from TASK-285 C++ working as designed); with the camera on them `hand_r` swings ~39 cm between samples. Idle plays on the shared skeleton; anim instance = `ABP_Footman_C` (the shared ABP the CardID-composed resolve produces for the Ogre — pre-existing contract, unchanged).
- **(d) Log sweep:** ensure 0 · Fatal 0 · `LogOutputDevice: Error` 0 · `Failed to compile Material` 0 · Accessed None **2** — both a single instant at the StartPIE boundary, from `ABP_Footman_C` on a `SkeletalMeshActor_0` in `/Engine/Transient.World_3` (an asset-PREVIEW world, not the Simulate world): the shared ABP's `TryGetPawnOwner` has no validity branch, so any non-pawn preview actor trips it. **Zero AccessedNone from the game world.** Pre-existing ABP authoring quirk, ABP untouched by this task — residue for the manager.
- Simulate stopped cleanly (7 s pacing honored); both VERIFYCAP actors deleted from the editor world; **`L_Arena` NEVER saved** (left dirty in-editor as found).

## Commit

ONE commit on main, explicit pathspecs, NO push, `git diff --stat` verified clean of anything foreign before staging:
`SM_Ogre.uasset` · `SK_Ogre.uasset` · `T_Ogre_{D,N,ORM}.uasset` · raw `Ogre.fbx` (SM) · `Characters/Ogre.fbx` (SK) · `Textures/Ogre/T_Ogre_{D,N,ORM}.png` · `pipeline_manifest.json` (Ogre `albedo_delight` override + team z-band, the new law) · TASKBOARD.md · CONVENTIONS.md (the per-asset-override law line) · `TASK-340-artist.md` · this handoff · 6 verify PNGs. (`MI_Ogre_PBR` byte-unchanged — nothing to commit. `Ogre.lod.json` regenerated byte-identical — no diff.)
`git reset --hard` / `git clean -fd` never used; one git command per shell call.

## WATCH (ruling 9 — binding)

**Jonathan's playtest eye is the FINAL authority on attempt #3.** The numbers passed every floor (UV-norm 0.2983 ≥ 0.2536 at 1.18×, retention 0.9399, team region 2.36%), and the in-engine read is clearly brighter than shipped — but if it STILL reads dark to Jonathan in play, the numbers go BACK TO THE MANAGER for adjudication. **No fourth silent re-run.**

## Residue / follow-ups for the manager

1. **Red team band legibility on the Ogre:** applied and readback-proven, but visually subtle at gameplay angles in the hunched idle (deep red on brown leather; the band geography partly folds under the head silhouette when hunched). Blue reads fine. Candidate levers if Jonathan flags it: brighten `MI_TeamColor_Red`, or band geography that survives the hunch — manager's call, not authored here.
2. **`ABP_Footman` `TryGetPawnOwner` AccessedNone in preview scenes** (2 hits, transient world only): pre-existing; a validity branch in the ABP event graph would silence it. Cosmetic-log-noise tier.
3. The deep idle hunch hides much of the silhouette (tusks/maul) at RTS camera angles — same as the shipped state (before-shots identical in pose). If the Ogre still reads "blobby" in play, the lever is the idle anim, not the mesh/textures.
4. Editor left RUNNING with MCP up, L_Arena dirty-unsaved (as found), remote-exec + throttle-off flips in-memory only (revert on restart).
