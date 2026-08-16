# TASK-586 — [WR-32] THE CASTLE LOD CHAIN, on FOUR meshes (build-master handoff)

**Date:** 2026-08-16 · **Route:** live editor + MCP (⛔ no close, ⛔ no bounce, ⛔ no commandlet) · ⛔ **NO COMMIT, NO PUSH** (TASK-570 owns the commit) · ⛔ **NO WRITE OF ANY KIND**

---

## THE ONE-LINE RESULT

✅ **CONFIRMED NO-OP — reported as a RESULT, not as an absence of work.** ⭐ **All FOUR castle-class meshes already carry the identical chain `lod_count 3 · lod_group "None" · [1.0, 0.4, 0.15] · [28698, 14348, 7174]`.** ⛔ **Nothing was changed, nothing was saved, and `W6-R4`'s prohibition on manufacturing a change was honoured.** ⭐⭐ **Item (5) — the REAL deliverable — PASSES: TASK-567's BOTH-SLOT crumble material assignment SURVIVED intact on all three stages, and so did bounds, LOD0 tris and the 25 collision boxes.**

⭐ **THE STRONGEST SINGLE PIECE OF EVIDENCE: all four `.uasset` SHA256s are BYTE-IDENTICAL to TASK-567's recorded end-state hashes.** ⇒ **Nothing has written to these packages since TASK-567 verified them, so item (5) is proven by file identity, ⛔ not merely by re-reading properties.**

---

## 1. ITEM (1) — THE SUBSYSTEM PROBE

⚠️ **THE LITERAL CALL THE SPEC ASKS FOR IS IMPOSSIBLE, AND THIS IS NOT A NEW FINDING.** `unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)` needs an arbitrary-Python door; **TASK-567 enumerated all 19 toolsets and proved none exists.** ✅ **Re-confirmed independently this session from `ProgrammaticToolset.get_execution_environment`:** importable modules are exactly `frozenset({'time','math','json','copy','datetime','re'})` — ⛔ **`unreal` is not among them.**

⇒ **PROBED BY PROXY, through the subsystem-backed tool, on a DIFFERENT ASSET and in a DIFFERENT SESSION than TASK-567's throwaway (spec's own requirement):**

```
get_lod_thresholds("/Game/Meshes/SM_Castle.SM_Castle")
  → [1, 0.4000000059604645, 0.15000000596046448]
  subsystem_backed_read: "OK"
```

✅ **The subsystem is REACHABLE LIVE. `W6-R4` (A) holds; ⛔ the conditional Jonathan escalation does NOT fire and ⛔ nothing was cross-posted to 🚨 Blockers.**

⚠️⛔ **ONE HONEST LIMIT, STATED RATHER THAN PAPERED OVER: this probe exercises the subsystem's READ path only.** ⛔ **The WRITE path (`set_lod_thresholds`) was deliberately NOT exercised** — the readback matched on all four meshes, so no write was owed, and the only ways to exercise it would have been to manufacture a change on a protected mesh (⛔ forbidden by item (2)/`W6-R4`) or to import a throwaway (⛔ needless disk/git risk for a capability this task never uses). ✅ **TASK-567 already proved the write path on its throwaway; ⛔ I did not re-prove it, and I do not claim it.**

---

## 2. ITEM (2) + (4) — THE FOUR-MESH READBACK, PASTED

| | `SM_Castle` | `Crumble01` | `Crumble02` | `Crumble03` |
|---|---|---|---|---|
| `lod_count` | **3** | **3** | **3** | **3** |
| `lod_group` | **`None`** | **`None`** | **`None`** | **`None`** |
| per-LOD `screen_size` | **`[1.0, 0.4, 0.15]`** | **`[1.0, 0.4, 0.15]`** | **`[1.0, 0.4, 0.15]`** | **`[1.0, 0.4, 0.15]`** |
| per-LOD triangles | **`[28698, 14348, 7174]`** | **`[28698, 14348, 7174]`** | **`[28698, 14348, 7174]`** | **`[28698, 14348, 7174]`** |
| per-LOD vertices | `[35172, 21082, 11194]` | `[35172, 21082, 11194]` | `[35172, 21082, 11194]` | `[35172, 21082, 11194]` |
| `auto_compute_lod_screen_size` | ⚠️ **NOT EXPOSED** — see below | ⚠️ same | ⚠️ same | ⚠️ same |

