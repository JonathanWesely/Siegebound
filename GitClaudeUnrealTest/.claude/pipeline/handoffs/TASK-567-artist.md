# TASK-567 — [WR-13] THE CRUMBLE TRIO, re-derived from the 9× castle (art-director handoff)

**Date:** 2026-08-15 · **Editor:** LIVE, MCP up, left running · **Concurrent:** TASK-568 (untouched)

---

## THE ONE-LINE RESULT

⛔ **BLOCKED, AND NOTHING WAS CHANGED.** The defect is **confirmed by measurement** and it is exactly as
specified. The re-derivation itself is blocked on **one** missing capability: ⛔ **MCP's
`StaticMeshTools.import_file` has no `replace_existing` and hard-refuses an existing asset path**, so the
same-path FBX overwrite TASK-351 performed **cannot be issued from a live-editor MCP session at all.**
⭐ **Every OTHER step in the recipe was PROVEN to work over MCP — measured on a throwaway probe, not
asserted** — so the remaining gap is exactly one engine call wide.

⭐⭐ **AND THE QUESTION I WAS SENT TO ANSWER IS ANSWERED, MEASURED: YES — the LOD chain CAN be applied
through `StaticMeshEditorSubsystem` in the running editor.** See §4. ⛔ **I did not touch `SM_Castle`.**

---

## 1. 🔒 PROTECTED-ASSET LEDGER — CLEAN, AND THIS IS THE FIRST THING TO CHECK

| asset | state |
|---|---|
| `Content/Maps/L_Arena.umap` | ✅ SHA256 **`b3dbc5d9…f8268` BEFORE and AFTER** — hash, not mtime. ⛔ Never opened, never saved |
| `SM_Castle_Crumble01.uasset` | ✅ SHA256 **`76947a1e…dc21` UNCHANGED** |
| `SM_Castle_Crumble02.uasset` | ✅ SHA256 **`150e14e3…b5c` UNCHANGED** |
| `SM_Castle_Crumble03.uasset` | ✅ SHA256 **`15f1c9f3…e38b` UNCHANGED** |
| `SM_Castle.uasset` (⛔ not mine) | ✅ SHA256 **`3c5554e2…eb5a0` UNCHANGED** — read-only access only |
| in-editor dirty flags | ✅ **all four `dirty: false`** — the editor's own words, after all work |
| `.gen.cpp` / `Intermediate/` / `.umap` in `git status` | ✅ **ZERO of each** |
| commits / pushes | ✅ **NONE.** `HEAD` still **`f205eb5`**, **`0 0`** vs `origin/main` |
| TASK-552's one-shot latch | ✅ **UNSPENT** — no PIE, no `DumpAssistantPrompt`/`SpikeEval`/`SpikePrompt` |
| `rescale_refined_fbx.py` | ✅ **NOT re-run** (not idempotent; a second run ships a 27× castle) |
| `pipeline_manifest.json` | ✅ **not edited** — the ASCII hold was never tested |
| TASK-568's assets | ✅ untouched. `IA_WarMap.uasset` appeared in `git status` mid-run — **that is TASK-568's, not mine** |

---

## 2. ⛔ THE DEFECT — CONFIRMED BY MEASUREMENT, NOT BY RESTATING THE SPEC

Read live over MCP. **The trio is at exactly ⅓ of the castle in every axis:**

| | `SM_Castle` (rebuilt) | `SM_Castle_Crumble01/02/03` | ratio |
|---|---|---|---|
| **bounds (uu)** | **7313.576 × 7384.367 × 8082.611** | **2437.859 × 2461.456 × 2694.203** (all three identical) | ⛔ **3.0000 / 3.0000 / 3.0000** |
| LOD0 tris | 28,698 | 28,702 | (old mesh) |
| LOD0 verts | 35,172 | 35,184 | (old mesh) |
| lod_count | 3 | 3 | |
| LOD thresholds | `[1.0, 0.4, 0.15]` | `[1.0, 0.4, 0.15]` | |
| Nanite | OFF | OFF | ✅ |
| slots | `[TeamRegion, CastlePBR]` | `[TeamRegion, CastlePBR]` | ✅ |

