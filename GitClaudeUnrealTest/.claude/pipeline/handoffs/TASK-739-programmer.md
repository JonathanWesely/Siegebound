# TASK-739 — [LADDER-5] `ABP_Footman` climb state — ⛔ **BLOCKED, NOTHING AUTHORED**

**Agent:** gameplay-programmer · **Date:** 2026-09-01 · **Status:** `blocked`
**Law:** `TOWER-§0` · `TOWER-§8.1` · `TOWER-§8.3` · `TOWER-§8.4(B)` · `TOWER-§8.5` (cited, ⛔ not restated)

---

## 0. ⛔ READ FIRST — WHAT THIS TASK DID AND DID NOT DO

| | |
|---|---|
| `/Game/` assets written | ⛔ **ZERO** |
| `ABP_Footman` modified | ⛔ **NO** — not one node, not one variable, not one pin |
| `A_SiegeBiped_Climb` imported | ⛔ **NO** — see §2, and it is ⛔ **not the clip's fault** |
| `SK_Footman_Skeleton` opened/saved | ⛔ **NO** (read-back: `is_dirty == false`) |
| `SM_WatchTower` reimported | ⛔ **NO** (read-back: `is_dirty == false`; still the OLD ramp mesh, TASK-742's job) |
| `L_Arena` / any `.umap` | ⛔ **NOT opened, NOT saved.** PIE never started (`IsPIERunning == false`) |
| C++ / compile / Git | ⛔ **NONE** |
| Editor lifecycle | ⛔ **NOT touched** — not closed, not restarted |

**Everything below is measured in this session — off the files or off the running editor. ⛔ Nothing is relayed from another handoff without an independent check.**

---

## 1. ⭐⭐ THE HEADLINE — TASK-733's CLIP IS **NOT** MALFORMED. THE ENGINE'S ERROR IS MISLEADING.

The editor's Message Log said:

```
Failed to find any bone hierarchy. Try disabling the "Import As Skeletal" option to import as a rigid mesh
Import failed.
```

⛔ **That message is literally true of the code path that emitted it and FALSE about the file.**

### 1.1 What the FBX actually contains — parsed cold, at the byte level

I wrote a binary FBX (v7400) reader and walked `SiegeBiped_Climb.fbx` node by node. ⛔ No trust was placed
in TASK-733's tooling, in its handoff, or in the importer.

| Measured | Result |
|---|---|
| `Model` nodes | **22** — `Footman_Rig` (`Null`) + **21 × `LimbNode`** |
| Bone names | `root · pelvis · spine_01 · spine_02 · spine_03 · neck_01 · head · clavicle_{l,r} · upperarm_{l,r} · lowerarm_{l,r} · hand_{l,r} · thigh_{l,r} · calf_{l,r} · foot_{l,r}` |
| `NodeAttribute` types | `{Null: 1, LimbNode: 21}` |
| `GlobalSettings.CustomFrameRate` | **60.0** |
| `AnimationStack` `LocalStop` | `12,316,308,800` FBX units = **0.266667 s** exactly |
| Keys per curve | **17** (`KeyTime` arrays, 198 of them) ⇒ 16 intervals / 0.266667 s = **60 fps** ✅ |

⇒ ⭐ **THE BONE HIERARCHY IS PRESENT, COMPLETE, AND CORRECTLY TYPED.**

### 1.2 The decisive comparison — against a clip UE has already imported successfully

Same scan, same script, on the shipped `Footman_Walk.fbx`:

| Marker | `SiegeBiped_Climb` | `Footman_Walk` (known-good) |
|---|---|---|
| `LimbNode` / `Skeleton` / `Null` | 42 / 21 / 3 | **42 / 21 / 3** — identical |
| `AnimationStack` / `AnimationLayer` | 2 / 2 | **2 / 2** — identical |
| `AnimationCurveNode` / `AnimationCurve` | 67 / 266 | **67 / 266** — identical |
| FBX version / binary | 7400 / yes | 7400 / yes |
| `Mesh` / `Geometry` / `Deformer` / `Cluster` / `Skin` / `BindPose` / `Pose` | **0 / 0 / 0 / 0 / 0 / 0 / 0** | 3 / 4 / 45 / 21 / 1 / 2 / 29 |

⭐ **The animation and skeleton payloads are structurally IDENTICAL. The ONE difference is the skinned mesh,
which the climb clip deliberately omits** (TASK-733 §1: `object_types={'ARMATURE'}`).

### 1.3 ⇒ WHY THE ERROR SAYS WHAT IT SAYS

UE's legacy FBX importer on the **skeletal-mesh** path (`bImportAsSkeletal = true` /
`MeshTypeToImport = FBXIT_SkeletalMesh`) discovers bones by walking **`Skin` → `Cluster` deformer links off a
`Mesh`**. With no mesh there are no clusters, `SkelMeshArray` comes back empty, and it reports
*"no bone hierarchy"* — **it never looks at the `LimbNode` chain, because on that path it does not need to.**

