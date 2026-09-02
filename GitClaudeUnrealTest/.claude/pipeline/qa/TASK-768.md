# QA Report — TASK-768
**Verdict: PASS** — 0 BLOCKER · 6 WARN · 5 NIT
**The four held files MAY LAND.** (See "The landing ruling" at the bottom for the explicit line.)

Scope: `Tools/reimport_meshes.py` + `Tools/reimport_cards.txt` (the reviewed diff), with
`Tools/ArtPipeline/build_watchtower.py` + `verify_watchtower_fbx.py` swept against the
`Tools/**/*.py` checklist as the other two held files. Reviewed as CODE (2026-07-07 rule).

Fences honoured: no edits · script NOT run · no compile · no engine/MCP · no Git · no TASKBOARD
write (the dispatch forbade it — orchestrator to flip the status).

---

## 0. What I verified independently, and how

I did not review this from the handoff. Three independent sources:

1. **The installed UE 5.8 source.** Every engine line reference in the handoff and in the new
   comments was opened and read. All five check out **at the exact cited line numbers** (table in §1).
2. **The production run's own log** — `Saved/Logs/GitClaudeUnrealTest-backup-2026.09.02-21.31.43.log:1855-1913`.
   This is the FIXED script executing, and it settles several questions empirically that the
   handoff could only argue. (The script ran before its QA gate; that inversion is declared and
   accepted by the dispatch — noted, not charged.)
3. **`pipeline_manifest.json` re-derived from scratch** for the blast-radius claim (§3).

---

## 1. Defect 1 — the mechanism, not just the flag flip. **CONFIRMED AT SOURCE.**

| Cited | Read at that exact line | Verdict |
|---|---|---|
| `InterchangeFbxAssetImportDataConverter.cpp:316-317` | `MeshPipeline->bCollision = StaticMeshImportData->bAutoGenerateCollision;` / `Collision = ... ? Convex18DOP : None` | ✅ exact |
| `InterchangeLODDataParser.cpp:739` | `TArray<FString> CollisionMeshUidsToProcess = GenericMeshPipeline->bCollision ? CollisionMeshUids : TArray<FString>();` | ✅ exact |
| `InterchangeStaticMeshFactory.cpp:796` | `if(ImportAssetObjectData.bImportCollision)` gates the whole fallback block | ✅ exact |
| `InterchangeStaticMeshFactory.cpp:798` | `if(!bImportedCustomCollision && Collision != None)` | ✅ exact |
| `GeomFitUtils.cpp:114` / `ShapeElem.h:41` | `ConvexElem.bIsGenerated = true;` / ctor default `bIsGenerated(false)` | ✅ exact |

One bool → `bCollision` → **the UID list is emptied before anything is fetched**. At `False` the
UCX nodes are *never fetched*, not "fetched and ignored". The payload count is the fingerprint of
that, and it is not only in the A/B probe — **it is in the production run itself**:

```
:1894  LogInterchangeImport: MeshPayload (FBXSDK) of WatchTower.fbx, [9 928 620]
```

1 render payload + 8 UCX payloads, in the run that produced the committed asset.