⇒ ⛔ **At the first 25 % damage tick the castle drops from 7313 × 7384 × 8082 to 2437 × 2461 × 2694 —
a third of its size, on screen, mid-match.** The 2437.859 figure is the CASTLE-3X shipped mesh, so the
trio is a byte-copy of the castle as it stood before TASK-555.

### ⭐ THE COLLISION IS STALE TOO — a number the spec did not have, and it is worse than the visual

I read the `BodySetup` `aggGeom` of all four meshes (`ObjectTools`, camelCase `bodySetup`):

| | `SM_Castle` | the trio (all three identical) | ratio |
|---|---|---|---|
| box elems | **25** | **22** | (the pre-TASK-555 set) |
| convex elems | 0 | 0 | |
| collision extent (uu) | **6720 × 5730 × 2490** | **2240 × 1680 × 830** | x ⅓ · z ⅓ |
| **gate aperture** (narrowest ground-standing front gap) | **1500** | **500** | ⛔ **⅓** |
| **lintel bottom z** | **1530** | **510** | ⛔ **⅓** |
| `collisionTraceFlag` | `CTF_UseDefault` | `CTF_UseDefault` | ✅ |

⛔ **SPEC ITEM (4), ANSWERED WITH A NUMBER: the door gap does NOT survive today.** It is **500 uu** on
every stage against the pristine **1500**. ⚖️ The gate opening law is **≥ 500 × ≥ 450** — so the stage-1
aperture is sitting **exactly ON the floor of the law** while the castle around it is three times larger,
and the lintel drops to **z 510** against a hero who needs ~440 of head height. ⇒ **A crumbled castle is a
sealed castle in practice.** 📌 The y-extent ratio is `5730 / 1680 = 3.41`, **not** 3.0, and that is
correct and expected: TASK-555 **re-derived** the approach (2 apron hulls → 5 treads, +3 hulls, extending
further in −y) rather than scaling it — `WR-§1`.

📌 **Metric note, stated so it is reproducible:** my 1500/500 aperture figure is measured by scanning
**every** ground-standing front-half box, so it lands on the interior `spine_west`/`spine_east` pair
(x −732 … +768) rather than the gate towers. TASK-555's **1560** (x −762 … +798) is the gate-tower gap.
**Both are correct; they measure different apertures.** The ⅓ ratio is identical either way.

---

## 3. ⛔ WHY IT IS BLOCKED — ONE CALL, NAMED, WITH THE ENGINE'S OWN ERROR

The trio needs new **geometry**. Geometry on a `UStaticMesh` is not a `UPROPERTY` — it can only arrive
through an import. Over MCP there is exactly one import entry point, and it refused:

```
StaticMeshTools.import_file(folder_path="/Game/Meshes", asset_name="SM_Castle_Crumble01",
                            source_file=".../Content/RawAssets/Castle.fbx", combine_meshes=true)
  -> ERROR: import_asset: SM_Castle_Crumble01 at /Game/Meshes already exists
```

⛔ **`import_file` exposes no `replace_existing`.** `unreal.AssetImportTask` does
(`replace_existing` / `replace_existing_settings`) — that is precisely how `Tools/reimport_meshes.py:444`
performs the in-place overwrite, and it is **engine Python, not MCP.**

**The three ways around it, and why each is refused:**

1. ⛔ **`AssetTools.delete` + re-import** — this is **delete+recreate**, banned three times over in my spec
   and in the "Castle remaster" same-path law. ⚠️ I note honestly that `ACastle::ApplyCrumbleStage`
   resolves the trio by **path string** (`SiegeGameMode`-style `ResolveStaticMesh("/Game/Meshes/SM_Castle_Crumble0%d")`,
   `Castle.cpp:958-965`) and `get_referencers` returns **`[]`** for all three — so a recreate would
   *probably* be harmless here. ⚖️ **That is exactly why I did not do it: "the references happen to be
   path-based" is a MANAGER RULING to relax a ⛔ law, not an art-director's improvisation at the keyboard.**
