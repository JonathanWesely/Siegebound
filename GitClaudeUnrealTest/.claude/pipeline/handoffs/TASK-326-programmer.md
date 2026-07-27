# TASK-326 handoff — fleet mesh-facing DIAGNOSIS (gameplay-programmer)

**Status:** ready-for-qa (diagnose-only) · **Date:** 2026-07-27 · **Lane:** exclusive editor + MCP, read-only
**Verdict:** **CONFIRMED REAL DEFECT — and it is THREE units, not two. Archer, Ogre, AND Wizard.**
**Recommendation: SYSTEMIC fix. The fleet constant is `-90.f`. TASK-327 proceeds.**

Nothing was edited: no C++, no Blueprint, no asset, no Git, no `L_Arena` save. Five temporary
`SkeletalMeshActor`s were spawned into the editor world for measurement and **all five were deleted**
(readback: `remaining: []`). `L_Arena` is left DIRTY-but-content-identical — **do not save it**; a
level reload discards the residue. No BP value was changed at any point, so nothing needed reverting.

---

## 1. The 12-unit yaw table (MEASURED, not inferred)

Read off each `BP_Unit_<Unit>` **component template** — `/Game/Blueprints/Units/BP_Unit_<U>.Default__BP_Unit_<U>_C:<Component>` — via MCP `ObjectTools.get_properties`. Pitch and roll are **0 on all 24 components**; only yaw varies.

| # | Unit | `SkeletalVisualMesh` yaw | `SkeletalVisualMesh` Z | static `VisualMesh` yaw | static `VisualMesh` Z | capsule half-height | renders |
|---|------|-------------------------:|-----------------------:|------------------------:|----------------------:|--------------------:|---------|
| 1 | Footman | **−90** | −90 | −90 | −90 | 90 | ✅ correct |
| 2 | **Archer** | **0** ❌ | 0 | −90 | −90 | 90 | ❌ **sideways** |
| 3 | Knight | **−90** | −90 | −90 | −95 | 95 | ✅ correct |
| 4 | Miner | **−90** | −90 | −90 | −86.5 | 86.5 | ✅ correct |
| 5 | Cleric | **−90** | −90 | −90 | −91 | 91 | ✅ correct |
| 6 | **Ogre** | **0** ❌ | 0 | −90 | −145 | 145 | ❌ **sideways** |
| 7 | Sapper | **−90** | −90 | −90 | −84.5 | 84.5 | ✅ correct |
| 8 | Pikeman | **−90** | −90 | −90 | −95 | 95 | ✅ correct |
| 9 | Cavalry | **−90** | −90 | −90 | −104 | 104 | ✅ correct |
| 10 | MilitiaMob | **−90** | −90 | −90 | −74.5 | 74.5 | ✅ correct |
| 11 | Longbowman | **−90** | −90 | −90 | −92 | 92 | ✅ correct |
| 12 | **Wizard** | **0** ❌ | 0 | **0** ⚠️ | **0** ⚠️ | 88 | ❌ **sideways** |

**Split: 9 units at −90 · 3 units at 0 (Archer, Ogre, Wizard).** The board's premise said "0 on Archer and
Ogre" — the **Wizard is a third case nobody had flagged**, and it is not a coincidence: those three are
exactly the units whose `SkeletalVisualMesh` was **never authored at all**. Their component sits at the
**C++ constructor default `(0,0,0)`** on BOTH yaw and Z. The 9 good units carry a hand-authored
`(0,−90,0)` yaw *and* a hand-authored `−HalfHeight` Z.

Note the Z column: it is the fingerprint of the **same** authoring trap. Archer/Ogre/Wizard Z = 0 is
precisely the "floating unit" TASK-306/307 fixed by deriving Z from the capsule at runtime. **The Z half
of the trap was closed; the yaw half was left open.** Facing is not a second bug — it is the other half
of the bug that was already diagnosed and only half-fixed.

Capsule half-heights are read live per unit (`CapsuleHalfHeight` on `CollisionCylinder`) and are all over
the map — Ogre 145 / Cavalry 104 / Knight 95 / Pikeman 95 / Longbowman 92 / Cleric 91 / Footman 90 /
Wizard 88 / Miner 86.5 / Sapper 84.5 / MilitiaMob 74.5. **Ruling 4 honoured: nothing here assumes 90.**
Grounding is NOT re-opened; the Z column is reported only as corroborating evidence of the shared root cause.

---

## 2. The TRUE baked forward axis — measured directly off the assets