The **animation-only** path (`MeshTypeToImport = FBXIT_Animation`, `Skeleton` set, `bImportMesh = false`)
maps FBX node names → skeleton bone names directly and **requires no mesh at all**. That is the path
TASK-733 §6 specified, and it is the correct one.

### 1.4 ⭐ AND IT WILL BIND — the name match is exact

Read back off the running editor (`SkeletalMeshTools.get_bone_names` on `SK_Footman`):

```
Footman_Rig, root, pelvis, spine_01, spine_02, spine_03, neck_01, head,
clavicle_l, upperarm_l, lowerarm_l, hand_l, clavicle_r, upperarm_r, lowerarm_r, hand_r,
thigh_l, calf_l, foot_l, thigh_r, calf_r, foot_r          →  22 bones
```

**22/22 exact match with the FBX's 22 `Model` nodes, in the same order.** Also confirmed in the same read:
`SK_Footman`, `SK_Archer`, `SK_Longbowman` and `SK_Wizard` **all** resolve to
`/Game/Characters/SK_Footman_Skeleton` — so `TOWER-§8.1`'s *"one clip, not three"* property holds.

> ### ⚖️ **VERDICT: IMPORT-SETTINGS PROBLEM. ⛔ NOT A RE-EXPORT. ⛔ NOT TASK-733's LANE. The clip is good.**

### 1.5 ⛔ WHAT I REFUSED TO DO

- ⛔ **Did NOT take the engine's suggested fix.** *"Disable Import As Skeletal, import as a rigid mesh"*
  produces a **StaticMesh** — it would have "succeeded" and delivered nothing to drive `ABP_Footman` with.
- ⛔ **Did NOT import against a new skeleton.** That is the one action that silently diverges every unit's
  ABP binding (TASK-733 §6).
- ⛔ **Did NOT request/force a mesh-bearing re-export as a workaround.** `import_file` with a mesh present
  would create a `SkeletalMesh` **bound to `SK_Footman_Skeleton`** — writing to the exact shared asset
  TASK-739 (3) and TASK-733 §6 forbid touching. ⚖️ *A workaround that endangers the fleet's skeleton to
  dodge a settings limitation is a bad trade.*
- ⛔ **Did NOT re-author or substitute a clip.**

### 1.6 One correction to TASK-733 §6's read-back checklist

Check **(d)** says *"the number of bone tracks is **21**"*. The skeleton has **22** bones (`Footman_Rig` is a
real bone, not a discarded armature node) and the FBX carries 22 animated nodes. ⚠️ **Expect 22, or the
read-back will false-alarm on a correct import.** Checks (a)–(c) are right as written.

---

## 2. 🚧 BLOCKER 1 — THE MCP SURFACE HAS **NO** ANIMATION-IMPORT ENTRY POINT