*(raw floats as returned: `[1, 0.4000000059604645, 0.15000000596046448]` — float32 representation of `0.4` / `0.15`, ⛔ not a divergence.)*

⭐ **`TL-§3`'s DIVERGENCE DEFECT IS ABSENT: four rows in, four rows out, ONE chain.** ⇒ **The "visible LOD pop at a distance that has nothing to do with damage" across a damage-state swap ⛔ cannot occur.** ✅ **This matches `CASTLE_LOD_CHAIN` (`Tools/reimport_meshes.py:119-122`) byte for byte: LOD0 `1.00 @ 1.00` · LOD1 `0.50 @ 0.40` · LOD2 `0.25 @ 0.15` · ⛔ NO LOD3 — the landmark-silhouette law is intact.**

### ⚠️ THE ONE ROW I COULD NOT MEASURE — REPORTED, ⛔ NOT ASSERTED

⛔ **`auto_compute_lod_screen_size` IS NOT READABLE THROUGH THE MCP SURFACE.** **Confirmed twice, independently:** (i) `ObjectTools.get_properties` rejected it — *"the following properties could not be read: autoComputeLODScreenSize"*, on all four meshes; (ii) `ObjectTools.list_properties` returns the **complete** reflected property set for `StaticMesh` and ⛔ **the property is absent from it** (`bAutoComputeLODScreenSize` is editor-only and unreflected). ⛔ **I did not retry a third name-guess against a known-complete enumeration.**

⚖️ **THE INFERENCE, OFFERED AS INFERENCE ONLY: the thresholds read back as EXACTLY `0.4` and `0.15`.** **Auto-computed screen sizes are derived from LOD geometry and do not land on authored round numbers**, so `auto_compute_lod_screen_size = False` is strongly implied. ⛔ **But it is inferred, not measured, and I will not report it as a passed row** (spec item (5): *"report, do not assert"*).

---

## 3. ⭐⭐ ITEM (5) — THE REGRESSION CHECK. **THIS IS THE DELIVERABLE, AND IT PASSES**

⚠️ **The class of defect this exists to catch — a reduction pass quietly re-mapping sections — is invisible in a compile and invisible until the castle takes damage.**

| readback | required | measured |
|---|---|---|
| ⛔ **`Crumble01` BOTH slots** | `MI_Castle_Crumble01` | `TeamRegion` → **`MI_Castle_Crumble01`** · `CastlePBR` → **`MI_Castle_Crumble01`** ✅ |
| ⛔ **`Crumble02` BOTH slots** | `MI_Castle_Crumble02` | `TeamRegion` → **`MI_Castle_Crumble02`** · `CastlePBR` → **`MI_Castle_Crumble02`** ✅ |
| ⛔ **`Crumble03` BOTH slots** | `MI_Castle_Crumble03` | `TeamRegion` → **`MI_Castle_Crumble03`** · `CastlePBR` → **`MI_Castle_Crumble03`** ✅ |
| slot names + order (all four) | `[TeamRegion, CastlePBR]` | **`[TeamRegion, CastlePBR]`** ✅ |
| `SM_Castle` slots (pristine) | un-damaged pair | `TeamRegion` → `MI_TeamColor_Blue` · `CastlePBR` → `MI_Castle_PBR` ✅ |
| bounds (all four) | `7313.576 × 7384.367 × 8082.611` | **identical, all four** ✅ |
| min-Z (all four) | `−0.166` | **`−0.166`** ✅ |
| LOD0 triangles | `28,698` | **`28,698`**, all four ✅ |
| ⛔ **collision box elems** | **25** | **25**, all four ✅ |
| convex / sphere / sphyl / taperedCapsule | 0 | **0 / 0 / 0 / 0**, all four ✅ |
| collision extent | `6720 × 5730 × 2490` | **`6720.0 × 5730.0 × 2490.0`**, all four ✅ |
| `collisionTraceFlag` | `CTF_UseDefault` | **`CTF_UseDefault`**, all four ✅ |
| Nanite | OFF | **`false`**, all four ✅ |
| `refs_before == refs_after` | yes | trio **`[]`** (matches TASK-567); `SM_Castle` → `[/Game/Maps/L_Arena]` ✅ |