**Answer: every fleet SK bakes its forward along local `+Y`. One forward, all 12, no exceptions.**

This was measured from the **assets themselves**, deliberately independent of any authored component value.

**(a) Shared skeleton confirmed.** `SkeletalMeshTools.get_skeleton` on all 12 `SK_<Unit>` returns
`/Game/Characters/SK_Footman_Skeleton` — **12/12, including the Meshy-new Wizard.** One rig, one skeleton,
so one baked forward is the expected and now-confirmed state.

**(b) The lateral axis is local X (bounds).** `SkeletalMeshTools.get_bounds` (ref-pose local):

| Unit | extent X | extent Y | wider |
|---|---:|---:|---|
| Knight | 71.85 | 35.27 | X (2.04×) |
| MilitiaMob | 55.08 | 27.68 | X (1.99×) |
| Miner | 60.98 | 37.61 | X |
| Pikeman | 95.64 | 65.36 | X |
| Ogre | 125.86 | 98.90 | X |
| Wizard | 64.94 | 49.62 | X |
| Cleric | 48.75 | 39.61 | X |
| Footman | 40.07 | 36.16 | X |
| **Cavalry** | 47.47 | **102.59** | **Y (2.16×)** ← a horse is LONG fore-aft |

Arms/shields/weapons spread along **X** (Knight 2.04×, MilitiaMob 1.99×). The **Cavalry is the decisive
one**: a mounted unit's body is long in the direction of travel, and its long axis is **Y** at 2.16×.
⇒ **X = left/right, Y = fore/aft.** Forward is ±Y, never ±X.

**(c) The generator source says the same.** `Tools/ArtPipeline/rig_character.py:337` — *"`_l` bones at +X
(character LEFT), `_r` at −X (character RIGHT). **Front is −Y**"* (Blender), exported at `:785`
`axis_forward="-Z", axis_up="Y"`, with the header at `:40` asserting *"axis contract IDENTICAL to the
static pipeline so SK faces the same way as SM."* The Blender→FBX→UE chain negates Y (handedness flip),
so Blender's −Y front arrives as UE-local **+Y** while left stays +X / right stays −X — self-consistent
under UE's `Right = Up × Forward` (forward `+Y` ⇒ right `(0,0,1)×(0,1,0) = −X` ⇒ left `+X` ✓).

**(d) EMPIRICAL PROOF, in-engine.** Spawned `SK_Footman`, `SK_Archer`, `SK_Ogre`, `SK_Wizard` as plain
`SkeletalMeshActor`s at **actor yaw 0** — no Blueprint, no component offset, nothing but the raw baked
asset — and captured from a camera parked on **+Y looking −Y**.
→ **All four present their FACE to the camera.** (`TASK-326-bakedforward-from-plusY.png`)
If the baked forward were ±X they would have appeared in profile. It is **+Y**, and the Meshy-new Wizard
bakes identically to the old-pipeline Footman.

**⇒ The one correct component yaw for the whole fleet is `-90`.**
`Rot(θ) · (0,1,0) = (−sinθ, cosθ)`; setting that to actor-forward `(1,0,0)` gives `θ = −90°`. The nine
units at −90 are right; the three at 0 leave the mesh front on **+Y = the actor's RIGHT**, i.e. the model
walks with its left shoulder leading. **That is literally Jonathan's "facing sideways when they walk."**

---

## 3. Which units actually render wrong — verified in the editor

I reproduced the **shipping geometry** exactly: a unit marching along +X has actor yaw 0, so the mesh's
world yaw *is* the authored component yaw. I set five diagnostic actors to their unit's authored yaw
(Footman −90, Knight −90, Archer 0, Ogre 0, Wizard 0) and captured from a camera on the **+X travel axis
looking back down it** — the pose a player sees when a unit marches toward them.
(`TASK-326-shipping-geometry-from-travel-axis.png`, left→right = Wizard, Ogre, Archer, Knight, Footman.)

| Unit | authored yaw | what it shows down the travel axis | verdict |
|---|---:|---|---|
| Footman | −90 | **full front / face + spear + shield** | ✅ faces direction of travel |
| Knight | −90 | **full front / face + sword + shield** | ✅ faces direction of travel |
| Archer | 0 | **profile, head turned 90° to screen-left (+Y)** | ❌ **walks sideways** |
| Ogre | 0 | **side/back, body turned 90° to screen-left (+Y)** | ❌ **walks sideways** |
| Wizard | 0 | **profile, hat + staff in side view, turned to +Y** | ❌ **walks sideways** |

