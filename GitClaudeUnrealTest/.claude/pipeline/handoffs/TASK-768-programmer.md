# TASK-768 — `Tools/reimport_meshes.py` collision path repair (gameplay-programmer)

**Status: ready-for-qa** · file-only · no editor, no MCP, no compile, no Git, script NOT run.
**C++ suite: 279 → 279 (zero C++ touched; this is a Python change).**

Files changed (only these two):
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\reimport_meshes.py`
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Tools\reimport_cards.txt` ← **declared departure, see §6**

---

## 1. I verified the measurement myself, and it HOLDS — plus the mechanism

I did not take the A/B on faith and I did not re-run it (fenced). I verified it two ways.

**(a) At source, in the installed UE 5.8.** The chain, all of it checkable:

| Where | What it says |
|---|---|
| `InterchangeFbxAssetImportDataConverter.cpp:316-317` | `bCollision = bAutoGenerateCollision` and `Collision = bAutoGenerateCollision ? Convex18DOP : None` — the one legacy bool is fanned onto **two** settings |
| `InterchangeLODDataParser.cpp:739` | `CollisionMeshUidsToProcess = bCollision ? CollisionMeshUids : {}` — **false drops every UCX uid** |
| `InterchangeStaticMeshFactory.cpp:796` | `if (ImportAssetObjectData.bImportCollision)` gates **all** collision work |
| `InterchangeStaticMeshFactory.cpp:798` | the Convex18DOP fallback fires **only** `if (!bImportedCustomCollision)` |
| `GeomFitUtils.cpp:114` / `ShapeElem.h:41,128-130` | generators stamp `bIsGenerated=true`; imported hulls keep the default `false` |

**(b) Empirically, in this project's own log** — `Saved/Logs/GitClaudeUnrealTest-backup-2026.09.02-03.17.35.log`:

```
:2712  auto_generate_collision=False   tris=836  hulls=0  is_generated=[]
:2720  auto_generate_collision=True    tris=836  hulls=8  is_generated=[False x8]
:2728  (importer defaults)             tris=836  hulls=8  is_generated=[False x8]
```

**Verdict: CONFIRMED. The flag gates whether collision is processed at all.** The corroborating
detail nobody quoted: the **mesh payload counts** in the same log — `[1 836 558]` at False vs
`[9 928 620]` at True. One payload vs nine. At False the 8 UCX nodes are not ignored downstream,
they are **never fetched**. That is as direct as this gets.

**Two things I found more subtle than the brief stated — reporting rather than making them fit:**

1. **The old comment was stale, not absurd.** In the *legacy* `UnFbx::FFbxImporter`,
   `ImportCollisionModels()` runs unconditionally (`FbxStaticMeshImport.cpp:598`) and the bool
   only gates the KDOP18 fallback (`:2434`) — i.e. the comment was **true of the importer it was
   written against**. UE 5.6+ routes FBX through Interchange instead. Confirmed for this project,
   not assumed: the log shows `LogInterchangeImport` + `InterchangeFbxParser` handling
   `WatchTower.fbx` and **zero `LogFbx` lines**. The comment outlived its importer.

2. **`True` also ARMS the Convex18DOP fallback** (converter `:317`). The brief didn't mention this.
   It cannot touch a mesh that has UCX (`!bImportedCustomCollision`), but a mesh **without** UCX
   would newly gain one fabricated hull. I measured the blast radius across every source FBX
   (`grep -a UCX_` per file) rather than reasoning about it:

   | class | UCX in FBX | what the flag flip changes |
   |---|---|---|
   | units (15) | 0 | nothing — `_apply_unit_hulls` wipes and re-decomposes anyway |
   | box-buildings (Castle 66, ArrowTower/Wall/BombTower/BallistaTower/Barracks/DeepMine/CrystalTower/GoldNode 1 each) | ≥1 | nothing — `_apply_box_collision` overwrites `agg_geom` wholesale |
   | castle-props (GateBanner, TramplePath, ToeRock01/02) | **0** | **would newly gain 1 fabricated hull** |
   | **WatchTower** | **8** | **0 hulls → 8 authored hulls — the fix** |

   So I added `_strip_generated_hulls()`, which drops hulls by `bIsGenerated` in the
   `ucx.boxes: []` branch. Net effect: **the flip is end-state-neutral for all 27 manifested
   cards except WatchTower.** No fleet regression.

---

## 2. Defect 1 — the flag and the comment