2. ⛔ **Import to a temp path + `AssetTools.move` over the original** — same thing wearing a hat; the
   original must be deleted first.
3. ⛔ **Run the `-run=pythonscript` commandlet now** — it needs the editor **closed**. The editor is
   running, **TASK-568 is working inside it**, and the running editor holds all three crumble packages in
   memory: a commandlet writing them on disk would be **silently clobbered** by the next in-editor save.
   That is the project's own recurring failure class. ⛔ **Not taken, and not mine to take** — closing the
   editor would destroy a peer agent's in-flight work.

⇒ ⛔ **I changed nothing.** Landing collision-at-9× or LODs onto a mesh that is still 3× would produce a
**hybrid that is worse than the current bug** — correct hulls floating around a third-size castle. **A
half-done crumble is a new defect, not partial progress.**

---

## 4. ⭐⭐ THE LIVE-EDITOR CAPABILITY REPORT — MEASURED ON A THROWAWAY PROBE

I imported `Castle.fbx` to a scratch path (`/Game/_Scratch567/SM_Task567Probe`), ran the **entire**
recipe on it, measured, and deleted it. ✅ **It was never saved, so it never touched disk** — confirmed:
no `Content/_Scratch567` directory, `find_assets` for `Task567` returns `[]`, and `/Game/Meshes` holds
exactly the four expected castle-family meshes.

| step | tool | result |
|---|---|---|
| import geometry | `StaticMeshTools.import_file` | ✅ **bounds `7313.576 × 7384.367 × 8082.611` — BIT-IDENTICAL to the pristine `SM_Castle`**; min-Z `−0.166`; slots `[TeamRegion, CastlePBR]` in order; Nanite OFF |
| tri/vert count | | **28,697 / 35,171** vs pristine **28,698 / 35,172** — ⚠️ **1 tri / 1 vert short**, see §5 |
| ⭐ **LOD generate** | `StaticMeshTools.generate_lods([0.5, 0.25])` | ✅ **returned `3`** |
| ⭐ **LOD thresholds** | `StaticMeshTools.set_lod_thresholds([1.0, 0.4, 0.15])` | ✅ **`true`**, read back `[1, 0.4, 0.15]`, tris **`[28697, 14348, 7174]`** |
| Nanite | `set_nanite_enabled(False)` | ✅ reads `false` |
| ⭐ **collision authoring** | `ObjectTools.set_properties` on `…:BodySetup_0` | ✅ **`true` — wrote 25 `boxElems`, cleared `convexElems`**; read back `box 25 / convex 0`, extent **6720 × … × 2490**, matching pristine exactly |

### ⇒ THE ANSWER TO THE QUESTION I WAS SENT TO ANSWER

⭐ **YES. `StaticMeshEditorSubsystem` IS available in the running editor over MCP, and the explicit LOD
chain applies cleanly** — `generate_lods` + `set_lod_thresholds` both succeeded and read back at exactly
`[1.0, 0.4, 0.15]` with `[28697, 14348, 7174]` triangles. **TASK-566's `LOD_STEP_FAILED` is a
headless-commandlet limitation only.** ⇒ **The castle's own LOD repair is a two-call MCP job in a live
editor and does NOT need a bounce.**

### ⚠️ BUT THE CASTLE MAY NOT NEED THAT REPAIR AT ALL — I MEASURED IT, AND SAY SO BEFORE IT COSTS A TASK

`SM_Castle` **as it stands right now** reads back:

```
lod_count 3 · thresholds [1.0, 0.4, 0.15] · tris [28698, 14348, 7174] · Nanite OFF
```