The other 7 units carry the byte-identical `(0,−90,0)` on the byte-identical baked forward, so they are
correct by construction — the same argument that makes the fix a no-op for them (§5).

**Why it is 90° and not 180°:** the mesh front lands on the actor's **right**, so the unit strafes rather
than moon-walks. Because facing is driven by `bOrientRotationToMovement = true` (`SummonedUnit.cpp:104`)
and attack aim is the same actor rotation, the **same 90° error applies identically to marching,
attacking, and death** — the mesh is rigidly 90° off the actor at all times. There is no state in which
these three units look right.

**Note on the earlier "0 / 180 / 270 all occur" screenshot hunt** (the finding that opened this batch):
that was a *camera*-side artifact of hunting a per-unit visual-front offset against a mix of −90 and 0
meshes. The underlying data has only **two** values (−90 and 0) and only **one** baked forward. There is
no third convention and no per-unit mesh divergence.

---

## 4. ROOT CAUSE — manager's read CONFIRMED, with one addition

**Confirmed:** the value is **per-BP authored on the component template**, and C++ never writes it.

- `grep -rn "SetRelativeRotation|SetWorldRotation|AddLocalRotation"` across the whole
  `Source/GitClaudeUnrealTest` module returns **exactly one hit**: `CaptureZone.cpp:41` on a decal.
  **Zero** rotation setters on `VisualMesh` or `SkeletalVisualMesh` anywhere.
- Binary string-scan of all 12 `BP_Unit_*.uasset`: `UserConstructionScript = 0` and
  `SetRelativeRotation = 0` on **every one**. No BP graph touches it either.
- ⇒ The **CDO component template is the sole authoring site**, and my CDO readback reproduces
  build-master's runtime observation exactly (−90 on 9, 0 on Archer/Ogre) — the template *is* the
  runtime value.

**The addition — this is not a new class of bug, it is a HALF-DONE fix.** `ResolveSkeletalVisual`
(`:317-325`) already derives the component's **Z** from the capsule + mesh bounds precisely so no BP has
to hand-author it. It leaves **yaw** alone. The three units that were never hand-authored are exactly the
three that were floating before TASK-307 and are mis-faced now. One trap, two properties, one property fixed.

**Precedent that settles the design question:** the placement ghost **already solved this exact problem
the exact way TASK-327 proposes**, and has shipped that way for months —
`SiegePlayerController.h:700-702`:

```cpp
/** Yaw applied to the ghost so raw SM_<CardID> meshes face +X — the whole family shares SM_Footman's export orientation and -90° fix (TASK-014/037/038 handoffs). */
UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Placement")
float GhostYawOffset = -90.f;
```

An `EditDefaultsOnly float` defaulted to **−90**, applied centrally in C++ (`:1470`), with a comment
already stating that **the whole family shares one export orientation**. That is why the ghost has never
mis-faced — not even the Wizard's, whose static `VisualMesh` is yaw 0. TASK-327 is not inventing a
pattern; it is **applying the project's own shipped, proven pattern to the one component that was left out.**

---

## 5. FIX SPEC for TASK-327

### Site
`ASummonedUnit::ResolveSkeletalVisual`, `SummonedUnit.cpp`, **immediately after the grounding block that
closes at `:325`** and before the AnimClass resolution at `:327`. Same single swap site the grounding fix
owns. No other file changes except the new header property.

### Header — `SummonedUnit.h`, beside `SkeletalVisualMesh` (`:342-343`)

```cpp
/**
 *  Relative YAW the skeletal visual carries so the mesh's baked forward faces ACTOR-FORWARD (+X).
 *  The whole fleet is rigged through one pipeline onto one skeleton (SK_Footman_Skeleton) and bakes
 *  its forward on local +Y, so -90 is correct for every unit — the same constant, for the same reason,
 *  as ASiegePlayerController::GhostYawOffset. This is the ABSOLUTE component yaw, not a delta.
 *  EXCEPTION HATCH: a genuinely differently-baked mesh may override it on its own BP. That is a
 *  NON-DEFAULT requiring an explicit manager ruling (same doctrine as a bespoke skeleton).
 */
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Unit")
float SkeletalVisualYawOffset = -90.f;
```

### Body — the ~4 lines

```cpp
FRotator FacingRot = SkeletalVisualMesh->GetRelativeRotation(); // keep authored pitch/roll
FacingRot.Yaw = SkeletalVisualYawOffset;                        // fleet forward: mesh +Y -> actor +X
SkeletalVisualMesh->SetRelativeRotation(FacingRot);
```

