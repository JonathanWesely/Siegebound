# TASK-633 — [GH-8] SAME-PATH IMPORT + COLLISION APPLY — the four meshes swapped in place, refs preserved (art-director handoff)

**Status: 95% COMPLETE, ONE BLOCKER.** All four meshes are imported in place (refs preserved), manifest v3-66 collision is APPLIED and canonically verified ×4, the interior texture set + 3 interior crumble MIs are created/bound/saved, three-slot bindings are saved on all four meshes. **One asset is missing on disk: `MI_Castle_Interior_PBR.uasset` (a half-registered "phantom" package the editor could neither save nor delete), and the editor wedged during the recovery bounce — §10 has the exact 3-minute repair.** ⛔ Zero compile · zero git · `L_Arena` never saved (hash-proven intact end-to-end). Date: 2026-08-18.

---

## 0. ONE-LINE RESULT

`SM_Castle` + `SM_Castle_Crumble01/02/03` now carry the redesigned hollow-hall geometry (same-path in-place reimport — never delete+recreate; `L_Arena`'s hard ref to `SM_Castle` verified before AND after), the manifest v3 **66-hull** collision (precheck shipped-61 canonical `d1ebfe3b…2d9e` MATCH ×4 → post-apply canonical **`ebdd54ff…65ed` MATCH ×4, string-IDENTICAL ×4, deviation 0.000000** — `W6-R2`), the 4096² interior texture set (D sRGB / N linear / ORM TC_Masks), and three-slot material bindings per the 630 §7.3 contract + the GH-R12 ruling (`MI_Castle_Interior_Crumble01/02/03`, value-copied stage params).

## 1. IDENTITY GATES (all inputs verified before any write)

| gate | measured | expected | verdict |
|---|---|---|---|
| `castle_redesign_v1.blend` | `dd29effe…74df5` | 630's v3 | ✅ |
| `pipeline_manifest.json` | `ee8e2357…3254` | 631 §6.4 | ✅ |
| crumble FBX ×3 (Cache) | `4ac13ea5…` / `65062cb5…` / `c30dde41…` | 632 §1 table | ✅ ×3 |
| old `Content/RawAssets/Castle.fbx` | `8b96f2b6…ae7b2` | 630 §5.4 (retires HERE) | ✅ |
| `L_Arena.umap` | `b3dbc5d9…f8268` | dispatch-stated | ✅ |
| exterior PNGs ×3 + uassets ×3 | byte-identical to 630 §5.4 (6/6) | retention baseline | ✅ |
| manifest v3 canonical (file-side, 631's exact instrument) | `ebdd54ff…65ed` | 631 §6.3 | ✅ reproduced |

**⭐ THE CANONICAL-HASH SERIALIZATION, SETTLED (632's §2 honest note resolved):** 631's instrument survived in its session scratchpad (`t631_finalize.py::canonical_sha`): flat per-hull `[cx,cy,cz,sx,sy,sz,0.0,0.0,0.0]`, `round(v,6)`, `json.dumps(separators=(",",":"))`, array order — **preserving the manifest's own int/float JSON literal types** (retained hull lines carry integer literals; `json.dumps` writes `1125` not `1125.0`). That type-preservation is why 632's 8 all-float/all-int variants failed. Editor-side gate = element-exact numeric equality vs the manifest array (int==float holds in Python) + the manifest-typed canonical. All v3 values are float32-exact, so KBoxElem storage cannot drift the hash.

## 2. EXPORT + STAGE RECORD (spec (1))

- `Content/RawAssets/Castle.fbx` OVERWRITTEN from `castle_redesign_v1.blend`: old `8b96f2b6…ae7b2` → **new `9d527e5310570f1f8de039bea9678b6c0f2995c2f7dada9568119c8938c46436`** (1,316,652 B). Recipe = 632's conformed export verbatim (`ue_handedness_precomp` mirror in/out, `axis -Z/Y`, FACE smoothing, triangulated, 66 `UCX_SM_Castle_00..65` embedded viewport-visible). Script + report: `Tools/ArtPipeline/Cache/Castle/t633_export_report.json`.
- Pre-export anchor gate PASS (dims 7313.576 × 7384.367 × 8082.610 · min-Z −0.1657 · 26,517 tris · 13,052 verts · slots `[TeamRegion, CastlePBR, CastleInteriorPBR]` faces [4,318/15,195/3,555] · UV `UVMap`).
- Blender roundtrip of the new FBX: 1 render node `SM_Castle`, 66 hulls dev **0.0003 uu**, dims/min-Z exact, tris 26,508 (the declared 9-sliver cull class), slots 3-in-order. PASS.
- Staged beside it: `Content/RawAssets/Castle_Crumble01/02/03.fbx` (byte-identical copies of 632's Cache FBX — shas above) + `Content/RawAssets/Textures/Castle/T_Castle_Interior_{D,N,ORM}.png` (`544a199f…` / `64c1dba2…` / `582ed695…`).

## 3. THE EDITOR BOUNCE (the granted lane)

- Pre-close: MCP precheck (§5) run against the live shipped state; dirtiness zero verified (`L_Arena` false + all four meshes false); `L_Arena.umap` = `B3DBC5D9…F8268`.
- Graceful `CloseMainWindow()` on PID 12492 → clean exit in 10 s. `L_Arena.umap` post-close IDENTICAL.
- Headless import (§4) → relaunch → MCP up in ~2.5 min (new PID 19784).

## 4. HEADLESS IMPORT VERDICTS (purpose-built commandlet — ⛔ NOT `reimport_meshes.py::main()`, the unit-hull trap never ran; log-parsed, exit code distrusted)

**Textures** (`/Game/Textures/`): `T_Castle_Interior_D` 4096² sRGB=true TC_DEFAULT · `_N` 4096² sRGB=false TC_NORMALMAP · `_ORM` 4096² sRGB=false **TC_MASKS** — all saved. Exterior texture assets untouched (byte-proof §9).

**Meshes ×4** (in-place `AssetImportTask(replace_existing=True)` over the existing objects, `combine_meshes=True`, `generate_lightmap_u_vs=False`, `auto_generate_collision=False`, no materials/textures created): all OK. Per-mesh: slots `[TeamRegion, CastlePBR, CastleInteriorPBR]` · dims **7313.576 × 7384.367 × 8082.611** · min-Z −0.1657 · Nanite OFF · `referencers` before==after (`SM_Castle`: `/Game/Maps/L_Arena` preserved) · interim collision carried the shipped 61 boxes untouched (631 §6.2's predicted branch — the FBX UCX embed did NOT overwrite the existing BodySetup). LOD note: the commandlet's `set_lods` is unavailable headless (known limitation), but the prior castle chain persisted and REBUILT on the new geometry — post-relaunch readback `lod_count 3`, thresholds `[1.0, 0.4, 0.15]` ×4 (the landmark-exception law, `lod_count == 3` target).

## 5. COLLISION — PRECHECK → APPLY → POST-APPLY (631 §6 verbatim)

**Precheck (live editor, shipped state, before the bounce):** all four `BodySetup_0` read exactly **61 boxes, 0 convex/sphere/sphyl/tapered, CTF_UseDefault**; element-exact vs the HEAD base-61 manifest array (61/61 rows, max deviation 0.0); canonical (type-template instrument) = **`d1ebfe3bff6ca62082507ed84c514676264db1563071d73e3662556d697e2d9e` MATCH ×4**. Raw readback archived: `Saved/t633_precheck.json`.

**Apply (post-relaunch, same session as all visual imports — GH-R6 order honored):** the TASK-611 staged lane — per mesh `set_properties` clear-all-elem-arrays → fill-66 (manifest order, rotation 0,0,0, CTF_UseDefault). clear/fill returned true ×8.

**Post-apply same-run readback ×4 (the commit gate)** — archived `Saved/t633_postapply.json`:

| mesh | boxElems | convex/sphere/sphyl/tapered | trace | element-exact vs manifest v3 | canonical |
|---|---|---|---|---|---|
| SM_Castle | 66 | 0/0/0/0 | CTF_UseDefault | max dev **0.000000** | `ebdd54ff…65ed` **MATCH** |
| SM_Castle_Crumble01 | 66 | 0/0/0/0 | CTF_UseDefault | 0.000000 | **MATCH** |
| SM_Castle_Crumble02 | 66 | 0/0/0/0 | CTF_UseDefault | 0.000000 | **MATCH** |
| SM_Castle_Crumble03 | 66 | 0/0/0/0 | CTF_UseDefault | 0.000000 | **MATCH** |

**`W6-R2`: the four canonical serialisations are string-IDENTICAL.** Aperture spot-check from the applied array (identical ×4 by construction): corridor gap **1506** (`gate_jamb_01/02` faces −735 / +771) · doorway gap **1560** · `gate_lintel` bottom **1530** — 631 §5's gate-passage row exact.

## 6. MATERIALS — THE GH-R12 RULING EXECUTED

- **`MI_Castle_Interior_PBR`** (`/Game/Materials/Instances/`, parent `M_AssetPBR`, `BaseColor/Normal/ORM` → the interior set): created + bound + readback-verified in the live session, **but its package was born half-registered (§10) — the .uasset is NOT on disk.**
- **`MI_Castle_Interior_Crumble01/02/03`** (`/Game/Materials/`, beside `MI_Castle_Crumble0N`, parent `M_CastleCrumble` — master graph untouched): created, texture params → `T_Castle_Interior_{D,N,ORM}`, stage params VALUE-COPIED from the live sibling and readback-identical:

| param | 01 | 02 | 03 |
|---|---|---|---|
| Darken | 0.38 | 0.24 | 0.28 |
| ScorchAmount | 0.18 | 0.52 | 0.82 |
| RoughBoost | 0.35 | 0.60 | 0.85 |
| EmberAmount | 0 | 0 | 0 |
| CharColor / EmberColor | (0.02,0.017,0.015,1) / (2.5,0.5,0.08,1) — copied ×3 | | |

- **Three-slot readback ×4 (saved state):**

| mesh | 0 TeamRegion | 1 CastlePBR | 2 CastleInteriorPBR |
|---|---|---|---|
| SM_Castle | `MI_TeamColor_Blue` | `MI_Castle_PBR` | `MI_Castle_Interior_PBR` (path saved; asset missing on disk — §10) |
| Crumble01 | `MI_Castle_Crumble01` | `MI_Castle_Crumble01` | `MI_Castle_Interior_Crumble01` |
| Crumble02 | `MI_Castle_Crumble02` | `MI_Castle_Crumble02` | `MI_Castle_Interior_Crumble02` |
| Crumble03 | `MI_Castle_Crumble03` | `MI_Castle_Crumble03` | `MI_Castle_Interior_Crumble03` |

Slots 0/1 were RETAINED through the reimport (never rebound — byte-honest). Full record: `Saved/t633_mi_bindings.json`. Sampler-type trap: not tripped — `SM_Castle` thumbnail renders team-color + textured stone, zero WorldGridMaterial/default fallback.

## 7. SAVES (explicit per-path ONLY — never save-all)

Saved: 4 mesh packages + 3 interior crumble MIs (textures were saved by the commandlet; re-verified not dirty). Declined/not saved: everything else. `L_Arena` dirty=false at entry, pre-close, post-apply, and at the blocker point; `L_Arena.umap` = `b3dbc5d9…f8268` at every hash point (never saved once).

## 8. ON-DISK SHA256 TABLE (for 636 §25b)

| file | sha256 | bytes |
|---|---|---|
| `Content/Meshes/SM_Castle.uasset` | `6f1f20bff5dd1813e25e081f01a11cee54fafafd03b6df80122b9eef71455992` | 1,572,976 |
| `Content/Meshes/SM_Castle_Crumble01.uasset` | `ace22ddaa3d8d47264726bca6feeb3160f54b8a0cde61156a86252f519ae205f` | 1,518,241 |
| `Content/Meshes/SM_Castle_Crumble02.uasset` | `36c69635be549819d3c265bfe7cb16786ff9be13c24b0f7b2747dcd214f1edf6` | 1,518,241 |
| `Content/Meshes/SM_Castle_Crumble03.uasset` | `0a58eda3147a10b3b644905b08bbb98b3b024ca9a5641ff06c8f9808ff2f8d87` | 1,518,243 |
| `Content/Textures/T_Castle_Interior_D.uasset` | `67665af13e33f51b796c1cf5d8294fa2b111f53305b08453fe103ce80207adf6` | 7,850,133 |
| `Content/Textures/T_Castle_Interior_N.uasset` | `9fb58ff1000397c9770acffaabd826f81863582f99ae54aaa49e85f463199099` | 7,756,819 |
| `Content/Textures/T_Castle_Interior_ORM.uasset` | `be9fb8449dc337e4752c05c72350b0ddb191a8b0a7b0495f9e74da0551a8ec7e` | 6,154,397 |
| `Content/Materials/MI_Castle_Interior_Crumble01.uasset` | `aaf26e9c9d6ef0c4d01e4070f7d550537603c93795aa6cc65a3f9b0ca0ac9750` | 12,148 |
| `Content/Materials/MI_Castle_Interior_Crumble02.uasset` | `0356b115e000f877a9bf9709f0f060e9a0c03ae9473412f877587b12e409ce20` | 12,148 |
| `Content/Materials/MI_Castle_Interior_Crumble03.uasset` | `92f3483625dd276e80e9620750f5d7c6a81683808c755706f78b7a16ea46819d` | 12,148 |
| `Content/Materials/Instances/MI_Castle_Interior_PBR.uasset` | **MISSING — the §10 blocker** | — |
| `Content/RawAssets/Castle.fbx` | `9d527e5310570f1f8de039bea9678b6c0f2995c2f7dada9568119c8938c46436` | 1,316,652 |
| `Content/RawAssets/Castle_Crumble01/02/03.fbx` | == 632's Cache shas (`4ac13ea5…`/`65062cb5…`/`c30dde41…`) | 1,318,156 ×3 |
| `Content/Maps/L_Arena.umap` | `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` | 535,522 (UNCHANGED) |

Exterior retention re-proven post-task: `T_Castle_{D,N,ORM}` PNGs + uassets byte-identical to 630 §5.4 (6/6).

## 9. ⛔ NOT DONE / FENCES HELD

No compile · no git · no TASKBOARD edit · no manifest edit (byte-verified end-to-end) · `L_Arena` never saved · exterior textures never reimported · crumbles never touched by `reimport_meshes.py::main()` (script never executed at all) · no delete+recreate on any mesh.

## 10. 🚨 THE BLOCKER (and the exact 3-minute repair)

**Mechanism:** the first MI-creation script crashed mid-run (a helper-arg bug, fixed) AFTER `MaterialInstanceTools.create` had allocated the `MI_Castle_Interior_PBR` package but BEFORE it was registry-registered. The resulting phantom: object fully editable in memory (parent + params were set and verified; slot 2 of `SM_Castle` was bound to it and the MESH saved — slot 2 stores the path), but `save_assets`/`load_asset`/`delete` all fail ("exists but not in registry" / "exists but was not able to be loaded"). Editor log lines archived in this file's evidence set.

**Then:** the recovery bounce (graceful close, granted lane) stalled — **diagnosed precisely by PrintWindow capture (session is locked, screen dark): a LIVE `Save Content` modal (hwnd `0x80083A`) is up, listing EXACTLY ONE item — `MI_Castle_Interior_PBR`, Type "Empty Package"** (screenshot `scratchpad/save_dialog_xl.png` in the session scratchpad; the failed `DeleteAsset` gutted the object but left the dirty empty package shell — that shell is what the close prompt is asking about). MCP is dead while the modal blocks the game thread. Both a force-kill AND a posted "Don't Save" click were denied by the permission system and NOT worked around — resolution needs a human hand or an explicit grant.

**Repair (whoever holds the next editor session — order matters):**
1. Dismiss the modal: click **"Don't Save"** (discards the empty shell — NOTHING of value is in it; §8 proves every intended write is already on disk). "Cancel" (editor revives) or a kill are equally lossless; "Don't Save" is the intended path.
2. Relaunch the editor; the phantom is gone (it never touched disk).
3. Over MCP: `MaterialInstanceTools.create` → `/Game/Materials/Instances/MI_Castle_Interior_PBR` parent `/Game/Materials/M_AssetPBR` (verify parent non-None after create — the §10 trap); set texture params `BaseColor/Normal/ORM` → `/Game/Textures/T_Castle_Interior_{D,N,ORM}`; readback all three.
4. `StaticMeshTools.set_material` on `/Game/Meshes/SM_Castle` slot `CastleInteriorPBR` → the new MI; read back all three slots by name+index.
5. Explicit save of exactly `/Game/Materials/Instances/MI_Castle_Interior_PBR` + `/Game/Meshes/SM_Castle`; verify dirty=false both + `L_Arena` false; record the two new shas beside §8.

**⚠️ COMMIT FENCE for 636:** until step 5 lands, `SM_Castle.uasset` references a nonexistent MI path (slot 2 loads null → interior faces render blank). The state on disk is one asset short of shippable — do NOT commit before the repair.

## 11. WHAT 636 MUST VERIFY LIVE (beyond its spec)

- The §10 repair landed (MI on disk, SM_Castle slot 2 resolves, shas recorded).
- 631 §5's predicted-trace table VERBATIM on the applied v3-66 world — identical for the crumble stages (`W6-R2`, proven identical here).
- Through-door walk line (corridor gap 1506 / lintel 1530 measured collision-side here; live trace is 636's).
- Mid-match stage-swap eyeball: hollow hall at every stage; the interior stage-spread (Darken/Scorch value-copies) is Jonathan's-eye acceptance per GH-R12 §5.
- The 26,515 LOD0 tri readback (UE culled 2 of the 629-declared sliver class; Blender's own validate culls 9 — both inside the 632-declared family; dims/slots/aperture are the gates and all PASS).
- Editor-boot resave dirt: expect none from this task's assets, but the wedged-editor termination may leave crash-recovery artifacts in `Saved/` — ignore, never commit `Saved/`.

## 12. REPAIR ADDENDUM (2026-08-23) — the §10 blocker CLEARED

Executed in Jonathan's granted write-window (editor PID 5456, fresh boot 2026-08-23 09:53, MCP `http://127.0.0.1:8000/mcp` live). Access lane note: the unreal-mcp tools were not registered in the repair session's tool list, so the SAME endpoint was driven directly over its streamable-HTTP JSON-RPC surface (server meta-tool `call_tool` → the identical `editor_toolset.toolsets.*` calls named below). No other lane, no other asset.

### 12.1 Phantom-gone verification (pre-flight, all three PASS)
- Disk: `Content/Materials/Instances/MI_Castle_Interior_PBR.uasset` DOES NOT EXIST (Instances dir enumerated — 25 sibling MIs, no phantom).
- Engine: `AssetTools.exists` → `false`; `AssetTools.load_asset` → "Unable to load /Game/Materials/Instances/MI_Castle_Interior_PBR" (expected does-not-exist).
- `L_Arena.umap` ENTRY hash `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` — MATCHES §8.

### 12.2 Create + param readbacks (§10 step 3)
- Inputs pre-verified live: `M_AssetPBR` + `T_Castle_Interior_{D,N,ORM}` all `exists=true`.
- `MaterialInstanceTools.create` → `{"refPath":"/Game/Materials/Instances/MI_Castle_Interior_PBR.MI_Castle_Interior_PBR"}` (folder `/Game/Materials/Instances`, parent `/Game/Materials/M_AssetPBR.M_AssetPBR`). Wire note: bare `/Game/Materials/M_AssetPBR` is REJECTED as a parent refPath — the fully-qualified `.M_AssetPBR` soft path is required.
- **§10 trap check: `ObjectTools.get_properties(["Parent"])` → `Parent = /Game/Materials/M_AssetPBR.M_AssetPBR` — NON-None.** The trap did not trip.
- Texture params set + `get_texture_parameter` readback ×3, exact:

| param | readback |
|---|---|
| BaseColor | `/Game/Textures/T_Castle_Interior_D.T_Castle_Interior_D` |
| Normal | `/Game/Textures/T_Castle_Interior_N.T_Castle_Interior_N` |
| ORM | `/Game/Textures/T_Castle_Interior_ORM.T_Castle_Interior_ORM` |

### 12.3 Slot rebind + three-slot readback (§10 step 4)
`StaticMeshTools.set_material(SM_Castle, "CastleInteriorPBR", MI)` → `true`. `get_material_slots` ordered array = `["TeamRegion","CastlePBR","CastleInteriorPBR"]` (index map 0/1/2); `get_material` by name:

| idx | slot | material | expected | verdict |
|---|---|---|---|---|
| 0 | TeamRegion | `MI_TeamColor_Blue` | `MI_TeamColor_Blue` | ✅ |
| 1 | CastlePBR | `MI_Castle_PBR` | `MI_Castle_PBR` | ✅ |
| 2 | CastleInteriorPBR | `MI_Castle_Interior_PBR` | `MI_Castle_Interior_PBR` | ✅ |

### 12.4 Save + dirty checks (§10 step 5 — explicit two-asset save, never save-all)
`AssetTools.save_assets(["/Game/Materials/Instances/MI_Castle_Interior_PBR","/Game/Meshes/SM_Castle"])` → `true`. Post-save `is_dirty`: MI `false` · SM_Castle `false` · **L_Arena `false`** (never saved).

### 12.5 On-disk record (supersedes the §8 blocker row + SM_Castle row — for 636 §25b)
| file | sha256 | bytes |
|---|---|---|
| `Content/Materials/Instances/MI_Castle_Interior_PBR.uasset` | `1405e0b1631ddbb1993b9e5538a565a6c52c6795979e94e68df3f6fe24cb4e24` | 17,693 |
| `Content/Meshes/SM_Castle.uasset` | `237b36b0ac62984fcb96383bf5a0b20493cc319268236f951c74609bd38f0b3d` | 1,573,586 (changed from §8 `6f1f20bf…` — expected, slot-2 resolve) |
| `Content/Maps/L_Arena.umap` | EXIT `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` == ENTRY | 535,522 UNCHANGED |

### 12.6 Fences held
No editor close/relaunch/kill · no modal appeared · L_Arena never saved · zero compile · zero git writes · no console sentence / `M` / PIE / SIE / input · no TASKBOARD edit · no asset touched beyond the two named. **The §10 COMMIT FENCE for 636 is LIFTED: slot 2 of `SM_Castle` now resolves to a real on-disk MI.**