⭐ **`WR-§1`'s collision-invariance clause still holds in its strict form — ⛔ not smaller, ⛔ NOT LARGER.** ⇒ **`ApplyCrumbleStage`'s `SetStaticMesh` still carries a `BodySetup` matching the actor-side `GateBlockerVolume` / `InteriorNavModifier`. TASK-567's commit-blocking repair is INTACT.**

📌 **The gate aperture (`1500`) and lintel (`z 1530`) were NOT re-derived — and did not need to be: the four `.uasset` files are byte-identical to the state in which TASK-567 measured them** (§5). ⛔ **File identity is a stronger proof than a re-computation, and re-deriving would have been redundant work.**

---

## 4. ITEMS (3) + (6) — WHAT WAS APPLIED AND WHAT WAS SAVED

⛔ **NOTHING, AND THAT IS THE CORRECT OUTCOME.**

- **(3)** — **The readback matched on every mesh ⇒ ⛔ NO `set_lod_thresholds`, ⛔ NO `generate_lods`, ⛔ NO `lod_group` write, on any of the four.** ✅ **`W6-R4`'s *"do not manufacture a change to prove you worked"* honoured literally.** ⛔ `Tools/reimport_meshes.py` was **read-only** (numbers sourced from `:119-122`) ⇒ ⛔ **no `SC-§27` QA gate is owed by this task.**
- **(6)** — ⛔ **ZERO `save_assets` calls. Nothing was dirtied, so nothing needed saving.** ✅ **All five packages read `dirty: false` AFTER the readback, confirming the reads were non-mutating.**
- ✅ **THE COMMIT SURFACE IS UNCHANGED: this task adds NO new path to TASK-570.** **The four meshes were already `M` from TASK-566/567; they are still `M` and still byte-identical.** ⛔ **No new path appeared — the finding branch of item (6) did not fire.**

---

## 5. 🔒 PROTECTED-ASSET LEDGER

| asset | before | after | |
|---|---|---|---|
| `Content/Maps/L_Arena.umap` | `b3dbc5d9…f8268` | **`b3dbc5d9…f8268`** | ✅ ⛔ never opened, never saved, `dirty: false` |
| `SM_Castle.uasset` | `3c5554e2…eb5a0` | **`3c5554e2…eb5a0`** | ✅ identical (⛔ not mine; its `M` pre-dates me) |
| `SM_Castle_Crumble01.uasset` | `f6d6c51f…c9b4` | **`f6d6c51f…c9b4`** | ✅ identical — **= TASK-567's end state** |
| `SM_Castle_Crumble02.uasset` | `70789fbb…2782` | **`70789fbb…2782`** | ✅ identical — **= TASK-567's end state** |
| `SM_Castle_Crumble03.uasset` | `3d779f0c…ca58` | **`3d779f0c…ca58`** | ✅ identical — **= TASK-567's end state** |

