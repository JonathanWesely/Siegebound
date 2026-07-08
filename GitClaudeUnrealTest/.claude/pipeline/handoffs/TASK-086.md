# TASK-086 Handoff — SM_Footman pilot: COMPLETE (import playbook for TASK-087)

- **From:** art-director
- **Date:** 2026-07-07/08 (final)
- **Status:** ready-for-integration (orchestrator owns the board flip). All acceptance criteria met: SM_Footman IS the textured mesh at the unchanged path; two slots named/ordered per law; M_AssetPBR + MI_Footman_PBR exist; overwrite verdict below.

## ⚖️ SAME-PATH-OVERWRITE VERDICT (ruling 7 — the load-bearing answer for TASK-087)

**Validated mechanism: human Content-Browser Reimport click (Jonathan), one click per mesh.**

1. **MCP direct import onto an existing SM path: REFUSED.** `StaticMeshTools.import_file(folder_path=/Game/Meshes, asset_name=SM_Footman)` errors verbatim `import_asset: SM_Footman at /Game/Meshes already exists` (TASK-031/085 precedent confirmed on the current server).
2. **Console `Obj Reimport`: UNAVAILABLE.** The MCP server exposes NO console/exec surface (EditorAppToolset = cvar search only; ProgrammaticToolset sandbox = registered tools only, no `unreal` module; AgentSkillToolset = doc-skill CRUD; no reimport tool anywhere; no top-level tools discoverable).
3. **Human fallback: WORKS PERFECTLY.** Jonathan right-clicked SM_Footman → Reimport. The stored FBX source path resolved with NO file prompt (source = Content/RawAssets/Footman.fbx, which Stage 2 overwrote in place — the same-path design working as intended). Reimport-in-place: same UObject/package, `BP_Unit_Footman` reference intact by construction, zero import warnings. He first dismissed the auto-reimport toast (Don't Import — it listed 5 source-file changes) then did the surgical reimport; **no blanket import ran** (verified: /Game/RawAssets registers as a folder but contains ZERO assets — it is just the on-disk Content/RawAssets source dir mirrored in the content browser; project-wide "Footman" asset search returns exactly the expected 7, no strays).
4. **Slot handling on reimport:** the reimport pulled slot names from the FBX materials — `["TeamRegion","FootmanPBR"]`, correct names AND order, no shuffle — and left them unassigned; MI assignment happens post-reimport via `StaticMeshTools.set_material` by SLOT NAME (order-independent, safe).
5. **TASK-087 procedure per mesh (Archer, Castle):** stage everything (Stages 1–2, textures, MI) → escalate ONE batched 🚨 Blockers ask for BOTH meshes → Jonathan: dismiss any auto-reimport toast with Don't Import, then right-click SM_Archer → Reimport and SM_Castle → Reimport → agent finishes slots/collision/verification. Pre-select the assets in his Content Browser (`SetContentBrowserPath` + `SelectAssets`) to make it trivial.
6. **Tooling-gap flag for the manager (pre-M7):** 16 more meshes = 16 human clicks unless the MCP server gains a reimport or console-exec tool. Recommend filing before the M7 batch.

## Stage 1 — GENERATE: SUCCESS (after one tooling QA loop)

- **Blocker found+fixed (QA loop):** gradio_client 2.5.0 renamed `Client(hf_token=)`→`Client(token=)`; crashed exit 1 pre-network. Gameplay-programmer fixed (trellis_generate.py:460) + added an offline signature assert to `--check`; re-QA PASS (handoffs/TASK-082.md "QA-loop 2").
- **Live run:** `uv run trellis_generate.py Footman` — **seed 0, accepted first try, no re-rolls**, 87 s end-to-end on the HF PRO queue. `Cache/Footman/trellis_raw.glb` 20,380,016 bytes.
- **TLS: NO SSL_CERT_FILE needed** — Norton exclusions proven on both `--check` and the authenticated GPU path. TASK-087: run bare first; cert-bundle fallback (qa/TASK-082-report.md) never needed here.
- **Concept warn (benign):** 1408×768 non-square accepted; square crops are optional polish.
- **Raw eyeball (mandatory step):** headless Workbench turntable → Cache/Footman/raw_inspect/. Intact anatomy; 491,138 tris dense; 2× 2048² WebP textures; **facing already -Y ⇒ `pre_rotate_z_deg: 0` correct. TASK-087: verify facing per asset the same way before trusting 0.**

## Stage 2 — REFINE: SUCCESS (two runs — selector tune at the eyeball gate)

`blender.exe --background --factory-startup --python-exit-code 1 --python Tools/ArtPipeline/refine_trellis_glb.py -- --card-id Footman` (~19 s/run).

- **Run-1 gate finding (why the eyeball gate exists):** the blockout-era `shield_band` selector striped the SPEAR arm/shaft/skirt — live mesh has spear at -X (thin full-height column, x_norm 0–0.12), shield at +X, head at x_norm 0.28–0.55 (numerically verified from the FBX). Shield heraldry was untouched — keep it off the team region.
- **Selector tune (art-director authority per manifest `_doc` "STARTING GUESSES … tune at the TASK-086/087 eyeball gate"):** Footman now `helm_dome` (box z 0.86–1.0, x 0.18–0.82, dodges the spearhead) + `shoulder_caps` (box z 0.70–0.85, x 0.14–0.90, y 0.12–0.88 AND normal-up min_dot 0.55). Result: blue helmet dome + shoulder caps, top-down team read, **5.7% area** (cap 35%). Recorded in pipeline_manifest.json (`_tuned` note).
- **Final report (all PASS):** tris **15,000** exact; minZ 0.028; `UVMap`; slots `["TeamRegion","FootmanPBR"]`; 406k→235k verts welded, 60 islands removed, 3 holes filled; D/N/ORM 1024².
- **Accepted WARN:** conformed dims [86.1, 47.7, 180] vs blockout [147, 80, 180] — X/Y warn-only under `fit_mode: height` (blockout width included a sword-out stance). Z exact. **WATCH note for Jonathan at TASK-088: the new Footman is slimmer than the blockout.**
- **FBX independently verified:** exactly 2 materials [TeamRegion, FootmanPBR] (QA TASK-083 WARN-3 closed live), 15k tris, single UVMap.
- Textures = real PNGs (magic-checked) at Content/RawAssets/Textures/Footman/. Concept copied → Content/RawAssets/Concepts/Footman.png. TASK-084 smoke residue cleaned from Cache/Footman before running.

## Stage 3 — EDITOR IMPORT: COMPLETE

### M_AssetPBR authoring (one-time master, /Game/Materials/M_AssetPBR)
| Node | ParameterName | SamplerType | Default Texture | Wired |
|---|---|---|---|---|
| TextureSampleParameter2D_0 | `BaseColor` | SAMPLERTYPE_Color | /Engine/EngineResources/DefaultTexture | RGB → MP_BaseColor |
| TextureSampleParameter2D_1 | `ORM` | SAMPLERTYPE_LinearColor | /Game/Textures/T_AssetPBR_NeutralORM | R → MP_AmbientOcclusion, G → MP_Roughness, B → MP_Metallic |
| TextureSampleParameter2D_2 | `Normal` | SAMPLERTYPE_Normal | /Engine/EngineMaterials/DefaultNormal | RGB → MP_Normal |

Recompiled clean, saved; names/wires readback-verified. Documented deviation: helper `/Game/Textures/T_AssetPBR_NeutralORM` (16×16 linear, AO 1/Rough 0.8/Metal 0, SRGB off) — params can't compile with Texture=None and no stock engine texture passes the LinearColor sampler check; MIs always override.

### Texture imports (/Game/Textures/)
| Asset | Readback settings | TASK-087 note |
|---|---|---|
| T_Footman_D | SRGB true, TC_Default (AutoDXT) | import defaults correct |
| T_Footman_N | SRGB false, TC_Normalmap (BC5) | auto-detected from the `_N` suffix |
| T_Footman_ORM | TC_Default, **SRGB manually set false** | ALWAYS needs the manual SRGB→false after import |

### MI_Footman_PBR (/Game/Materials/Instances/)
Created from M_AssetPBR; `BaseColor`→T_Footman_D, `Normal`→T_Footman_N, `ORM`→T_Footman_ORM (readback-verified). Master has no scalar/vector params.

### Mesh (post-reimport, all readback-verified)
- Slots: `["TeamRegion","FootmanPBR"]`; slot TeamRegion → **MI_TeamColor_Blue** (design-time placeholder; BeginPlay recolor owns it), slot FootmanPBR → **MI_Footman_PBR**.
- Tris **15,000** (blockout was 2,152); bounds X −43.0..43.0 / Y ±23.8 / Z 0.028..179.68 (matches refine_report exactly; blockout was 147×80×180).
- **Nanite false** (was false pre- and post-reimport; law honored).
- Collision: `generate_convex_collisions(hull_count=4, max_hull_verts=16)` → success (units ≤4 hulls law). NOTE: no hull-count readback tool exists — TASK-088's structural check should confirm visually/PIE.
- Referencers: **BP_Unit_Footman intact** (/Game/Blueprints/Units/BP_Unit_Footman).
- Log scan: **zero import warnings, zero MikkTSpace/degenerate/tangent/smoothing matches** for the reimport (Interchange FBXSDK, "Built static mesh [0.17s]"). Only Footman warnings in session = pre-existing PIE NavMesh lines (01:34, TASK-081's test) + my documented refused MCP attempt.
- Saved: SM_Footman, T_Footman_D/_N/_ORM, MI_Footman_PBR, M_AssetPBR, T_AssetPBR_NeutralORM (`save_assets` true).
- Visual record: editor thumbnail decoded at Cache/Footman/sm_footman_editor_thumb.png — blue helmet + textured armor + griffin heraldry rendering live in-engine.

## Commit manifest for TASK-088 (nothing committed by me — no Git in my lane)
Content/RawAssets/Footman.fbx (overwritten), Content/RawAssets/Textures/Footman/T_Footman_{D,N,ORM}.png (new), Content/RawAssets/Concepts/Footman.png (new), Content/Meshes/SM_Footman.uasset (reimported+saved), Content/Textures/T_Footman_{D,N,ORM}.uasset + T_AssetPBR_NeutralORM.uasset (new), Content/Materials/M_AssetPBR.uasset (new), Content/Materials/Instances/MI_Footman_PBR.uasset (new), Tools/ArtPipeline/pipeline_manifest.json (selector tune). Lane isolation honored — no CardArt paths touched; no level/BP edits.

## Slack ledger
🎨 Art: 1783480513.247849, 1783480647.294109, 1783481000.364949, 1783481736.126409, 1783487776.800409, 1783488121.308969, + final ✅ (see thread). 🚨 Blockers: 1783480630.063159 (Stage-1 bug — resolved), 1783488105.900489 (one-click ask — CLOSED by Jonathan's reimport).