**ABSOLUTE ASSIGNMENT, NOT ADDITIVE — load-bearing.** Adding the offset to the authored yaw would give the
9 good units `−90 + −90 = −180` and regress every one of them. Absolute assignment is the *only*
formulation that is simultaneously a no-op for the 9 and a fix for the 3. Do not write `+=`.

### THE NO-OP ARGUMENT (TASK-327's acceptance gate)

1. **The 9 correct units are byte-identical.** Measured: all 9 carry `(pitch 0, yaw −90, roll 0)` — the
   yaw reads **exactly `−90`**, not `270` or `−90.0001`, so no normalization ambiguity. The fix reads
   that rotator, overwrites `.Yaw` with `−90.f` (the same value), and writes back `(0, −90, 0)`.
   **Input rotator == output rotator, component-wise.** Pitch and roll are read-and-written-back
   untouched — structurally the same preservation the grounding fix gives authored X/Y. A
   `SetRelativeRotation` to the value already held produces no transform delta, so no render change, no
   attached-component shift, no physics/bounds change.
2. **Un-rigged / static-fallback units never reach the code.** Three guards sit above it: `:266`
   (`bUsingSkeletalVisual || !SkeletalVisualMesh || CardID.IsNone()` → return) and `:279-282`
   (`!SkeletalAsset` → return). A unit with no `SK_<CardID>` returns before the grounding block and
   therefore before the new block — **byte-for-byte today's behavior**, and `SkeletalVisualMesh` is
   non-null by construction at that point (the `:266` guard), so the deref is safe.
3. **The 3 broken units are the only behavioral change**, moving from yaw 0 to yaw −90 — the whole point.
4. **Nothing downstream is coupled to this rotation.** The lunge caches `VisualMeshBaseRelativeLocation`
   from the **static** `VisualMesh` at BeginPlay (`:186-190`) — location only, different component.
   `SiegeMeshJuiceComponent` and `SiegeHitFlashComponent` contain **zero** references to `Rotation`/
   `FRotator` (grepped) — juice is scale/location. `MeshJuiceComponent->SetTargetMesh(...)` runs at
   `:1027`, **after** `ResolveSkeletalVisual()` at `:1020`, so ordering is already correct and unchanged.
   The placement ghost is a separate `AStaticMeshActor` owned by `ASiegePlayerController` with its own
   `GhostYawOffset` — untouched.
5. **`AMinerUnit` inherits the base path unchanged** (Miner is a −90 unit ⇒ no-op).

### Do NOT touch
The grounding math (`:317-325`), `VisualMeshBaseRelativeLocation` / the lunge, the static `VisualMesh`
path, the placement ghost, `GhostYawOffset`, any Blueprint, any asset.

### Do NOT "clean up" the three stale BP values
Leave `BP_Unit_{Archer,Ogre,Wizard}` at 0. The C++ overwrites them at swap time, and re-authoring them
per-BP is the exact trap this fix closes. Editing them would also make the fix untestable (it would then
be a no-op for all 12 and prove nothing).

---

## 6. SYSTEMIC vs PER-UNIT — recommendation

**SYSTEMIC, unambiguously.** Ruling 3's escape hatch ("per-unit patching authorized ONLY if TASK-326
proves the baked forwards genuinely differ per unit") **is not triggered**: 12/12 units bind
`SK_Footman_Skeleton`, 12/12 come off one `rig_character.py` export contract, and 4/4 spot-checked
in-engine (spanning both cohorts and both pipeline generations, old-pipeline Footman/Archer/Ogre + new
Meshy Wizard) bake forward on **+Y**. There is one forward and therefore one constant. Per-unit patching
would re-author the three BPs and leave unit #13 to fall into the same hole on its first day — the
grounding regression, replayed.

---

## 7. Out-of-scope findings (reported, NOT fixed here)

1. **`BP_Unit_Wizard`'s static `VisualMesh` is yaw 0 AND Z 0** while all 11 others are −90 / −HalfHeight.
   **Latent only** — the static component is hidden the moment `SK_Wizard` resolves (it does), and the
   placement ghost uses the controller's own correctly-offset actor, not this component. It would only
   surface if `SK_Wizard` ever failed to load, in which case the Wizard would be both mis-faced *and*
   floating 88 cm (the runtime Z derivation is on the SK path only). Worth a follow-up ticket; **not**
   TASK-327's business and **not** a reason to open the BP.