**Ruling on the nuance — UPHELD, and it is the correct reading.** The old comment was **STALE, NOT
ABSURD**. Pre-Interchange, `UnFbx::FFbxImporter::ImportCollisionModels()` ran unconditionally and
the bool gated only the KDOP18 fallback; the sentence was true of the importer it was written
against and outlived it when UE 5.6+ routed FBX through Interchange. The programmer reported this
instead of making it fit the brief's framing, and that is the right instinct: a comment that was
never true and a comment that stopped being true fail differently, and only the second one tells
you to go looking for the other sentences written in the same era. **`+1 — report the awkward
finding.**

---

## 2. The declared side effect (`True` arms Convex18DOP) — **REAL, and the mitigation is sound**

Confirmed at `InterchangeStaticMeshFactory.cpp:798-861`: the fallback fires only when
`!bImportedCustomCollision`, and each shape case is *additionally* guarded (`Convex18DOP` →
`if(!bHasConvexCollision)`, `:847-852`). `_strip_generated_hulls()` filters on the right property
(`AggregateGeom.h:528-531` — `EmptyImportedElements()` keeps exactly `bIsGenerated == true`, the
same discriminator, used by the engine for the same purpose).

---

## 3. Blast radius — **RE-DERIVED FROM THE MANIFEST, CONFIRMED** (with two caveats, W-3/W-4)

Counted from `pipeline_manifest.json` category keys, not from the handoff's table:

| class | n | `ucx` | what the flip changes | why |
|---|---|---|---|---|
| unit | **13** | `null` ×13 | nothing | `_apply_unit_hulls` → `remove_collisions()` + re-decompose |
| building w/ boxes | **9** (Castle, ArrowTower, Wall, BombTower, BallistaTower, Barracks, DeepMine, CrystalTower, GoldNode) | `boxes:[…]` | nothing | `_apply_box_collision` overwrites `agg_geom` wholesale |
| castle-prop | **4** | `null` ×4 | fabricated hull **stripped** | `ucx:null` → `boxes` resolves `[]` → strip branch |
| **WatchTower** | **1** | `boxes: []` | **0 → 8 authored hulls** | the fix |
| **total** | **27** | | | matches the handoff's 27 |

**End-state-neutral for 26 of 27 — CONFIRMED.** ⚠️ NIT-1: the handoff's table says *"units (15)"*;
the manifest has **13** (13+9+4+1=27). The conclusion is unaffected — every unit takes the same
wipe-and-decompose path whatever the count — but `TL-§5` asks for the count with the scope.

---

## 4. Defect 2 (unmanifested CardID) — **FIXED, fail-closed, and I checked it breaks no workflow**

`_category_of()` returns `None` for an absent entry *or* an absent `category`; `_reimport_one()`
STOPs on `None` **before the FBX is touched** (`:661-669`), so a refused card's mesh is
byte-identical after the run. `_apply_unit_hulls()` re-checks `category != "unit"` and refuses —
belt-and-braces on the one destructive branch, which is the right place to spend a redundant check.

I verified the STOP does not break a documented workflow: **all 10 `DEFAULT_CARD_IDS` and all 19
archived fleet CardIDs are manifested**, and so is `Castle`. The only CardIDs this newly refuses are
`Castle_Crumble01/02/03` — which are named in `CASTLE_CLASS_CARD_IDS` and carry **66 authored hulls
each with no manifest entry**. That is precisely the trap, and refusing them is correct.

`_load_manifest()` → `None` + batch halt: correct, and `main()` still emits a `SUMMARY_JSON` so the
failure is machine-visible rather than just loud. Good.

---

## 5. Defect 3 (stale textures) — **FIXED, and the fix is EXERCISED in the log**

The fix is structural, not incidental: `mi_exists` is captured up front, the three
`_import_texture()` calls run **unconditionally**, and only MI *creation* sits behind
`if mi_exists:`. `_import_texture()` uses `AssetImportTask(replace_existing=True)` over the same
`/Game/Textures/T_<CardID>_*` path — the same in-place overwrite the whole script rests on, so the
MI's texture parameters keep pointing at the same UObjects.

Verified as behaviour, not as intent — the run log shows the exact line that used to be the bug:

```
:1887  [REIMPORT] WatchTower: reimported T_WatchTower_{D,N,ORM}.
:1911  notes: ["MI already exists (…/MI_WatchTower_PBR) -- textures REIMPORTED, MI creation skipped.", …]
```
plus three `LogSavePackage: Moving … -> Content/Textures/T_WatchTower_{D,N,ORM}.uasset` at
`:1865/:1877/:1885`. **The MI existed and the textures still rewrote.** Under the shipped code they
would not have, and the tower would have sampled the pre-repack atlas — `CONTACT-§13` exactly.

⚠️ **WARN-1 lives here — see §9.** The *header* still describes the old behaviour.

---

## 6. Defect 4 (`ok=True` unconditionally) — **FIXED; a zero-hull tower now fails**

`result["ok"] = not stops` (`:842`), with three real STOP tokens (`COLLISION_ZERO`,
`BOX_COUNT_MISMATCH`, `COLLISION_READBACK_FAILED`), a batch `_err` roll-up, a dedicated
`COLLISION_ZERO … Do NOT commit them` line, and `collision_required` scoped to
`unit/building/goldnode` so decorative `ucx:null` cards are **exempt rather than false-failed**.
The exemption list is right as shipped: the props' zero collision is load-bearing
(`_ucx_source`: mount-proof; "a road decal-mesh with collision would perturb the measured entry
chain"). ⚠️ Two gaps in the gate remain: **WARN-3** and **WARN-4**.

---

## 7. Defect 5 — the fresh-vs-survivor chain. **RULING.**

### 7.1 The programmer's conditional claim is CORRECT AT SOURCE
`InterchangeStaticMeshFactory.cpp:757-793` — when the import *did* bring collision, the stash is run
through `AggregateGeom::EmptyImportedElements()`, and I read that function
(`AggregateGeom.h:524-544`): `RemoveAllSwap([](const FKShapeElem& Elem){ return Elem.bIsGenerated == false; })`.
**Every restored *imported* element is physically deleted**; only editor-generated ones are
re-created, and `NewConvexElem = ConvexElem` copies `bIsGenerated = true` back with them. So
*conditional on the import having brought collision*, `bIsGenerated == false` ⇒ from this import.
The shipped code states exactly that conditional at `:782-783`. ✅

### 7.2 The build-master's refinement is CORRECT, and it is the sharper statement of the limit
`bIsGenerated` separates **GENERATED from AUTHORED**, not **FRESH from RESTORED**. A survivor of the
`:753-755` verbatim restore is `false` too, having itself been imported UCX. Therefore the
discriminator is only valid *given* a premise — "the import brought collision" — **and that premise
is not observable anywhere in the readback**. From the readback alone, "8 authored hulls" does not
prove freshness. **⚖️ Both are right and they are not in conflict**: one states the implication, the
other names its antecedent. ⚠️ The handoff's blockquote headline (*"YES, and `bIsGenerated` is
exactly the discriminator"*) drops the antecedent and **must not be copied into CONVENTIONS in that
form**; the code comment carries it, and the code is what lands.

### 7.3 Is the build-master's closed chain conclusive? **YES for this run — and I can close it twice more.**
Its chain (8 UCX in the FBX → 9 payloads fetched → 8 authored hulls out, survivor path structurally
unreachable because it is conditional on the import bringing *no* collision) is valid, and the
antecedent is established **in the production run itself**, not only in the 03:17 A/B probe:
`:1894` `MeshPayload … [9 928 620]` with `:1911` `"hull_is_generated": [false ×8]`.

Two confirmations neither party cited:

- **(a) The sockets are an independent freshness proof that does not involve collision at all.**
  `:1907-1908` read back `LadderFoot (-460.0, 0.0, 0.0)` / `LadderTop (-160.0, -0.0002, 1200.0)` —
  the **TASK-783 coordinates**. The pre-run mesh carried `-450 / -150`. The asset provably took the
  new FBX. ⭐ This is the only row in the entire readback that could have distinguished "new FBX
  landed" from "nothing changed" (see §8).
- **(b) For this asset the question was moot.** TASK-783 declares *"THE BODY AND ALL 8 HULLS DID NOT
  MOVE"*. Restored and fresh hulls would have been **numerically identical**, so the defect-5 hazard
  had zero consequence here even had it fired.

### 7.4 Its declared limit stands — and I extend it
The stated limit (no pre/post hull-vertex diff; `KConvexElem` exposes neither `elem_box` nor
`vertex_data` to Python) is consistent with `ConvexElem.h`, where `VertexData`/`IndexData` are bare
`UPROPERTY()` with no editor visibility. **The deeper limit: no readback can EVER settle freshness
for a same-path reimport whose hull set is unchanged** — the restored and the fresh `AggGeom` are
equivalent by construction. The only durable answers are external to the readback: **CLEAR FIRST**
(which `TL-§2a` already prescribes and this change does **not** implement for the UCX-authority
branch) or an expected-count/hash gate. ⇒ **WARN-5**.

---

## 8. The socket read-back — **RULED IN. Keep it.**

Justification verified at source, both refs exact: `StaticMesh.h:1500-1501` — `Sockets` is a bare
`UPROPERTY()` with no `EditAnywhere`/`BlueprintReadWrite`, so MCP's generic editor-property read
genuinely cannot see it; `StaticMesh.h:2315-2316` — `FindSocket` **is**
`UFUNCTION(BlueprintPure)`, so the commandlet reaches it. This is a capability the commandlet has
and MCP does not, which is the correct reason to put an instrument here.

By-name lookup is the right design: a **misnamed** socket reports MISSING, and the consuming C++
reads by literal `FName` and **degrades open to the fallback literals** — which per the manifest's
`_socket_law` reconstruct the **OLD, standoff-voiding line**. A typo would otherwise be completely
silent.

It earned its keep on the first run: **the hulls could not have detected a failed import (identical
between builds), and the sockets did.** The `0.5 uu` tolerance is empirically justified, not
padding — `LadderTop` came back `Y = -0.0002` (`:1908`).

On the programmer's Q4 (sockets don't set `ok=False`): **accept as shipped** — `ok` meaning "the
reimport ran correctly" is a defensible line, and the `_err` + batch roll-up are loud. ⚠️ But
recommend upgrading to a STOP for cards whose manifest **declares** a `sockets` block: declaring it
*is* the statement that C++ depends on it. One line; board it, don't block on it.

---

## 9. Findings

- **[WARN-1] `Tools/reimport_meshes.py:18-19` — the file header still describes defect 3's
  behaviour. THIS IS A SIXTH DEFECT, AND IT IS DEFECT 1'S OWN CLASS.**
  Shipped text: `1. TEXTURES + MI (units + non-GoldNode buildings, **skipped if MI already
  exists**)`. The parenthetical modifies the whole `TEXTURES + MI` step. The behaviour is now:
  textures **always** reimport; only MI creation is skipped. This is a stale sentence about the
  exact defect just repaired, sitting in the file's operating manual — the first thing the next
  reader reads. It is not cosmetic: an operator who believes it concludes a re-bake cannot ship
  through this script and works around it — plausibly by **deleting the MI to force the path**,
  which destroys the MI's parameter wiring and every reference to it.
  *Fix (house style `HIGH-§1`, quote-don't-erase):* `1. TEXTURES + MI (units + non-GoldNode
  buildings; TEXTURES ALWAYS REIMPORT — only MI CREATION is skipped when the MI exists. The old
  header said "skipped if MI already exists" of the whole step; that was TASK-768 defect 3 and it
  shipped stale textures on a repacked mesh.)`
  **Not a blocker — see the landing ruling — but it must be repaired on the next touch of this
  file, and the next touch is already required by WARN-3/4/5.**

- **[WARN-2] `:430-433` — "the mixed case cannot arise" is over-broad, and the reason given is not
  the reason it holds.** The comment argues from `!bImportedCustomCollision` alone. Two corrections:
  **(a)** the restore-then-fallback composition is closed by a guard the comment does not cite —
  `InterchangeStaticMeshFactory.cpp:848` `case Convex18DOP: if(!bHasConvexCollision)`, evaluated at
  `:805` **after** the `:746-794` restore block, so a mesh that just had hulls restored never gets a
  fabricated one. **(b)** the mixed case *is* reachable by a different route the comment does not
  consider: `:757-793`, where an import that DOES bring UCX coexists with re-created old
  **editor-generated** hulls (`bIsGenerated=true`) ⇒ `kept` non-empty ⇒ the unproven write-back runs
  on authored hulls. Not reachable for today's roster (no SM carries an editor-generated convex
  hull; the run measured `stripped=0`, so the write-back was never taken — as predicted). *Fix:*
  correct the citation and soften "cannot arise" to "requires a pre-existing editor-generated hull
  on the same mesh; none exist in the roster today". Keep the `if stripped:` guard — it is the right
  call for the wrong stated reason.

- **[WARN-3] `:798-806` — the newly-armed fallback can MASK the `COLLISION_ZERO` STOP.** `total =
  convex_hull_count + box_count` counts **fabricated** hulls. Precondition (narrow but real): a
  `collision_required` card with **no prior collision** (so nothing is restored) and **no UCX**, whose
  authoring step then fails or no-ops. Before this change that card STOPped at 0; now the fallback's
  single 18-DOP satisfies the gate and it reports `DONE` with a `COLLISION_GENERATED` warning. *Fix:*
  gate on `authored_hull_count + box_count` for `collision_required` cards, or promote `any(gen)` to
  a STOP for them — **but measure first whether `set_convex_decomposition_collisions` stamps
  `bIsGenerated`** (the `:1309-1377` stamping I read is `AddSimpleCollisions`, not the decomposition
  path), or all 13 units will false-fail.

- **[WARN-4] No symmetric assertion for declared-collisionless cards.** `ucx: null` (13 units +
  4 castle-props) is correctly exempt from `COLLISION_ZERO`, but **nothing STOPs when such a card
  ends up WITH collision** — and this change is exactly what newly enables that. The only mitigation
  is `_strip_generated_hulls()`, whose failure path is a WARN (`:446-449`) on a category that is also
  exempt from the STOP, so a fabricated hull on a `ucx:null` prop can ship green. The manifest calls
  their zero collision load-bearing (mount-proof corollary; nav perturbation). *Fix:* for `ucx: null`
  cards, non-zero collision ⇒ STOP. Two lines, and it makes the gate symmetric.

- **[WARN-5] No expected-hull-count gate; `TL-§2a`'s "CLEAR FIRST" is not implemented on the
  UCX-authority branch.** The manifest's `_acceptance` (*"expect convex_hull_count == 8 … bIsGenerated
  false on all 8"*) is **prose only**; the script asserts non-zero + box count. A reimport whose UCX
  set changed (e.g. the real 14→8 ramp→ladder transition) and whose import brought nothing would
  restore 14 stale hulls and **pass every gate**. This is the residual of defect 5 (§7.4). *Fix (needs
  a follow-up task — the manifest is fenced here):* a defensively-read optional `ucx.expected_hulls`
  (absent → skip), and/or a pre-import `remove_collisions()` on the `boxes: []` branch, which
  converts a UCX bind failure into a loud `COLLISION_ZERO` instead of silent stale hulls.

- **[WARN-6] Textures now ALWAYS reimport — ACCEPTED for the fleet, with a recorded caveat.** The
  script cannot tell which side is stale: it will just as silently regress a shipped texture whose
  `Content/RawAssets/Textures/<CardID>/*.png` is **older** than the `.uasset`. Accepted because the
  RawAssets PNGs are the declared source of truth, are LFS-committed and recoverable with
  `git checkout`, the log names every set it rewrote, and the opposite default is a *silent* failure
  with no recovery signal at all. **Direction-of-staleness is an operator obligation before any
  fleet sweep** — say so in the sidecar when it is re-widened.

- **[NIT-1]** Handoff blast-radius table says "units (15)"; the manifest has 13 (§3). Conclusion
  unaffected. `TL-§5`.
- **[NIT-2] `:203-204`** — `_load_manifest()` still degrades to `{}` when the JSON parses but has no
  top-level `assets` key: `json.load(fh).get("assets", {})`. Harmless now (every card STOPs
  `UNMANIFESTED_CARD`), but it is the exact `{}` shape the new docstring says was eliminated.
  `.get("assets")` + a `None` check closes it and makes the docstring true.
- **[NIT-3] `:178-181`** — no inline-comment stripping: `WatchTower # note` parses as a CardID. Fails
  closed (unmanifested → STOP), so cosmetic.
- **[NIT-4] `:715-717`** — `imported_object_paths` is logged but never asserted; an import that
  produced nothing is not a STOP. Covered empirically here by the sockets (§7.3a), but a card with no
  declared sockets has **no freshness signal at all**. One-line STOP candidate; pairs naturally with
  WARN-5.
- **[NIT-5] `verify_watchtower_fbx.py:83-87`** — the early "render node absent" exit does
  `json.dump(rep, open(OUT, "w"), indent=2)` **before** `OUT.parent.mkdir(parents=True)` at `:210`,
  so on a clean checkout that path raises `FileNotFoundError` instead of the intended FAIL, and leaks
  the handle. Happy path is fine.

---

## 10. The other two held files — `Tools/**/*.py` checklist sweep

Context-only per the dispatch, and their output is already committed (`1231bc4`) and independently
verified, so this is a checklist sweep, not a re-derivation of their geometry.

| check | `build_watchtower.py` | `verify_watchtower_fbx.py` |
|---|---|---|
| **secret handling** | no env reads, no tokens, no argv secrets, no network | same — clean |
| **network timeouts** | n/a — no remote calls | n/a |
| **write confinement** | writes only `Content/RawAssets/WatchTower.fbx`, `Content/RawAssets/Textures/WatchTower/`, `Tools/ArtPipeline/Cache/WatchTower/` — all declared in the docstring's OUTPUTS and all matching the manifest's paths; no path escapes, nothing in another chain's territory | writes only `Tools/ArtPipeline/Cache/WatchTower/roundtrip_report.json` (NIT-5 on the early-exit ordering) |
| **headless-bpy** | `bpy.ops` calls are all data-context ops (`mode_set`, `uv.cube_project`, `pack_islands`, `object.bake`, `render`, `export_scene.fbx`) with the active object set explicitly via `bpy.context.view_layer.objects.active`; no window/screen/area assumptions; re-runs are idempotent (scene rebuilt from scratch) | clears `bpy.data.objects` before importing — idempotent; `--python-exit-code 1` + explicit `sys.exit` |
| **failure surfacing** | top-level `except` prints the traceback and `sys.exit(1)` | per-row PASS/FAIL print + non-zero exit |

✅ Both pass. `verify_watchtower_fbx.py` is a genuinely good artifact — it re-declares the contract
locally *"ONLY so the probe is independent of the build script's own constants — a verifier that
imports the thing it verifies proves nothing"*, and its `climb_delta_uu` row is exactly the
invariant a one-socket edit would break. That is the same instinct as the socket read-back, and it
is the right one.

---

## 11. Is there a sixth? And is the path sound, or merely less broken?

**There is a sixth, and there are arguably three.** WARN-1 is a sixth defect of defect 1's exact
class — a comment that outlived its code, in this same file, describing the very behaviour just
repaired. WARN-3 and WARN-4 are two more ways the gate can still return DONE on a mesh whose
collision it did not itself establish. **Do not treat the count as closed** — this is now the third
consecutive honest look at this file that has raised the count, which is a statement about the
file's test coverage, not about its authors.

**My answer, stated plainly: the path is MATERIALLY SAFER, but it is NOT YET SOUND.**

- ✅ **What is now structurally impossible:** silently *destroying* authored collision. The one
  destructive branch is gated behind an explicit manifest category and refuses twice; a bad manifest
  halts the batch before an import; zero collision on a load-bearing card is a STOP with a named
  token. That is the important half, and it is genuinely closed.
- ⛔ **What is still possible:** silently *keeping or fabricating* collision the script did not
  establish this run — WARN-3 (a fabricated hull satisfies the zero gate), WARN-4 (no upper
  assertion on declared-collisionless cards), WARN-5/NIT-4 (no count gate, no clear-first, no
  import-happened assertion). The failure mode has moved from **destruction** to **false
  attestation**, which is a large improvement and not a finish line.

Sound would mean: *the script cannot report DONE on a mesh whose shipped collision it did not
itself establish in that run.* It cannot yet say that. WARN-3/4/5 + NIT-4 are the closing set, and
they are all small.

---

## 12. Rulings the dispatch asked for

- **The `reimport_cards.txt` departure — ACCEPT** (confirming the lean, not deferring to it). The
  fence's *purpose* was "no fleet sweep"; the file as shipped listed **19 CardIDs** and the sidecar
  **wins** over `DEFAULT_CARD_IDS`, so obeying the fence's letter would have executed the sweep its
  spirit forbade. It is data, not code; it is a **narrowing**; the 19-card list is preserved as
  comments (`HIGH-§1`); it reverts with one `git checkout`; and it was **declared and flagged for
  ruling** rather than buried. Verified live: `:1857` `Using sidecar CardID list … ['WatchTower']`,
  one card processed, `NOT_OK=[]`. The rewritten header is a net gain — it now carries the run's
  acceptance criteria and the "do not add a hull at the ladder foot" law where the next operator
  will actually read them.
- **The pairing (holding the sidecar WITH its script) — CORRECT, and I would have required it.** They
  are one behavioural unit. Sidecar alone = a narrowed scope pointing at the destroyer. Script alone
  = a fixed script whose in-repo default scope is a 19-card fleet sweep. The same logic extends to
  the two ArtPipeline scripts: they authored the FBX and PNGs already committed at `1231bc4`, so
  holding them out leaves **committed assets with no committed generator** — a reproducibility hole
  of the kind this pipeline otherwise refuses. **All four together.**
- **Sockets — RULED IN** (§8).
- **Programmer's Q1 (blast radius)** — re-derived, CONFIRMED, caveated by WARN-3/4 (§3).
  **Q2 (mixed case)** — see WARN-2: right conclusion, incomplete reason.
  **Q3 (exemption list)** — correct as shipped; the gap is the missing *opposite* assertion (WARN-4).
  **Q4 (sockets not `ok=False`)** — accept, with a recommended upgrade for manifest-declared sockets.
  **Q5 (always-reimport for a fleet)** — ACCEPTED with the WARN-6 caveat.
- **TASK-769** — its items (1)(2)(3) and the control lane (4) are implemented and verified here.
  Recommend closing it against this handoff rather than dispatching a second agent into the file.
  **The WARN-3/4/5 + NIT-1..4 set is the natural re-scope for it** (one author, one file, no lost
  write) — and WARN-1 must ride with it.

---

## 13. The landing ruling — explicit

✅ **THE FOUR HELD FILES MAY LAND: `Tools/reimport_meshes.py`, `Tools/reimport_cards.txt`,
`Tools/ArtPipeline/build_watchtower.py`, `Tools/ArtPipeline/verify_watchtower_fbx.py` — as one
commit, no partial landing.**

Why this is a PASS and not a FAIL on WARN-1, stated so the manager can overrule me on the record:
the counterfactual to landing is **not** "no script" — it is "the five-defect script stays at HEAD",
where any future session that checks out and runs it decomposes 66 authored hulls and reports DONE.
A FAIL buys a one-line prose correction at the price of leaving the destroyer in the tree for
another cycle, and none of my six WARNs describes a way this change destroys an asset. The strict
reading is the one I applied to the *behaviour*: 0 blockers there is a measured result, not a
courtesy.

**Conditions on the landing (none block the commit):**
1. **WARN-1 rides on the next touch of this file** — it is one sentence, and this file's whole
   history is a false sentence surviving because it was "not worth a cycle". Board it now.
2. **Do NOT re-widen `reimport_cards.txt` before WARN-3/4/5 are closed.** Every one of them is a
   fleet-scale risk and a single-asset non-risk. The file's own header already says re-widen
   deliberately or not at all — hold it to that.
3. **The build-master's `bIsGenerated` refinement goes into CONVENTIONS `TL-§2a` WITH its
   antecedent** (§7.2): *"`bIsGenerated == false` ⇒ from this import — **only if the import brought
   collision**, which the readback cannot observe."* The unqualified form is the more dangerous
   sentence of the two, and this file has already paid for one of those.

## Notes for build-master

- **No compile, no suite impact.** Python only; C++ suite stays **279**. Editor is DOWN and nothing
  in this review needs it.
- **The evidence for the run is already on disk** and should be quoted in the integration record
  rather than re-derived: `Saved/Logs/GitClaudeUnrealTest-backup-2026.09.02-21.31.43.log`
  `:1857` (sidecar → `['WatchTower']`) · `:1887` + `:1865/:1877/:1885` (textures rewrote with the MI
  present — defect 3 closed in behaviour) · `:1894` (`MeshPayload [9 928 620]` — the antecedent for
  the freshness ruling) · `:1907-1908` (sockets at the **new** −460/−160 — the freshness proof) ·
  `:1909` (`DONE … hulls=8 authored=8 boxes=0 lods=4 lod_group=LargeProp nanite=False`
  refs=`BP_Building_WatchTower` preserved) · `:1911` (`SUMMARY_JSON`, `"stops": []`,
  `hull_is_generated: [false ×8]`, `collision_ucx_preserved(8)`, `fabricated stripped=0`).
- **Known outstanding (not this task's fence):** the mesh's slot→material pointers do not persist
  from a commandlet (documented at `:73-84`); finalize `MI_WatchTower_PBR` on slot `WatchTowerPBR`
  over MCP in the relaunched editor before anyone judges the tower on pixels.
- **Commit all four or none** — see §13.