Enumerated all 19 registered toolsets. **Exactly one import tool exists in the whole surface:**
`editor_toolset.toolsets.skeletal_mesh.SkeletalMeshTools.import_file` — and it forces the skeletal-mesh path.
Its `import_animations` flag is a *rider* on a skeletal-mesh import, ⛔ not an animation-only mode.

| Escape route | Probed | Result |
|---|---|---|
| An AnimSequence / animation toolset | `list_toolsets` | ⛔ **Does not exist** |
| Console-command execution (`py`, `Automation`) | `EditorAppToolset` full schema | ⛔ **No such tool** (`SearchCVars` searches, it does not execute) |
| Arbitrary Python in-editor | `ProgrammaticToolset.get_execution_environment` | ⛔ **No.** It only orchestrates *registered tools*; allowed imports are `{re, time, math, datetime, copy, json}` — **no `unreal` module** |
| Pre-configure the factory via reflection | `ObjectTools.list_properties` on `/Script/UnrealEd.Default__FbxFactory` | ⛔ **`ImportUI` is NOT in the reflected list** (only `bCreateNew`, `supportedClass`, `contextClass`, `formats`, `bEditAfterNew`, `bEditorImport`, `bText`, `automatedImportData`, `assetImportTask`, `supportedWorkflows`) ⇒ cannot be set |
| Code-carrying "skills" | `AgentSkillToolset` | ⛔ Text instructions only, no execution |

⭐ **Worth recording, because it names the fix precisely:** `/Script/UnrealEd.Default__FbxImportUI` **does**
expose every field the correct import needs — `meshTypeToImport` (enum incl. **`FBXIT_Animation`**),
`bImportMesh`, `bImportAsSkeletal`, `skeleton`, `bImportAnimations`, `animSequenceImportData`,
`overrideAnimationName`, `animStartFrame`/`animEndFrame`. **The settings object is reachable; the factory
that would consume it is not.**

### 2.1 The three ways out — ⚖️ orchestrator's call, ⛔ not mine