2. `L_Arena` is left dirty from the (deleted) diagnostic actors. **Do not save it.** Content is identical.

---

## 8. What QA should scrutinise

- That §5's no-op claim holds for **all 9** measured `(0,−90,0)` values — the table in §1 is the input.
- That the assignment is **absolute, never additive** (an additive form silently −180s the fleet).
- That the property is `EditDefaultsOnly` with default `-90.f`, not a magic literal at the call site.
- That the early-return guards above the block genuinely protect the un-rigged path.
- That TASK-327 touches **only** `SummonedUnit.{h,cpp}` and nothing in the grounding/lunge/ghost/static paths.
- That the fleet constant in the code matches the one this report measured: **−90**.

## 9. Files / assets referenced

- Read-only C++: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (`:100-106`, `:125-156`, `:186-190`, `:261-368`, `:1020-1030`), `SummonedUnit.h` (`:328-355`), `SiegePlayerController.{h,cpp}` (`:698-702`, `:1460-1500`, `:2551-2575`), `SiegeMeshJuiceComponent.*`, `SiegeHitFlashComponent.*`, `CaptureZone.cpp:41`
- Read-only pipeline: `Tools/ArtPipeline/rig_character.py` (`:30-51`, `:105`, `:337`, `:353`, `:777-812`)
- Read-only assets: `/Game/Blueprints/Units/BP_Unit_<12 units>` (CDO templates), `/Game/Characters/SK_<12 units>`, `/Game/Characters/SK_Footman_Skeleton`
- Evidence: `.claude/pipeline/handoffs/TASK-326-bakedforward-from-plusY.png`, `.claude/pipeline/handoffs/TASK-326-shipping-geometry-from-travel-axis.png`

---

## 10. PROPOSED CONVENTIONS law text — for the MANAGER to ratify and write

> *(Drafted per ruling 5. I have NOT edited CONVENTIONS.md — it is the manager's file. Suggested home: a
> new bullet under "Skeletal rig & animation workstream (M7)", directly after the "SkeletalMeshComponent
> swap contract" bullet at line 307.)*

**— Unit mesh facing (TASK-326/327 — the yaw half of the per-BP authoring trap TASK-306/307 closed for Z):**
Every fleet skeletal mesh is rigged through ONE pipeline (`rig_character.py`, character front authored on
Blender **−Y**, exported `axis_forward="-Z" / axis_up="Y"`) onto ONE shared skeleton
(**`SK_Footman_Skeleton`**, armature root `Footman_Rig`), so **every `SK_<CardID>` bakes its forward on
UE-local `+Y`** — measured in-engine 2026-07-27 across all 12 units (12/12 on the shared skeleton; raw
`SkeletalMeshActor`s at actor yaw 0 all present their front to a `+Y` camera). Actor forward is `+X`,
therefore the **ONE correct relative yaw for `SkeletalVisualMesh` is `-90`**, fleet-wide, with pitch and
roll `0`.
**This value is owned by C++, never by a Blueprint.** `ASummonedUnit::ResolveSkeletalVisual` writes it at
swap time from `UPROPERTY(EditDefaultsOnly) float SkeletalVisualYawOffset = -90.f`, as an **ABSOLUTE
assignment** (an additive offset would rotate an already-authored unit to −180 and is FORBIDDEN),
preserving the component's authored pitch/roll exactly as the grounding fix preserves authored X/Y. This
mirrors the long-shipped `ASiegePlayerController::GhostYawOffset = -90.f`, which solves the identical
problem for the placement ghost and for the same reason: **the whole family shares one export
orientation.** A `BP_Unit_<Unit>` MUST NOT hand-author `SkeletalVisualMesh` rotation — hand-authoring is
what left Archer, Ogre and Wizard at the constructor default `(0,0,0)` and made them walk **sideways**
(mesh front on the actor's right) through marching, attacking and death alike. `SkeletalVisualYawOffset`
is the documented **exception hatch** for a genuinely differently-baked mesh; overriding it is a
**NON-DEFAULT requiring an explicit manager ruling**, exactly like a bespoke skeleton, and any unit that
needs one has almost certainly left the shared rig pipeline by mistake.
**Corollary (the general law):** any per-unit visual offset that is really a property of the SHARED export
contract — Z grounding (TASK-307), yaw facing (TASK-327) — belongs in `ResolveSkeletalVisual`, derived or
constant, **never** hand-authored per `BP_Unit_<Unit>`. A per-BP transform value on `SkeletalVisualMesh`
is a regression waiting for the next new unit.
