# TASK-567 — [WR-13] THE CRUMBLE TRIO, re-derived from the 9× castle (build-master handoff)

**Date:** 2026-08-15 · **Route taken:** ⛔ **(0b) — the headless `-run=pythonscript` commandlet** · **Editor:** closed gracefully, re-opened, MCP live · ⛔ **NO COMMIT, NO PUSH** (TASK-570 owns the commit)

⚠️ `handoffs/TASK-567-artist.md` is the **record of the block** and is **PRESERVED UNTOUCHED**. This file is additive.

---

## THE ONE-LINE RESULT

✅ **DONE. All three stages re-derived in place, and every acceptance row PASSES — including the commit-blocking one.** ⭐ **`WR-§1` collision INVARIANCE is proven by two independent instruments: the engine-Python commandlet that WROTE the meshes, and an MCP/`ObjectTools` readback of the RELOADED packages in a fresh editor process.** ⭐⭐ **And `TL-§3`'s DIVERGENCE DEFECT DID NOT MATERIALISE — all four castle-class meshes read the SAME chain. Measured, not assumed.**

---

## 1. ⭐⭐ RUNG (0a) — THE MCP TOOL-SURFACE ENUMERATION, ANSWERED AND PASTED

⛔ **THE ANSWER IS NO. There is no arbitrary editor-side Python / `exec` entry point anywhere in the MCP surface.** ⚠️ **This did NOT dissolve the blocker — and it will not dissolve TASK-589's either.** The full surface, so nobody enumerates it a third time:

| toolset | Python door? |
|---|---|
| `ToolsetRegistry.AgentSkillToolset` | no — skill list/read/write |
| `EditorToolset.EditorAppToolset` | ⛔ **no — and this is the important negative: there is NO console-command tool.** `SearchCVars` is READ-ONLY; there is no `exec`/`ExecuteConsoleCommand` ⇒ ⛔ **the `py <file>` console door does not exist** |
| `EditorToolset.LogsToolset` | no |
| `ActorTools` · `SceneTools` · `PrimitiveTools` | no |
| `AssetTools` | ⛔ no. `write_file`/`read_file` are **plain-text only**, sandboxed to `/Game/`, plugin `Content/`, `Saved/`. ⛔ **No reimport tool of any kind** |
| `BlueprintTools` · `CurveTableTools` · `DataAssetTools` · `DataTableTools` · `StringTableTools` | no |
| `MaterialTools` · `MaterialInstanceTools` · `TextureTools` | no |
| `ObjectTools` | ⛔ no — **exactly 6 tools** (`reset_properties`, `set_properties`, `get_properties`, `list_properties`, `get_class`, `search_subclasses`). ⭐ **Independently confirms TASK-568: there is NO call-function/UFUNCTION-invoke tool** |
| `SkeletalMeshTools` | no |
| `StaticMeshTools` | ⛔ no — **exactly 16 tools; `import_file` is the SOLE import door, with no `replace_existing` and NO `reimport` sibling.** ✅ **TASK-567's one-call finding is CONFIRMED by full enumeration, not by a single probe** |
| `ProgrammaticToolset` | ⚠️⛔ **the near-miss, and the one worth recording.** `execute_tool_script` runs Python — but **sandboxed**: importable modules are exactly `frozenset({'json','copy','re','math','datetime','time'})`, ⛔ **`unreal` is NOT among them**, and its only escape hatch `execute_tool()` reaches **the same registered MCP tools**. Its own docstring: *"tool orchestration, not general Python execution."* ⇒ ⛔ **a batcher, NOT a capability upgrade** |

📌 **USEFUL ANYWAY, AND I USED IT AS SUCH:** `ProgrammaticToolset` collapses N round-trips into one. Every readback below was taken through it — including a **3,169-asset dirtiness sweep in a single call**.
⛔ **FOR TASK-589 SPECIFICALLY: (0a) IS ALREADY ANSWERED — do NOT re-enumerate.** `UWidgetTree::RootWidget` needs a property write MCP cannot do and a UFUNCTION MCP cannot call; ⛔ **neither gap is closed by anything above.**

---

## 2. THE ROUTE, AND THE TWO DEFECTS IN THE PRESCRIPTION

⇒ **(0a) failed on enumeration ⇒ took (0b), the commandlet.** ⛔ **(0c) delete+recreate was never approached.**