`_make_import_options()` now takes `import_collision` and actually **uses** it
(the parameter was previously dead — the body hardcoded `False` while the call site read
`_make_import_options(auto_collision=False)`, so the call site itself was misleading).
The call site passes `import_collision=True`.

The false comment is replaced with the measurement, the payload counts, the log line refs, the
five engine line refs, and an explicit note on the fallback cost. Per `SC-§36` the prose failed
nothing and was therefore silent — so it is now the part carrying the evidence.

## 3. Defect 2 — the unmanifested-CardID default

`_category_of()` **no longer defaults**. Absent entry or absent `category` → `None`, and
`_reimport_one()` hard-STOPs on `None` **before the FBX is touched**, so a refused card leaves
its mesh byte-identical. Belt-and-braces: `_apply_unit_hulls()` now refuses outright if
`category != "unit"`, so the destructive branch cannot be reached by a future edit.

**The control lane TASK-769(4) asks for, and it is not hypothetical here:**
`Castle_Crumble01/02/03` carry **66 authored UCX hulls each**, are named in this script's own
`CASTLE_CLASS_CARD_IDS`, and have **no manifest entry**. Under the old default, listing one in the
sidecar decomposed 66 authored hulls into 4 and reported DONE.

| lane | before | after |
|---|---|---|
| `WatchTower` → building, `boxes: []` | WARN + leaves imported collision (which was **0**) | keeps the **8 authored** UCX hulls |
| `Castle_Crumble01` (no entry) → **the trap** | `"unit"` → `remove_collisions()` + decompose 66→4, **DONE** | **STOP `UNMANIFESTED_CARD`**, nothing touched |

## 4. Was there a third? Yes — and a fourth and fifth. Treating it as the signal it is.

**(3) `_ensure_textures_and_mi()` skipped the TEXTURE import whenever the MI already existed.**
This is the one that would have broken the immediate consumer. `MI_WatchTower_PBR.uasset` **exists
on disk today**, so the very next run would have reimported the geometry, reused the MI, and left
the mesh sampling the **old** `T_WatchTower_*` — exactly the "samples garbage while every readback
reads correct" failure `CONTACT-§13` warns about, given the translation repacked 2508/2508 loop UVs.
Fixed: textures are **always** reimported (`replace_existing=True`, same in-place overwrite the
script is built on); only **MI creation** is now conditional.

**(4) Nothing was ever a STOP.** `result["ok"] = True` was set unconditionally at the end of the
readback; every collision failure was a non-fatal `WARN`. A 0-hull WatchTower would have reported
`DONE`. Now: zero collision on a unit/building/GoldNode, a box count disagreeing with the manifest,
or a failed collision readback each set `ok=False` and print a named STOP, with a batch-level
`_err` roll-up. Decorative categories (`castle-prop`, `ucx: null`) are **exempt** rather than
false-failed.

**(5) Same-path reimport silently RESTORES last build's collision — this answers TASK-768(5) directly.**
`InterchangeStaticMeshFactory.cpp:500-506` stashes the old `AggGeom` and empties it; then at
`:746-792`, **if the new import produced no collision it restores the entire old `AggGeom`
verbatim** (`:753-755`); otherwise it drops the old *imported* elements and re-creates the old
*editor-generated* ones, copying `bIsGenerated=True` back with them (`:760-791`).

> **Can the readback distinguish "from the FBX I just imported" from "survived from last time"?
> YES, and `bIsGenerated` is exactly the discriminator.** After an import that *did* bring
> collision, `bIsGenerated == False` ⇒ from this FBX (old imported elements are dropped);
> `True` ⇒ survivor or fallback.

This also explains the ramp's 14 surviving hulls, and it makes the old flag worse than reported:
`False` guaranteed zero imported collision, which took the **restore** branch — so a building on
the "leave imported collision" path would have shipped **last build's hulls on new geometry** and
read back a healthy non-zero count. The probe only saw `hulls=0` because it called
`remove_collisions()` first (log `:2607`). Worse than zero, and invisible.

**(6) `_load_manifest()` degraded to `{}` on a decode failure** (TASK-769(3)). Now returns `None`
and `main()` halts the whole batch with `MANIFEST_LOAD_FAILED` before importing anything.

## 5. WatchTower-scoped run — confirmed possible

`_resolve_card_ids()` reads `Tools/reimport_cards.txt`, one CardID per line, `#` comments and
blanks skipped, and the sidecar **wins** over `DEFAULT_CARD_IDS`. Replaying that exact parse loop
against the file on disk now yields **`['WatchTower']`** — verified, and WatchTower resolves to
`category='building'`, `boxes=0`, i.e. the keep-the-UCX branch.