`14348 / 28698 = 50.00 %` · `7174 / 28698 = 25.00 %` — ⇒ ⭐ **the reduction chain was rebuilt against the
NEW LOD0 and the screen sizes are already the law's `0.4` / `0.15`.** ⚖️ **And screen size is a ratio of
mesh screen height to viewport height — it is scale-INVARIANT by construction, so "thresholds computed
for the pre-rescale mesh" could not have rotted even in principle.** 📌 **`LOD_STEP_FAILED` was real, but
its consequence was NOT: the "prior chain" the mesh kept is byte-for-byte the chain `CASTLE_LOD_CHAIN`
would have written.** ⛔ **This is a REPORT on an asset I do not own — the disposition is the manager's.**

---

## 5. 🚩 FINDINGS THE NEXT RUN NEEDS — ⛔ none of these is fixed here

1. ⭐⭐ **THE FBX's `UCX_` HULLS *DO* SURVIVE UE 5.8 IMPORT — TASK-566's WarTable finding does NOT
   generalise, and acting as if it did would be a mistake.** My probe imported **25 convex elems**
   straight from `Castle.fbx`'s `UCX_SM_Castle_00..24`, with **zero** manifest authoring. The WarTable
   case differs in the way that matters: its FBX carries a render node **`WarTable`** *and* a node
   **`SM_WarTable`** alongside `UCX_SM_WarTable_00`, so the `UCX_<render-node-name>` binding does not
   resolve; `Castle.fbx`'s render node is **`SM_Castle`** and its hulls are **`UCX_SM_Castle_NN`** — an
   exact match. ⇒ **A plain in-place reimport of `Castle.fbx` restores correct collision on its own**
   (TASK-555 already verified FBX hulls vs manifest: **0 mismatches, all 25 within 0.05 uu**). ✅ Authoring
   the 25 **box** elems afterwards is still worth doing — the pristine mesh carries boxes, not convex
   hulls — but it is now a **parity** step, not a rescue.
2. ⚠️ **MCP's `import_file` generates lightmap UVs and the commandlet does not.** The probe came in with
   `lightMapCoordinateIndex = 1`, `lightMapResolution = 64`; `reimport_meshes.py` sets
   `generate_lightmap_u_vs = False`. **This is the likely source of the 1-tri / 1-vert delta**
   (28,697/35,171 vs 28,698/35,172). ⇒ **Use the commandlet options in §6 for an exact match with the
   pristine mesh** — ⛔ do not chase the delta afterwards.
3. ⛔⛔ **SLOT 1 IS THE TRAP AND IT IS STILL ARMED.** All three crumbles **currently** carry
   `MI_Castle_Crumble0N` on **BOTH** slots (measured — inherited intact from TASK-351), so spec item (2)
   is satisfied *today*. ⚠️ **A reimport is expected to reset slot 1**, and `ApplyCrumbleStage` writes
   **slot 0 only** ⇒ a half-crumbled castle — crumble on the roofs, pristine on the walls — **with no
   error logged.** **Re-assert both slots after the import and read them back.**
4. ⚠️ `set_nanite_enabled` returned `null` rather than a boolean (the mesh was already OFF and reads OFF).
   Cosmetic; noted so nobody reads it as a failure.

---

## 6. ➡ THE PRESCRIPTION — one command, editor closed, ⛔ NOT run by me

A ready-to-run script is at
`C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\5ae71072-bb8f-4659-87cc-ab1ab1e2cc1e\scratchpad\task567_crumble_rederive.py`
— **scratchpad only, deliberately NOT in `Tools/`** (TASK-566 R6b precedent: not shipped tooling, owes no
QA gate). **It has NOT been executed.** It does, per stage, in this order (⚠️ **order is load-bearing —
the import wipes collision, LODs and materials, so authoring must FOLLOW it**):

1. `AssetImportTask(replace_existing=True, replace_existing_settings=True, automated=True)` over
   `/Game/Meshes/SM_Castle_Crumble0N` from `Content/RawAssets/Castle.fbx`, with
   `combine_meshes=True`, `generate_lightmap_u_vs=False`, `auto_generate_collision=False`.
2. Nanite OFF.
3. `StaticMeshEditorSubsystem.set_lods` with `CASTLE_LOD_CHAIN` (`lod_group="None"` first).
4. The 25 manifest boxes onto `body_setup.agg_geom.box_elems`, convex/sphere/sphyl cleared,
   `CTF_USE_DEFAULT`, `invalidate_physics_data()`.