⚠️⛔ **THE PRESCRIPTION SCRIPT WOULD HAVE CRASHED AS WRITTEN. Two defects, both found by running it, both fixed in a separate runner** (`scratchpad/task567_run.py`; ⛔ **the artist's `task567_crumble_rederive.py` is left byte-unmodified as the record**):

1. ⛔ **`invalidate_physics_data()` IS NOT EXPOSED ON `BodySetup` IN UE 5.8 PYTHON.** Measured — run 1 died on **all three** stages with `'BodySetup' object has no attribute 'invalidate_physics_data'`. ⭐ **The project's OWN proven routine already guards this exact call in a bare `try/except: pass` (`Tools/reimport_meshes.py:335-338`) — the prescription simply dropped the guard.** ✅ **Restored.**
2. ⛔ **`TL-§3` BITES THE BOOTSTRAP, NOT JUST THE LODs.** The prescription reaches `StaticMeshEditorSubsystem` **twice** — `set_lods` (line 101) *and* the `body_setup` bootstrap (line 106) — **both unguarded**, and the subsystem is `None` headless ⇒ an `AttributeError` would have aborted the run **before any collision authoring**. ✅ **Fixed the way the proven code does it: `EditorStaticMeshLibrary` FIRST (it works headless), subsystem as fallback** (`reimport_meshes.py:309-316`). 📌 ⛔ **The bootstrap was NOT "cleaned up"** — it is retained exactly as the manager's note requires, and its convex set is cleared two lines later.
3. ✅ **Acceptance rows the prescription never read (per-LOD tris/verts, min-Z, aperture, lintel, extent, trace flag) were ADDED** — ⭐ **and computed by the SAME code on the pristine `SM_Castle`, so invariance is proven by COMPARISON rather than by reproducing a remembered number.** ✅ **Validation that this works: my reimplementation independently reproduced the artist's pristine figures to the decimal — aperture `1500.0` at `x −732 … +768`, lintel `z 1530.0`.**

⛔ **RUN 1 WAS STATE-NEUTRAL — VERIFIED, NOT ASSUMED:** it threw before `save_asset`, and all five protected SHA256s were **byte-identical afterwards**.

---

## 3. ⭐ ACCEPTANCE — EVERY ROW, MEASURED TWICE

**Instrument A** = engine Python in the commandlet (wrote it). **Instrument B** = MCP `StaticMeshTools`/`ObjectTools` reading the **reloaded packages in a fresh editor process** (`Saved/task567_result.json` + the MCP readbacks).

| readback | required | `SM_Castle` (pristine) | **Crumble01 / 02 / 03** | |
|---|---|---|---|---|
| bounds | `7313.576 × 7384.367 × 8082.611` | `7313.576 × 7384.367 × 8082.611` | **identical, all three** | ✅ |
| *(was)* | *(`2437.859 × 2461.456 × 2694.203`)* | | ⇒ ⭐ **the ⅓ defect is GONE** | ✅ |
| min-Z | `−0.166` | `−0.166` | **`−0.166`** | ✅ |
| `lod_count` | 3 | 3 | **3** | ✅ |
| thresholds | `[1.0, 0.4, 0.15]` | `[1.0, 0.4, 0.15]` | **`[1.0, 0.4, 0.15]`** | ✅ |
| per-LOD tris | `[28698, 14348, 7174]` | `[28698, 14348, 7174]` | **`[28698, 14348, 7174]`** | ✅ |
| per-LOD verts | LOD0 `35,172` | `[35172, 21082, 11194]` | **`[35172, 21082, 11194]`** | ✅ |
| Nanite | OFF | `false` | **`false`** | ✅ |
| **box elems** | **25** | 25 | **25** | ✅ |
| **convex / sphere / sphyl** | **0** | 0 / 0 / 0 | **0 / 0 / 0** | ✅ |
| **collision extent** | **`6720 × 5730 × 2490`** | `6720 × 5730 × 2490` | **`6720 × 5730 × 2490`** | ✅ |
| ⛔ **gate aperture** | **1500**, ⛔ not 500 | `1500.0` (`x −732 … +768`) | **`1500.0` (`x −732 … +768`)** | ✅ |
| ⛔ **lintel bottom z** | **1530**, ⛔ not 510 | `1530.0` | **`1530.0`** | ✅ |
| `collisionTraceFlag` | `CTF_UseDefault` | `CTF_UseDefault` | **`CTF_UseDefault`** | ✅ |
| slots | `[TeamRegion, CastlePBR]` | ✔ | **`[TeamRegion, CastlePBR]`** | ✅ |
| ⛔ **BOTH slots → `MI_Castle_Crumble0N`** | both | *(n/a)* | **both slots `MI_Castle_Crumble0N`, per stage** | ✅ |
| `refs_before == refs_after` | yes | — | **`[]` == `[]`**, all three | ✅ |
| git | ` M` | — | **` M`**, ⛔ never `D` + `??` | ✅ |

⭐⛔ **`WR-§1`'s INVARIANCE CLAUSE IS SATISFIED IN ITS STRICT FORM: the trio's collision is IDENTICAL to the pristine on every field — ⛔ not smaller, and ⛔ NOT LARGER.** ⇒ **`ApplyCrumbleStage`'s `SetStaticMesh` now carries a `BodySetup` that matches the actor-side `GateBlockerVolume` / `InteriorNavModifier` it has to live beside** (`Castle.cpp:179-191`). **The commit-blocking gameplay defect is closed.**

✅ **Spec (3) — the STAGE-LEGIBILITY band is NOT re-opened and I did not touch it.** Re-confirmed from the artifact: `git status --porcelain | grep -c "Content/Textures"` = **0**. ⛔ **The luma protocol was NOT re-run. The TASK-157 spread stands.**

---

## 4. ⭐⭐ `TL-§3` — THE DIVERGENCE DEFECT **DID NOT HAPPEN**, AND HERE IS WHY

⛔ **REPORTED EXACTLY AS OBSERVED, per spec item (6) — the LOD step DID fail, as pre-declared:**

```
LOD_STEP_FAILED: StaticMeshEditorSubsystem unavailable (TL-3, headless).
                 No lod_group write attempted.        [× all 3 stages]
```

✅ **Non-fatal and state-neutral by construction** — options were built first and `lod_group` was **never written** on the failure path (`reimport_meshes.py:378-379` precedent).

⭐⭐ **BUT THE DEBT DID NOT SURVIVE THE FAILURE, AND THIS IS THE FINDING:**

| | `SM_Castle` | Crumble01 | Crumble02 | Crumble03 |
|---|---|---|---|---|
| thresholds | `[1.0, 0.4, 0.15]` | `[1.0, 0.4, 0.15]` | `[1.0, 0.4, 0.15]` | `[1.0, 0.4, 0.15]` |
| tris | `[28698, 14348, 7174]` | `[28698, 14348, 7174]` | `[28698, 14348, 7174]` | `[28698, 14348, 7174]` |

⚖️ **THE MECHANISM: `replace_existing_settings=True` preserves the package's existing build/reduction settings across the in-place overwrite** ⇒ the rebuild re-derived **50.00 % / 25.00 %** against the **new** LOD0 and kept the same screen sizes. **The chain the `set_lods` call would have written is the chain the meshes already carry.**

⇒ ⭐ **ALL FOUR CASTLE-CLASS MESHES CARRY THE SAME CHAIN — so the "visible LOD pop at a distance that has nothing to do with damage" cannot occur.** ⛔ **TASK-586 still OWNS the formal four-mesh readback and I am NOT closing it** — but it should expect to find **convergence, not repair**, and ⛔ **it must not "fix" a chain that is already correct.** 📌 **The four-mesh readback above IS that measurement, taken live over MCP after a fresh boot.**

---

## 5. 🔒 PROTECTED-ASSET LEDGER

| asset | state |
|---|---|
| `Content/Maps/L_Arena.umap` | ✅ SHA256 **`b3dbc5d9…f8268`** — ⛔ **IDENTICAL at four checkpoints**: before close, after close, after the run, at finish. ⛔ Never opened, never saved. `Saving Package: /Game/Maps` count = **0** |
| `SM_Castle.uasset` (⛔ not mine) | ✅ SHA256 **`3c5554e2…eb5a0` UNCHANGED throughout** — read-only. ⚠️ Its ` M` in `git status` **pre-dates me** (TASK-566's rebuild); same hash at session start and finish |
| `SM_Castle_Crumble01` | ⭐ `76947a1e…dc21` → **`f6d6c51f…c9b4`** — ` M` |
| `SM_Castle_Crumble02` | ⭐ `150e14e3…b5c` → **`70789fbb…2782`** — ` M` |
| `SM_Castle_Crumble03` | ⭐ `15f1c9f3…e38b` → **`3d779f0c…ca58`** — ` M` |
| in-editor dirty flags | ✅ all four **`dirty: false`** after the reboot — the saves persisted and load clean from disk |
| `.gen.cpp` / `Intermediate/` / `.umap` / `Saved/` in `git status` | ✅ **ZERO of each** |
| commits / pushes | ✅ **NONE.** `HEAD` still **`f205eb5`**, **`0 0`** vs `origin/main` |
| TASK-552's one-shot latch | ✅ **UNSPENT** — no PIE, no Simulate, nothing model-side |
| `pipeline_manifest.json` | ✅ **READ ONLY** — ⛔ no `Castle_Crumble0N` entries added (TASK-588's question left open) |
| `Tools/reimport_meshes.py` | ✅ **READ ONLY, and ⛔ NEVER POINTED AT THE TRIO** — the manager's refusal holds; it would have `SKIP`ped three times behind a green run |
| `Tools/**/*.py` | ✅ **none added or edited** ⇒ ⛔ **no `SC-§27` QA gate is owed by this task** (the runner is scratchpad-only, TASK-566 R6b precedent) |
| TASK-568's assets | ✅ untouched |

### THE EDITOR CLOSE — ALL THREE LIMITS DISCHARGED

1. ✅ **GRACEFUL ONLY.** `Process.CloseMainWindow()` (posts `WM_CLOSE`) → accepted `True`, exited within timeout. ⛔ **`Kill()` was never called, and never would have been** — the wedge path was a STOP-and-report, not a kill.
2. ✅ **DIRTINESS RE-MEASURED IMMEDIATELY BEFORE CLOSING, BY ME** (`SC-§9` — I did not trust TASK-568's ledger): a **3,169-asset** sweep over `/Game` ⇒ ⭐ **`dirty_count: 0`**. **Nothing was Jonathan's to pick.** 📌 58 `__ExternalObjects__` paths were excluded — stale registry stubs for UE **template** maps (`Fire_Magic`, `Ice_Magic`, `ThirdPerson`, `Variant_*`) that do not resolve to assets on disk; **none are ours** and none can be dirty.
3. ✅ **THIS WINDOW ONLY.** Editor re-opened and left running with MCP live for **TASK-589**.

---

## 6. 🚩 FINDINGS FOR THE BATCH — ⛔ none of these is fixed here

1. ⚠️ **`invalidate_physics_data` is NOT a UE 5.8 Python binding.** Harmless here (`reimport_meshes.py` already guards it, and the collision reads back correct without it), but ⛔ **any future script copying the prescription verbatim will die on all stages.** 📌 Worth a `TL-§` line so it is not re-discovered a third time.
2. ⚠️ **`TL-§3` is broader than its own heading claims.** It is written as an *LOD* limitation, but the same missing subsystem also takes out `set_convex_decomposition_collisions` — i.e. the **`body_setup` bootstrap**. ⇒ 📌 **Suggest amending `TL-§3` to say "`StaticMeshEditorSubsystem` is unavailable headless" rather than "the LOD chain is editor-only"** — the LOD chain is the *symptom* the project happened to hit first.
3. ⚠️ **`Castle.fbx` imports with "nearly zero tangents / bi-normals" warnings** (`LogStaticMesh`, all three stages, `Tolerance 1E-4`). ⛔ **Not a stop** — the pristine `SM_Castle` was built from the same FBX and carries the same geometry; tris/verts match exactly. 📌 Recorded because it is an **export-side** hygiene item that belongs with TASK-588's name-discipline work.
4. ⚠️ **The MCP `ObjectTools` refPath needs the FULL object path** (`/Game/Meshes/SM_Castle.SM_Castle`), not the package path — a bare package path is rejected with *"not a valid object path"*. ✅ **`AssetTools.load_asset` returns the correct ref; use it rather than hand-building.** 📌 Cheap trap, costs one call to hit.
5. ⚠️ **`git status` still carries the auto-staged `A ` entries from TASK-568 and earlier** (`IA_WarMap`, `WBP_WarMap`, `M_Torch`, `M_TorchFlame`, `M_WarTable`, `SM_Torch`, `SM_WarTable`). ⛔ **I added NOTHING to that set** — my three are plain ` M`, unstaged. **TASK-570 still owes the `git diff --cached` reconciliation.**

---

## 7. WHAT I DID NOT DO, ON PURPOSE

- ⛔ **No commit, no push.** `HEAD` = `f205eb5`, `0 0`. **TASK-570 is the batch's only commit.**
- ⛔ No delete, no recreate, no move, no duplicate — **(0c) was never approached.** The overwrite preserved the packages and yielded ` M`, exactly as `SC-§29b` wants.
- ⛔ No write to `SM_Castle` (read-only, hash-proven), no `L_Arena`, no PIE, no Simulate, no C++, no compile (**this task changes no `.cpp`**).
- ⛔ No `Tools/` file added or edited ⇒ no QA gate owed. No `pipeline_manifest.json` edit. No `rescale_refined_fbx.py` re-run.
- ⛔ No touch of `M_CastleCrumble`, `MI_Castle_Crumble0N`, `MI_Castle_PBR`, `T_Castle_*` — **the luma protocol was NOT re-run.**
- ⛔ No save-all — **three explicit-path saves only.** No Blender, no Meshy, no FAB request.
- ⛔ No force-kill. ⛔ No LOD improvisation — the observed chain is reported as observed and the debt is named to TASK-586.
