# QA Report — TASK-172 / TASK-173 (M7 mesh-reimport pipeline tooling)

**Verdict: PASS**  ·  Blockers: 0  ·  Warnings: 2  ·  Nits: 4
**Reviewed:** 2026-07-16 (qa-reviewer)
**Files reviewed:**
- `Tools/reimport_meshes.py` (headless `-run=pythonscript` commandlet, step 1)
- `Tools/reimport_finalize_materials_mcp.py` (MCP ProgrammaticToolset body, step 2)
- `Tools/reimport_cards.txt` (CardID sidecar)
- Cross-checked against `Tools/ArtPipeline/pipeline_manifest.json`, `.claude/pipeline/CONVENTIONS.md`,
  and handoffs TASK-172 / TASK-172-173.

This is Python editor tooling (no compiled C++), reviewed with the Tooling-Python rigor:
destructive-safety, save scoping, collision math, category branching, robustness, secret/side-effect hygiene.
Empirically validated on 14 meshes this run; this review confirms it for the record and for reuse on Wall/DeepMine.

---

## Scrutiny findings (mapped to the 6 review asks)

### 1. Destructive-safety — PASS (this is the crux; it holds)
- **In-place reimport, never delete+recreate.** `_reimport_one` (reimport_meshes.py:364-379) issues a single
  `unreal.AssetImportTask` with `filename=<CardID>.fbx`, `destination_path=/Game/Meshes`,
  `destination_name=SM_<CardID>`, `replace_existing=True`, `replace_existing_settings=True`, `automated=True`.
  Destination path+name are identical to the existing asset, so the importer reimports the SAME UObject — the
  UObject identity and its content path are preserved, which is exactly what keeps `BP_Unit_<CardID>` /
  `BP_Building_<CardID>` hard refs and the `/Game/Meshes/SM_<CardID>` string soft ref (ghost + cards.csv) intact.
  This matches CONVENTIONS line 224/226 ("same-path swaps keep every code/BP soft reference intact. NEVER
  delete+recreate the SM asset"). There is **no `delete_asset`, `rename_asset`, or new-path create anywhere** in
  either file — confirmed by full read.
- **Never-create guard.** Preconditions (reimport_meshes.py:337-344) SKIP any CardID whose FBX is missing OR whose
  `SM_<CardID>` does not already exist — the script reimports only, it will never author a brand-new mesh at a
  stray path. Good.
- **Ref diagnostics captured.** `referencers_before` / `referencers_after` are read via
  `find_package_referencers_for_asset` (lines 346-347, 420-421) and logged. (See NIT-3: captured but not asserted.)
- **Save scoping is correct — no accidental save-all.** Every persist is a per-asset
  `EditorAssetLibrary.save_loaded_asset(<obj>, False)`: mesh (line 406), each texture (line 191), the MI (line 231).
  The `False` is `only_if_is_dirty=False` (force this one asset), NOT a save-all. The MCP finalize saves only the
  explicit `save_paths` list of the reimported meshes (finalize:172-173). No unrelated dirty package is touched.

### 2. Editor-bounce safety — NOT IN THESE FILES (out of code scope); commandlet preconditions are sound
- The graceful bounce (MCP save-all → `CloseMainWindow()` → wait clean exit → run commandlet → relaunch → confirm
  MCP:8000) is **orchestrated externally**, not code in either reviewed file, so there is no half-close / force-kill
  / project-lock code path here to fault. What IS in-file and correct: the header (reimport_meshes.py:55-56)
  documents the hard precondition that the interactive editor MUST be closed (project lock) before the commandlet
  runs, and the commandlet itself takes no action that would fight a running editor. The bounce sequence is
  attested clean in both handoffs (no `/F`, save-all first). Nothing to fix in code.

### 3. Collision correctness — PASS
- **Unit branch** (`_apply_unit_hulls`, lines 258-274): ≤4 convex decomposition hulls via
  `EditorStaticMeshLibrary.set_convex_decomposition_collisions(sm, 4, 16, 100000)`, with the UE5.8 gotcha handled
  correctly — `StaticMeshEditorSubsystem` is None inside a commandlet, so the code prefers the
  `EditorStaticMeshLibrary` free-function path and only falls back to the subsystem if the library is absent
  (`getattr(unreal, "EditorStaticMeshLibrary", None)`). NON-FATAL on failure (WARN, finish over MCP). Correct.
- **Building/GoldNode box branch** (`_apply_box_collision`, lines 277-323): boxes come from
  `manifest[cid].ucx.boxes` (line 402), each `{center:[x,y,z], size:[x,y,z]}` in UE units. The code maps
  `size` → `KBoxElem.x/y/z` and `center` → `KBoxElem.center`. **This is the correct mapping**: `FKBoxElem.X/Y/Z`
  are the box's FULL dimensions (not half-extents; `FKBoxElem::CalcAABB` uses `0.5*(X,Y,Z)`), which I verified
  against the Castle manifest data (wall_west size `[85,440,280]` at center z=140 spans z 0..280 = full-height wall,
  only self-consistent as full-dims). `unreal.KBoxElem` / `agg_geom.box_elems` are the correct UE5.8 Python
  bindings for `FKBoxElem` / `FKAggregateGeom.BoxElems`.
- **Deterministic bootstrap + overwrite.** It bootstraps a `body_setup` via a 1-hull convex decomposition (guarantees
  a body exists even if the FBX shipped no UCX), then clears `convex_elems/sphere_elems/sphyl_elems`, writes
  `box_elems`, sets `CTF_USE_DEFAULT`, and `invalidate_physics_data()` (wrapped defensively). Struct-copy write-back
  discipline is correct throughout (agg_geom and nanite_settings are value-type structs fetched, mutated, and
  re-set; body_setup is a UObject ref). Net result = boxes only. This deterministically prevents the
  "building ships with no/auto collision → M1 plinth interior dead-zone" failure. Correct.
- **The other UE5.8 gotcha** (commandlet post-build resets empty mesh slots to WorldGridMaterial) is correctly
  offloaded to the MCP finalize step — documented at lines 42-53 and handled in reimport_finalize_materials_mcp.py.

### 4. GoldNode single-slot / CrystalTower emissive — PASS
- **Category detection** (`_category_of`, lines 156-162): returns `goldnode` only when `category=="building"` AND
  `team_region` is explicitly `null`. Verified against the manifest — GoldNode has `"team_region": null` → resolves
  `goldnode`; all other buildings have a `team_region` dict → resolve `building`. The `"MISSING"` sentinel default
  means an ABSENT team_region key does NOT trigger the goldnode variant (only an explicit null) — safe, conservative.
- **GoldNode geometry step** (lines 353-362): correctly SKIPS the texture/MI import (keeps authored `M_GoldGlow`
  emissive, no orphan PBR MI, no glow loss) — matches CONVENTIONS line 86-87/94 ("Gold nodes … carry the authored
  M_GoldGlow emissive regardless of team") and the manifest `_variant` note.
- **GoldNode finalize** (finalize:152-169): applies `M_GoldGlow` to EVERY slot, no TeamColor, and flags (non-fatal)
  if the refined FBX emitted ≠1 slot (the known `refine_trellis_glb.py` two-slot-when-null quirk). Correct + defensive.
- **CrystalTower finalize** (finalize:134-150): slot0→`MI_TeamColor_Blue`, slot1→`MI_CrystalTower_PBR`, and slot2→
  `M_CrystalGlow` ONLY if a 3rd slot exists; else raises the documented non-blocking FLAG (M_AssetPBR has no emissive
  param, no T_CrystalTower_E baked). Matches the manifest `_emissive_note`. Correct handling of a known art gap.

### 5. Robustness — PASS
- **Per-asset isolation:** `main()` wraps each `_reimport_one` in try/except (lines 436-440) so one bad asset never
  corrupts others or aborts the batch; internally, texture/MI and both collision paths are individually non-fatal
  (geometry reimport is the protected priority).
- **Idempotency:** `_ensure_textures_and_mi` early-returns if the MI already exists (lines 199-201); reimport
  overwrites deterministically; collision is re-authored each run; save is per-asset. Re-running the same sidecar is
  safe. The U1 done-units are deliberately excluded from the sidecar (verify-only) and can be re-added for an
  idempotent re-run.
- **Path handling:** all runtime paths derive from `unreal.Paths.project_content_dir()` and a `__file__`-relative
  manifest — no hardcoded absolute paths in the logic (the absolutes in the header are documentation of the run
  command only). Portable/reusable.
- **Sidecar/finalize/manifest are consistent:** sidecar = 10 cards; finalize `REIMPORTED` = 10, `ALL14` = 14; every
  building in the wave has a `ucx.boxes` entry. Reuse for Wall/DeepMine is safe by construction (both are
  `category:building` with a footprint box and will route through the box branch) — the only precondition is that
  their baked D/N/ORM textures exist first (see WARN-1).

### 6. Secret / side-effect hygiene — PASS
- No env reads, no tokens, no network calls, no subprocess, no filesystem writes outside the UE asset save API and
  reading the sidecar/manifest. Nothing to leak. Correct for local editor tooling.

---

## Findings

- **[WARN] reimport_meshes.py:195-234 / reimport_finalize_materials_mcp.py:100-109** — Reuse hazard for a
  premature run (e.g. Wall/DeepMine before textures are baked): if `_ensure_textures_and_mi` can't import the PNGs it
  returns None (WARN, no MI created) and the geometry reimport still proceeds; the downstream MCP `_finalize_two_slot`
  will then `set_material` slot1 → a NON-EXISTENT `MI_<CardID>_PBR`, leaving that slot pointing at a missing/failed
  material. It degrades to an MCP error surfaced in the finalize readback rather than a crash, but the operator must
  ensure the baked textures exist before adding a CardID to the sidecar. Non-blocking for THIS wave (Wall/DeepMine
  intentionally excluded). Suggested: have the finalize skip/flag the PBR-slot assignment when `does_asset_exist(MI)`
  is false, so a missing MI is reported explicitly instead of silently nulling a slot.

- **[WARN] reimport_finalize_materials_mcp.py:100-109 (index-based slot assignment)** — `_finalize_two_slot`
  assigns by slot index (`sl[0]`→TeamColor, `sl[1]`→PBR), relying on the refined FBX always emitting slots in
  `[TeamRegion, <CardID>PBR]` order. The 2-slot contract guarantees this and the handoff confirms it held for all 8
  units/buildings, but if a future refine reorders slots the TeamColor and PBR materials would swap silently. Low
  risk given the contract; suggested hardening: match on slot NAME (`TeamRegion` vs `<CardID>PBR`) rather than index.

- **[NIT] reimport_meshes.py:50** — Comment points to `Tools/reimport_apply_materials_mcp.py` (the TASK-172
  2-slot-only finalize, still present on disk) as the step-2 companion, but this wave's step-2 is the superset
  `reimport_finalize_materials_mcp.py`. Doc drift only; update the pointer to avoid a future operator running the
  wrong (non-GoldNode/non-Crystal-aware) finalize.

- **[NIT] reimport_meshes.py:346-347, 420-421** — `referencers_before`/`referencers_after` are captured and logged
  but never diff-asserted. The whole point of the in-place-reimport claim is ref survival; a cheap
  `assert set(after) >= set(before)` (or a WARN when a referencer disappears) would turn the manual eyeball-check into
  an automatic guard. Diagnostic-only today; empirically the refs survived. Optional hardening.

- **[NIT] reimport_meshes.py:376-378** — `imported_object_paths` is logged but not asserted equal to the expected
  `/Game/Meshes/SM_<CardID>.SM_<CardID>`. Given `replace_existing`+`destination_name` it cannot diverge, but an
  explicit check would make the "path unchanged → refs intact" invariant self-verifying. Optional.

- **[NIT] reimport_meshes.py:294-317** — The box-authoring bootstrap (create 1 convex hull, then clear
  `convex_elems` and overwrite with `box_elems`) is slightly roundabout purely to guarantee a `body_setup` exists.
  It works and is defensively wrapped in try/except (a None body_setup would raise → caught → WARN, non-fatal), so
  no correctness issue; a direct `body_setup` create would be cleaner but this is fine as-is.

---

## Notes for build-master (PASS)

- Safe to commit both scripts + the sidecar in the hygiene pass, and to reuse the same two-step flow
  (commandlet → relaunch → MCP finalize) for **Wall** and **DeepMine** once their baked D/N/ORM textures and
  refined FBX land — both are `category:building` with a `ucx.boxes` footprint, so they route through the verified
  box-collision branch with no code change (add the two CardIDs to `Tools/reimport_cards.txt`).
- The `.uasset` outputs of this wave (`SM_{Sapper,Cleric,Longbowman,Miner,ArrowTower,BallistaTower,Barracks,
  BombTower,CrystalTower,GoldNode}`, 27 `T_*`, 9 `MI_*_PBR`) are LFS-tracked and were saved dirty=false; they are
  the assets to stage. This QA covers the TOOLING only — the asset visual/collision correctness was attested by the
  programmer's MCP readback (14/14) and is not re-verifiable from code review.
- **Carry-forward flags (non-blocking, already documented):** (1) CrystalTower emissive glow not preserved — needs
  an art-director emissive pass (T_CrystalTower_E + emissive-capable master, or a dedicated M_CrystalGlow slot in
  the FBX). (2) Building box hulls are single full-height footprint boxes ("acceptable start" per manifest); any
  building that reads hollow in-engine should be re-hulled wall-footprint-exact to avoid an interior placement
  dead-zone.
- Editor-bounce (save-all → graceful CloseMainWindow → relaunch) is an operator/orchestrator procedure, not code in
  these files; nothing for build-master to compile or guard there.

**Bottom line:** no BLOCKER; the destructive-safety, save-scoping, collision math, category branching, and hygiene
are all correct for UE 5.8. Verdict PASS.