Expected acceptance for the run (from the manifest's own `_acceptance`): **8 convex hulls,
`box_count == 0`, `bIsGenerated == False` on all 8.** Zero hulls is now an enforced STOP, not a
log line. The **ladder keeps zero collision** — nothing in this change adds a hull there, and
`_strip_generated_hulls` only ever *removes*.

## 6. ⚠️ DECLARED DEPARTURE — QA please rule

The board's TASK-768 fence says `Tools/reimport_meshes.py` **ONLY**. **I also edited
`Tools/reimport_cards.txt`.** My dispatch prompt explicitly permitted the sidecar "only if strictly
required" and asked me to confirm it can scope a run to WatchTower alone and **"do not widen it
into a fleet sweep."** As shipped it listed **19 CardIDs** — running the script in that state *was*
the fleet sweep the same fence forbids. I narrowed it to `WatchTower` and preserved the 19-card
list as comments. It is a narrowing, not a widening, and reverts with
`git checkout -- Tools/reimport_cards.txt`. **No `pipeline_manifest.json` edit was made.**

## 7. ⚠️ Board note — TASK-769 is covered by this change set

I was dispatched to fix defects 1 **and** 2 together, but the board carries defect 2 as **TASK-769**
(serialized behind this task, *"two agents in one file is how a lost write happens"*). Defect 2 and
TASK-769's items (1)(2)(3) and control-lane (4) are **already implemented here**.
**Do not dispatch a second agent into this file** — please close TASK-769 against this handoff, or
re-scope it, rather than re-implementing.

---

## What QA should scrutinise

1. **The flag flip's blast radius** — my claim is it is end-state-neutral for all 27 manifested
   cards except WatchTower. The argument rests on units/box-buildings overwriting `agg_geom`
   wholesale, and `_strip_generated_hulls` covering the UCX-less props. Re-derive it.
2. **`_strip_generated_hulls` write-back.** `FKConvexElem::VertexData`/`IndexData` are bare
   `UPROPERTY()` (`ConvexElem.h:36-40`) — reflected, so they ride a struct copy, but not
   editor-visible, so I could not *prove* it without running (fenced). The `if stripped:` guard is
   deliberate: the mixed case is unreachable (the fallback only fires when nothing was imported),
   so `kept` is either every hull → **no write at all** (WatchTower's path) or empty → assigns `[]`.
   **WatchTower never takes the write-back.** Confirm you agree the mixed case is unreachable.
3. **The zero-collision STOP's exemption list** (`unit`/`building`/`goldnode` required;
   `castle-prop` exempt). If a decorative category should be required, say so.
4. **Socket mismatch is loud but does NOT set `ok=False`** — I kept `ok` meaning "the reimport ran
   correctly" and gave sockets the `LOD_STEP_FAILED` treatment (named token + `_err` + batch
   roll-up). Escalate to a STOP if you disagree; it is a one-line change.
5. **Textures now always reimport.** Confirm that is wanted for a *fleet* run too, not just this one.

## Socket read-back — verdict: CHEAP, and I built it (say the word and it comes out)

The brief said propose, and build only if trivial. It was trivial and I was already editing the
readback block, so it is in — as `_readback_sockets()`, ~40 lines, no new dependency.

**Why MCP can't and this can, precisely:** `UStaticMesh::Sockets` is declared bare `UPROPERTY()`
with no `EditAnywhere`/`BlueprintReadWrite` (`StaticMesh.h:1500-1501`) — hence MCP's generic
editor-property read failing explicitly. `UStaticMesh::FindSocket` **is**
`UFUNCTION(BlueprintPure)` (`StaticMesh.h:2315-2316`), so a `-run=pythonscript` commandlet reaches
it. This is already proven in this project: the same log printed
`SOCKET LadderFoot = (-450.0, 0.0000, 0.0)`.

It is **data-driven off the manifest's `sockets` block — no hardcoded literals** — and looks
sockets up **by name**, so a *misnamed* socket returns `None` and is reported as MISSING, not just
a mispositioned one. That matters because the consuming C++ reads by literal `FName` and
**degrades open** to fallback literals.

Note the log's `-450 / -150` is the **pre-translation** mesh; the manifest now declares
`LadderFoot (-460,0,0)` / `LadderTop (-160,0,1200)`, consistent with the `(-10,0,0)` translation.
The read-back compares against the manifest with a `SOCKET_TOLERANCE_UU = 0.5` per-axis tolerance
(not zero — a measured `LadderTop` came back `Y=-0.0002`).