5. ⛔ **`MI_Castle_Crumble0N` onto BOTH `static_materials` entries.**
6. Save by explicit path. ⛔ **Never save-all** — `L_Arena` must not move.

**⚖️ WHO SHOULD RUN IT.** It needs the editor closed, which is **not mine to do while TASK-568 is live**.
Two clean options, both cheap — ⭐ **and note that (b) is now possible only because `import_file` is the
sole gap; everything downstream of it is proven MCP-callable:**
- **(a)** Re-dispatch **TASK-567 to me** once TASK-568 is finished and I have the editor to myself, or
- **(b)** fold it into **TASK-569** (build-master already owns an editor bounce and the commandlet lane).

### ACCEPTANCE — the exact numbers to demand, all measured this session

| readback | required |
|---|---|
| bounds, all three | **7313.576 × 7384.367 × 8082.611** (from **2437.859 × 2461.456 × 2694.203**) |
| min-Z | **−0.166** |
| lod_count / thresholds | **3** / **`[1.0, 0.4, 0.15]`** |
| tris per LOD | **`[28698, 14348, 7174]`** |
| verts LOD0 | **35,172** |
| box elems | **25**, convex **0** |
| collision extent | **6720 × 5730 × 2490** |
| ⛔ **gate aperture** | **1500** (mine) / **1560** at the gate towers, x −762 … +798 (TASK-555) — ⛔ **not 500** |
| ⛔ **lintel bottom z** | **1530** — ⛔ not 510 |
| slots | **`[TeamRegion, CastlePBR]`**, ⛔ **BOTH → `MI_Castle_Crumble0N`** |
| Nanite | **OFF** |
| git | `SM_Castle_Crumble0N.uasset` shows **` M`**, ⛔ never `D` + `??` |

---

## 7. ✅ SPEC ITEM (3) — THE STAGE-LEGIBILITY BAND IS **NOT** RE-OPENED, AND HERE IS THE PROOF

**Stated explicitly so no future session burns a task re-running the luma protocol.** The crumble stage
check is **BASE-RELATIVE** — it re-opens only on a **brightness change to `T_Castle_D`**. Verified from
the artifact, not relayed: `git status --porcelain | grep -c "Content/Textures"` returns **`0`** — no
castle texture is modified, added or staged anywhere in this batch, and TASK-566 independently recorded
its texture/MI step as **skipped by design**. ⇒ ⛔ **The TASK-157 spread (S1 `0.80/0.12`, S2 `0.50/0.45`,
S3 `0.30/0.80`) STANDS UNCHANGED, and TASK-351's full-pass band — S1 `0.7735` · S2 `0.5163` ·
S3 `0.3737`, gaps `0.2572`/`0.1426` — is still the live evidence.** ⛔ **Do not re-run the luma protocol.**
⛔ **`M_CastleCrumble`, `MI_Castle_Crumble01/02/03`, `MI_Castle_PBR` and `T_Castle_*` were all untouched.**

---

## 8. WHAT I DID NOT DO, ON PURPOSE

- ⛔ No delete, no recreate, no move, no duplicate of any shipped asset.
- ⛔ No write of any kind to `SM_Castle` — ⛔ not mine (`parallel-safe: no`); read-only throughout.
- ⛔ No `L_Arena`, no PIE, no Simulate, no C++, no Git, no commit, no push.
- ⛔ No `rescale_refined_fbx.py` re-run; no `pipeline_manifest.json` edit (ASCII hold untested).
- ⛔ No touch of `IA_WarMap` / `WBP_WarMap` / `IMC_Hero` — TASK-568's lane.
- ⛔ No Blender, no Meshy, no generation, no re-bake, no FAB request.
- ⛔ No editor close, no save-all; the editor is **left running with MCP live**.
- ⛔ No retry loop — `import_file` failed **once**, on a capability gap, and I stopped and reported.