1. ⭐ **A `py` / `-run=pythonscript` route** (the project's own precedent — `MEMORY: UE same-path FBX
   reimport automation`). ⚠️ It is a **second engine process against a live project**, and I hold no grant
   to start one; the editor's lifecycle is the orchestrator's.
2. **Jonathan drags the FBX into `/Game/Characters/Anims/` by hand** and sets the four fields in the import
   dialog (§4.1 gives them verbatim). ⭐ Cheapest, zero risk, ~30 seconds.
3. **Extend the MCP toolset** with an animation import. Correct long-term; not this batch.

⚠️ **This is the same class as the recorded `MCP UMG limits` / `manual Reimport click` debt (TASK-086/087/151).
It is a TOOLING gap, ⛔ not a defect in anybody's deliverable.**

---

## 3. 🚧 BLOCKER 2 — `IsClimbing()` DOES NOT EXIST IN THE RUNNING EDITOR (and the board has a **cycle**)

**Measured, ⛔ not assumed.** `ObjectTools.list_properties` on `/Script/GitClaudeUnrealTest.Default__SummonedUnit`:

- **155 reflected properties.** All the shipped ones are present (`attackDamage`, `attackRange`,
  `attackCadence`, `heightBonusStepUU`, `heightBonusPerStep`, `stuckTuning`, `chargeMultiplier`, …).
- **ZERO climb members.** Filtering the 155 names for `limb` / `adder` returns **`[]`**.
  ⇒ **`ladderClimbSpeedUU` is absent.**

TASK-738 declares it as a reflected property (verified in source, `SummonedUnit.h:1385`):

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Unit|Climb", meta = (ClampMin = "0"))
float LadderClimbSpeedUU = 350.f;
```

and `IsClimbing()` as `UFUNCTION(BlueprintPure)` at `SummonedUnit.h:1050-1051`. **A reflected property that is
absent from the CDO proves the loaded module predates TASK-738.**

> ### ⛔⛔ **A BLUEPRINT CANNOT BIND A `UFUNCTION` THAT IS NOT COMPILED INTO THE LOADED MODULE.**
> There is no node to place, no pin to connect, and no property to read. ⛔ **Inventing a substitute signal
> is forbidden by the dispatch and would be wrong anyway** — it would have to be replaced at the compile.

### 3.1 ⚠️⚠️ THE SCHEDULING DEFECT THIS EXPOSES — **a genuine dependency cycle on the board**

```
TASK-739 (this task)  needs  IsClimbing() compiled
     └─ TASK-741 (QA gate)   blocked-by 734 + 738 + 739
          └─ TASK-742 (the ONE compile)  blocked-by 741
               └─ ...which is what makes IsClimbing() exist.        ⟲
```

⇒ **739 → 741 → 742 → 739.** As boarded, TASK-739 **cannot** complete before TASK-742, and TASK-742 cannot
start before TASK-739. ⭐ **This is not a blocker I can work around — it is a board edit**, and it is the
manager's/orchestrator's to make. The natural resolutions:

- **(a) ⭐ Recommended — split TASK-739 in two.** `739a` = the **import** (art/tooling, no code dependency);
  `739b` = the **ABP wiring**, re-boarded **after** TASK-742's compile, with TASK-741 reviewing 734+738 as it
  already does (⭐ 741's spec *already* says it reviews 739 "from its handoff, ⛔ QA does not open the editor",
  so QA is not actually gated on the ABP existing).
- **(b)** Let TASK-742 compile first and re-run 739 in the same editor session afterwards.

⚠️ **Either way, `TOWER-§8.1`'s warning stands and must be carried forward: a unit gliding up a ladder bolt
upright reads as BROKEN. The ladder must not ship playable until the ABP state lands.**

---

## 4. WHAT IS READY TO EXECUTE — the moment both blockers clear

⭐ Every number below is measured. This section is written so the next run is mechanical.

### 4.1 THE IMPORT (TASK-733 §6, with the settings that make it work)

Source: `Content/RawAssets/Characters/Anims/SiegeBiped_Climb.fbx` → `/Game/Characters/Anims/A_SiegeBiped_Climb`

| Field | Value | Why |
|---|---|---|
| **`MeshTypeToImport`** | ⭐ **`FBXIT_Animation`** | ⛔ **THE ONE FIELD THAT DECIDES SUCCESS.** `FBXIT_SkeletalMesh` is what produced the misleading error |
| **`bImportMesh`** | **false** | there is no mesh, by design |
| **`bImportAsSkeletal`** | **false** | ⛔ or the mesh path runs again |
| **`Skeleton`** | **`/Game/Characters/SK_Footman_Skeleton`** | ⛔ mandatory on the animation path; also what stops a NEW skeleton |
| `bImportAnimations` | true | |
| **`bEnableRootMotion`** | ⭐ **false** | fleet convention — the traversal drives translation, the clip supplies pose |
| Loop | true | it is a cycle |
| Materials / textures | none | |

**Read-back (⛔ never trust the call returning success):**
(a) `GetSkeleton()` → `SK_Footman_Skeleton` · (b) `bEnableRootMotion == false` ·
(c) `SequenceLength ≈ 0.2667 s` **and frame rate 60** — ⚠️ **0.533 s means it came in at 30 fps and RateScale
is wrong by exactly 2×** · (d) bone tracks = **22** (⛔ **not 21** — see §1.6).

### 4.2 ⚠️ `ABP_Footman` HAS **NO STATE MACHINE** — the spec's shape has to be restated

TASK-739 (1) says *"add exactly one **state** to `ABP_Footman`'s **locomotion machine**"* and *"⛔ no touching
walk/idle/**attack** transitions."* **Measured — the asset is not shaped that way.** Its AnimGraph is **5
nodes**, with ⛔ **no state machine and no attack/death nodes at all**:

```
SequencePlayer 'A_Footman_Walk' ──► BlendPose_0 ┐
                                                ├─ BlendListByBool ──► OutputPose
SequencePlayer 'A_Footman_Idle' ──► BlendPose_1 ┤   bActiveValue ← bIsMoving
                                                │   BlendTime_0 = 0.1, BlendTime_1 = 0.1
                          Get bIsMoving ────────┘
```
(`FAnimNode_BlendListByBool` selects child `bActiveValue ? 0 : 1` ⇒ pose 0 = Walk, pose 1 = Idle. ✅ consistent.)

EventGraph (`read_graph_dsl` round-trips K2 fine; ⛔ it returns **empty** for the AnimGraph, so AnimGraph work
must go through `create_node`/`connect_pins`):

```
Event BlueprintUpdateAnimation(DeltaTimeX):
    Speed = VectorLengthXY(GetVelocity(TryGetPawnOwner()))
    SetGroundSpeed(Speed); SetbIsMoving(Speed > 10.0)
```

⇒ **The faithful reading of "exactly one state, no re-layout" on THIS asset is one added selector at the
top of the chain:**

```
NEW SequencePlayer 'A_SiegeBiped_Climb' ──► BlendPose_0 ┐
                                                        ├─ NEW BlendListByBool ──► OutputPose
EXISTING BlendListByBool (walk/idle) ─────► BlendPose_1 ┤   bActiveValue ← NEW bIsClimbing
                                                        │   BlendTime_0 = 0.20, BlendTime_1 = 0.20
                        Get bIsClimbing ────────────────┘
```

**The ONLY edit to an existing node is re-pointing the existing `BlendListByBool`'s output pose from
`OutputPose` to the new node's `BlendPose_1`.** ⛔ Walk/idle logic, `bIsMoving`, and both existing 0.1 s blend
times are untouched. ✅ Verified creatable in the AnimGraph context: `Animation|Sequences|SequencePlayer`
and `Animation|Blends|BlendPosesbybool` both appear in `find_node_types`.

EventGraph addition, **appended** after `SetbIsMoving` (⛔ no re-layout), using a `Cast` with a **failure
branch** so non-`ASummonedUnit` pawns cannot produce an Accessed-None:

```
Cast To SummonedUnit (TryGetPawnOwner())
  ├─ Succeeded → SetbIsClimbing( IsClimbing() )
  │              SetClimbPlayRate( LadderClimbSpeedUU / 337.5 )
  └─ Failed    → SetbIsClimbing(false) ; SetClimbPlayRate(1.0)
```

### 4.3 ⭐ `RateScale` — AS A LIVE FORMULA, ⛔ NOT A BAKED CONSTANT

```
RateScale = LadderClimbSpeedUU / 337.5
```

**The arithmetic, shown as the spec demands:**

| Quantity | Value | Source |
|---|---|---|
| Advance per loop, **ALONG the climb line** | **90.00 uu** | TASK-733 §2 — ⛔ **the along-line pair, ⛔ NOT the vertical 87.31** |
| Loop length | 16 frames @ 60 fps = **0.266667 s** | ✅ re-measured at the FBX (§1.1) |
| **Native along-line speed** | 90.00 / 0.266667 = **337.50 uu/s** | ⭐ the divisor |
| Shipped rate | `LadderClimbSpeedUU = 350.f` | `SummonedUnit.h:1386`, verified |
| **⇒ RateScale at 350** | 350 / 337.5 = **1.037037…** | inside the declared readable band **0.4 – 1.4** |

⛔ **Pairing `LadderClimbSpeedUU` with the VERTICAL 327.42 uu/s gives 1.06898 — a silent 2.99 % foot-skate.**
`LadderClimbSpeedUU` is the speed **along** the pinned line (TASK-738 interpolates Foot→Top at it), so it
pairs with the along-line figure. ⛔ Do not mix them.

**Where it goes: the SequencePlayer node's `Play Rate` pin, driven by the graph — ⛔ NOT `UAnimSequence::RateScale`
on the asset.** Baking it on the asset would (a) be a constant, defeating the whole point, and (b) apply to
every future consumer of the clip.

> ### ⭐ **AND IT IS ONLY AUTHORABLE AS A FORMULA BECAUSE TASK-738 MARKED THE FLOAT `BlueprintReadOnly`**
> (`SummonedUnit.h:1385`). With plain `EditDefaultsOnly` the value would be invisible to Blueprint and the
> only option would have been the baked 1.03704 that the dispatch forbids. **Worth recording as a dependency
> between the two tasks that neither spec stated.**

`337.5` is a property of the **clip**, ⛔ not a tunable: it must land as a named constant
(`ClimbClipNativeSpeedUU`) with a comment citing TASK-733 §2, so a re-export that changes the advance is
caught rather than silently mis-scaling.

### 4.4 ⭐ HOW THE BLEND HANDLES THE 24 uu OFFSET

TASK-733 §7.2: the clip holds the body a **constant 24 uu behind the rung plane** (no drift, no rate
dependence, inside the 34 uu capsule radius). Blending Walk → Climb therefore **moves the mesh 24 uu backward**.

**Setting: `BlendTime_0 = BlendTime_1 = 0.20 s`** on the new selector (the existing walk/idle 0.1 s pair is
⛔ untouched). **Derived, ⛔ not picked:**

| Blend | Apparent drift | Verdict |
|---|---|---|
| 0.10 s | 24 / 0.10 = **240 uu/s** | **69 % of the unit's own 350 uu/s travel** — reads as a sideways slide ⛔ |
| **0.20 s** | 24 / 0.20 = **120 uu/s** | ⭐ **~⅓ of travel speed — reads as the unit leaning into the ladder.** And it is **shorter than one loop** (0.257 s at the shipped rate), so the first stroke lands with the pose at full weight ✅ |
| 0.30 s | 80 uu/s | ⛔ still blending in **after** a full loop ⇒ the first stroke's hand contact is visibly off the rung — worse than the pop |

Symmetric on exit so the dismount resolves the same way. ⛔ **Nothing in the graph compensates the 24 uu** —
forbidden by the dispatch, and see §5 for why it would be wrong anyway.

---

## 5. ⚠️⚠️ THE 24 uu STANDOFF vs THE LANDED MESH — **MEASURED, AND IT DOES NOT MATCH**

TASK-733 §7.2 flagged this exact risk. **I checked it, and the flag was justified.**

### 5.1 How I measured it

The in-editor `/Game/Meshes/SM_WatchTower` is **still the OLD ramp mesh** — read back: bounds
x **−600 → +2458.46**, z → 1420, **14 collision prims**, `AssetImportData` MD5 `8995b131…`. ✅ Consistent
with TASK-737 not having reimported and TASK-742 owning it. ⇒ **The loaded asset is the wrong thing to
measure**, so I parsed the **new** `Content/RawAssets/WatchTower.fbx` (62,076 bytes, 2026-09-01 16:48,
matching TASK-737's declared size) directly.

Frame: `LadderFoot (−450,0,0)` → `LadderTop (−150,0,1200)`; length **1236.932 uu**, lean **75.9638°** —
all three re-derived, ✅ matching `TOWER-§8.3`. Let **m** be the in-plane normal to the climb direction
**pointing away from the tower** (the climber's side): `m = (−0.970143, 0, 0.242536)`.

### 5.2 What the shipped ladder actually measures

FBX is in metres (render node x −4.364 → 3.000 m ⇒ **−436.42 → 300 uu**, matching TASK-737's −436.418 ✅).
Ladder-region vertices (x < −300 uu), signed offset along **m**:

| m-offset | verts | What it is |
|---|---|---|
| **−12 / −13 uu** | 56 | ladder slab **near face** (climber side) |
| **−31 / −32 uu** | 48 | ladder slab **far face** |
| ⇒ **mid-plane** | | ⭐ **m = −22.0 uu**, slab thickness **20 uu** |

Lateral Y values **±66 / ±86** ⇒ **132 uu clear / 172 uu overall** — ✅ exactly TASK-737's declared figures,
which independently validates the parse.

⭐ **The ladder sits 22.0 uu TOWARD the tower, off the pinned line** — precisely TASK-737 §6's declared
*"the ladder plane is offset 22 uu toward the tower so the climbing unit is outboard of the stiles."*
**Independently confirmed, and now quantified with its thickness and faces.**

### 5.3 ⇒ THE CONSEQUENCE

TASK-733 built to `TOWER-§8.3`'s *"the ladder's centreline and the climber's path share one line"*: grips on
the line (**m = 0**), body at **m = +24**.

| | m (uu) |
|---|---|
| Clip's body / `root` | **+24** |
| Clip's grip plane | **0** |
| **Shipped ladder near face** | **−12** |
| **Shipped rung mid-plane** | **−22** |

⇒ ⚠️ **The hands would grip 22 uu short of the rungs (12 uu clear of the nearest ladder surface), and the
body-to-rung distance becomes 24 + 22 = 46 uu against an authored reach of 24 uu.** Both lanes are internally
correct; **they were built to two different readings of the same law.**

### 5.4 ⛔ WHY I DID NOT COMPENSATE IN THE GRAPH — and what the fix is

⛔ The dispatch says check and report. **And a graph-side fix would be wrong on the merits:** the offset is a
constant translation on the `root` bone *inside the clip*; cancelling 22 uu of it would need a per-bone
component-space transform node — **a mechanism where a constant belongs**, invisible to the artist, and it
would silently fight any future re-export.

⭐ **The fix is one constant and a 30-second re-export**, exactly as TASK-733 §7.2 anticipated by name
(`BODY_STANDOFF_M` in `Tools/ArtPipeline/author_climb_anim.py`). **The clip wants a rigid −22 uu shift along m:**

| | before | after |
|---|---|---|
| grip plane | 0 | **−22** (onto the shipped rung plane) |
| `root` / body | +24 | **+2** |
| **body → rung distance** | 24 | ⭐ **24 — UNCHANGED** |

⭐⭐ **Because the body-to-rung distance is preserved, NOTHING needs re-solving** — the reach budget (arm
64.8 / leg 78.1 uu), the elbow 43–112° / knee 41–141° ranges, and the leg's +1.5 uu headroom all carry over
untouched. It is a rigid translation of the whole rig, not a re-authoring.

✅ **And it is safe against the capsule:** the capsule stays centred on the line (r 34), so the ladder at
−12 → −32 is partly inside it — **harmless, because the ladder ships with ZERO collision hulls.**
Independently confirmed in my own parse: all 8 `UCX_` hulls have **min x = −300 uu**, so ⛔ **no hull extends
into the ladder region at all.** (TASK-737 §3 states the same.)

### 5.5 🙋 FOR-MANAGER — ⛔ NOT MY RULING

`TOWER-§8.3` says the ladder's centreline and the climber's path share one line. TASK-737 deliberately moved
the ladder 22 uu off it for a good, stated visual reason (*"a unit stands **at** the foot of a ladder, not
inside it"*) and declared the deviation. **Two consistent resolutions:**

1. ⭐ **Re-export the clip with the −22 uu shift** (my recommendation — cheapest, preserves every measured
   property, and the mesh's reasoning is sound).
2. **Amend `TOWER-§8.3`** to pin the rung-plane offset explicitly, so both lanes build to it and the next
   clip cannot repeat this.

⚠️ **Neither is urgent-blocking:** the offset is **perpendicular to travel**, so it does ⛔ **not** affect the
along-line advance, the native 337.5 uu/s, `RateScale`, the 75.9638° lean, or the loop seam. **§4.3 stands
regardless of how this is resolved.**

---

## 6. WHAT QA / THE NEXT RUN SHOULD SCRUTINISE

1. ⭐⭐ **That nothing was authored is the correct outcome, ⛔ not an omission.** A half-wired climb state in
   `ABP_Footman` — the **entire rigged fleet's only locomotion asset** — would risk every unit in the game
   to save one round trip. §0's read-backs are the evidence.
2. ⭐ **Do not let the engine's error message re-enter the record as "the clip is broken."** §1 refutes it at
   the byte level against a known-good control file. **TASK-733's deliverable is sound.**
3. **Re-check §5's arithmetic** — it is the one finding that changes another lane's work. The measurement is
   reproducible from `Content/RawAssets/WatchTower.fbx` with the frame in §5.1.
4. **The board cycle in §3.1 needs a decision before TASK-739 is re-dispatched**, or it will block again
   identically.
5. ⚠️ **`SequenceLength` after import: 0.2667 s, ⛔ not 0.533 s.** A 30 fps import is a silent 2× RateScale error.
6. ⚠️ **Bone-track read-back expects 22, ⛔ not 21** (§1.6).

---

## 7. DECLARED DEVIATIONS, RESIDUALS AND OBSERVATIONS

1. ⚠️ **`ABP_Footman` is DIRTY IN MEMORY.** I issued ⛔ **no write call** against it — only
   `list_graphs` / `get_parent` / `list_variables` / `read_graph_dsl` / `find_nodes` / `get_node_infos` /
   `find_node_types`. The editor **autosaved** it to `Saved/Autosaves/Game/Characters/ABP_Footman_Auto1.uasset`
   at `00:08:59`. I ⛔ **cannot** distinguish whether a read forced a lazy node reconstruction or the dirt
   predated my session, **so I am declaring it rather than claiming innocence.**
   ✅ **Nothing reached disk:** `Content/Characters/ABP_Footman.uasset` is **unchanged since 2026-07-17
   22:13:43** and `git status` is clean for it.
   🙋 **ACTION: Jonathan should DECLINE saving `ABP_Footman` when he closes the editor.**
2. ⚠️ **A pre-existing Blueprint runtime error in `ABP_Footman`, ⛔ not introduced by me and ⛔ not fixed by me.**
   The log carries repeated
   `ABP_Footman_C /Engine/Transient.World_0:…SkeletalMeshComponent0.ABP_Footman_C_0` +
   `Function …:ExecuteUbergraph_ABP_Footman:64X` — the signature of an Accessed-None in the update event.
   **The cause is visible in §4.2's EventGraph:** `GetVelocity(TryGetPawnOwner())` with no null guard, which
   fires in asset-editor preview worlds where the pawn owner is null. ⛔ Out of scope (TASK-739 (1) forbids
   re-layout) and it is the `SC-§35` 1,806-errors class. **Flagged for a future task, ⛔ not boarded by me.**
3. **TASK-733 §6's check (d) is off by one** — see §1.6. ⛔ Not a defect in the clip.
4. **The in-editor `SM_WatchTower` is still the OLD ramp mesh** (§5.1) — expected, TASK-742 owns the reimport.
   ⛔ I did not touch it. ⚠️ Anyone reading tower geometry from `/Game/` before TASK-742 is reading the ramp.
5. **The failed import left no debris** — `/Game/Characters/Anims/A_SiegeBiped_Climb` and
   `/Game/Characters/A_SiegeBiped_Climb` both read `exists == false`; a `Siege*` search under
   `/Game/Characters/Anims` returns `[]`.
6. **Airlock clean:** ⛔ no `Capture()`/`EnsureSnapshot()`, ⛔ Zone A untouched, ⛔ no token figure, ⛔ no Git,
   ⛔ no compile, ⛔ no `.umap`, ⛔ no editor lifecycle action.

---

## 8. FILES

**Added:** this handoff. **Modified:** `TASKBOARD.md` (TASK-739 status only).
**`/Game/` touched:** ⛔ **none.** **Code touched:** ⛔ **none.**