- ⛔ **`Saving Package: /Game/Maps` = 0.** ⛔ **No `.umap` in the diff.** **The nav-save exception was NOT invoked** (it is SPENT/EXPIRED).
- ✅ **`git status` forbidden-pattern sweep (`.gen.cpp` / `Intermediate/` / `.umap` / `Saved/`) = **0**.** ⛔ **No STOP condition.**
- ✅ **`HEAD` = `f205eb5`, `0 0` vs `origin/main`.** ⛔ **No commit, no push, no stage.**
- ✅ ⚠️ **`Content/UI/WBP_WarMap.uasset` still reads `AM` and the index still holds the empty shell — ⛔ UNTOUCHED, exactly as instructed. TASK-570's to fix.**
- ✅ **I added NOTHING to the staged set.** The pre-existing `A `/`AM` entries from TASK-568 and earlier are as I found them.
- 🔒 ✅ **TASK-552's one-shot latch UNSPENT** — ⛔ no PIE, no Simulate, nothing model-side, ⛔ the assistant console was never opened.
- ✅ **⛔ NO EDITOR CLOSE WAS NEEDED OR PERFORMED** — the whole task was a live-MCP readback. ⛔ `CloseMainWindow()` not called, ⛔ `Kill()` not called. **The editor is LEFT RUNNING with MCP live, ready for TASK-593.**
- ✅ ⛔ **No `Tools/` file added or edited. No `pipeline_manifest.json` edit. No C++, no compile** (this task changes no `.cpp`). ⛔ **No Blender, no Meshy, no FAB request. No `.csv`.**
- ✅ ⛔ **Untouched:** `MI_Castle_Crumble0N` · `MI_Castle_PBR` · `T_Castle_*` · `SM_Torch` · `SM_WarTable` · every `.uasset` not named above.

---

## 6. 🚩 FINDINGS — ⛔ none is fixed here

1. ⚠️⭐ **`auto_compute_lod_screen_size` IS UNMEASURABLE OVER MCP** (§2). ⇒ 📌 **Any future spec that lists it as an acceptance row is asking for something this project's live-editor lane cannot deliver** — it needs the commandlet (where `TL-§3` then takes out the subsystem) or a human in the Static Mesh Editor. **Worth a `TL-§` line so it is not re-discovered a fourth time.**
2. ⭐ **`TL-§3`'s inheritance clause is now CONFIRMED BENIGN for this batch, and the mechanism is worth recording as law:** `replace_existing_settings=True` preserves a package's reduction settings across an in-place overwrite, so **an FBX re-derivation re-computes 50 %/25 % against the NEW LOD0 and keeps the authored screen sizes.** ⇒ **`LOD_STEP_FAILED` in a headless run is even more benign than `TL-§3` claims — the chain survives the failure.** 📌 Suggest folding this into `TL-§3` alongside TASK-567's finding 2.
3. 📌 **`SM_Castle` and the trio now differ in exactly ONE intended respect — the material instances** (`MI_TeamColor_Blue`/`MI_Castle_PBR` vs `MI_Castle_Crumble0N`). **Geometry, LODs, bounds and collision are identical across all four.** ✅ **That is precisely what `WR-§1` wants and it is stated here so a future reader does not mistake the material difference for divergence.**
4. ⚠️ **Restating TASK-567 finding 4 because it cost me a call again: `ObjectTools` needs the FULL object path** (`/Game/Meshes/SM_Castle.SM_Castle`). ✅ **Also note the BodySetup sub-object path form that works: `/Game/Meshes/SM_Castle.SM_Castle:BodySetup_0`.**

---

## 7. WHAT I DID NOT DO, ON PURPOSE

- ⛔ **No commit, no push, no stage.** `HEAD` = `f205eb5`, `0 0`. **TASK-570 is the batch's only commit and it may proceed WITHOUT me — I am non-blocking and I did not delay it.**
- ⛔ **No LOD write of any kind** — the chain was already correct on all four and `W6-R4` forbids "fixing" it.
- ⛔ **No save, no save-all** — nothing was dirty.
- ⛔ **No editor close, no bounce, no force-kill.** ⛔ No commandlet re-run. ⛔ No throwaway import.
- ⛔ **No PIE, no Simulate, no assistant console** — TASK-552's latch is Jonathan's to spend.
- ⛔ **No `L_Arena`, no `.cpp`, no `Tools/` edit, no manifest edit.**
