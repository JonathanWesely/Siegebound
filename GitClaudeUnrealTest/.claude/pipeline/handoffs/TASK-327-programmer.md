# TASK-327 handoff — systemic unit mesh-facing FIX (gameplay-programmer)

**Status:** ready-for-qa · **Date:** 2026-07-27 · **Lane:** file-only (no editor, no MCP, no compile, no Git)
**Implements:** the fix spec in `.claude/pipeline/handoffs/TASK-326-programmer.md` §5, verbatim.

**Files touched — exactly two, nothing else:**
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h` (+24 lines: one new `UPROPERTY` + its doc block)
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` (+28 lines: 3 statements + 25 comment lines, inside `ResolveSkeletalVisual`)

No Blueprint, no asset, no `.uasset`, no `CONVENTIONS.md`, no `TASKBOARD.md`, no Git operation, no build. The
editor was never opened. `L_Arena` was not touched (TASK-326 left it dirty-but-identical — still do not save it).

---

## 1. THE DIFF

### `SummonedUnit.h` — new property, immediately after `SkeletalVisualMesh` (was `:342-343`)

```diff
 	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Siegebound|Unit")
 	TObjectPtr<USkeletalMeshComponent> SkeletalVisualMesh;

+	/**
+	 *  Relative YAW the skeletal visual carries so the mesh's baked forward faces ACTOR-FORWARD (+X)
+	 *  (TASK-326/327 — the yaw half of the per-BP authoring trap TASK-306/307 closed for Z).
+	 *  The whole fleet is rigged through ONE pipeline (Tools/ArtPipeline/rig_character.py, character
+	 *  front authored on Blender -Y) onto ONE shared skeleton (SK_Footman_Skeleton), so every
+	 *  SK_<CardID> bakes its forward on UE-local +Y — measured in-engine across all 12 units
+	 *  (12/12 on the shared skeleton; raw SkeletalMeshActors at actor yaw 0 all present their front
+	 *  to a +Y camera). Rot(θ)·(0,1,0) = (1,0,0) ⇒ θ = -90, so -90 is correct for EVERY unit — the
+	 *  same constant, for the same reason, as ASiegePlayerController::GhostYawOffset, which has
+	 *  solved the identical problem for the placement ghost since TASK-014/037/038.
+	 *
+	 *  This is the ABSOLUTE component yaw, NOT a delta: ResolveSkeletalVisual OVERWRITES the
+	 *  component's yaw with it (an additive offset would rotate the already-authored -90 units to
+	 *  -180 and is FORBIDDEN). Pitch and roll are preserved exactly, as the grounding fix preserves
+	 *  the authored X/Y. A BP_Unit_<Unit> MUST NOT hand-author SkeletalVisualMesh rotation — that is
+	 *  what left Archer/Ogre/Wizard at the constructor default 0 and made them walk sideways.
+	 *
+	 *  EXCEPTION HATCH: a genuinely differently-baked mesh may override this on its own BP. That is
+	 *  a NON-DEFAULT requiring an explicit manager ruling (same doctrine as a bespoke skeleton) —
+	 *  no current unit triggers it.
+	 */
+	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Unit")
+	float SkeletalVisualYawOffset = -90.f;
+
 	/** §6 white hit-flash on every actual damage event (M7, TASK-154). ... */
```

### `SummonedUnit.cpp` — `ASummonedUnit::ResolveSkeletalVisual`, immediately after the grounding block (old `:325`), before the AnimClass resolution

```diff
 		GroundedLoc.Z = -HalfHeight - MeshMinZ; // mesh's lowest point → capsule bottom (= floor)
 		SkeletalVisualMesh->SetRelativeLocation(GroundedLoc);
 	}

+	// FACING-FIX (TASK-326/327, SYSTEMIC): the OTHER HALF of the same authoring trap the grounding
+	// block above closes. Z was derived in C++ so no BP has to hand-author it; YAW was left to the
+	// BP — and the three units whose SkeletalVisualMesh was never authored at all (Archer, Ogre,
+	// Wizard) sat at the constructor default (0,0,0) on BOTH. Yaw 0 leaves the mesh's baked forward
+	// on the actor's RIGHT, i.e. the unit walks SIDEWAYS — and because facing comes from
+	// bOrientRotationToMovement and attack aim is the same actor rotation, the 90° error is rigid
+	// across marching, attacking and death alike.
+	//
+	// The fleet bakes ONE forward: all 12 units are rigged through Tools/ArtPipeline/rig_character.py
+	// (front on Blender -Y) onto the ONE shared SK_Footman_Skeleton, which arrives as UE-local +Y
+	// (measured in-engine, TASK-326: 12/12 on the shared skeleton; raw SkeletalMeshActors at actor
+	// yaw 0 all face a +Y camera). Actor forward is +X and Rot(θ)·(0,1,0) = (-sinθ, cosθ) = (1,0,0)
+	// ⇒ θ = -90 — the same constant, for the same reason, as ASiegePlayerController::GhostYawOffset,
+	// which is why the placement ghost has never mis-faced even for units whose BP yaw is 0.
+	//
+	// ABSOLUTE assignment, NEVER additive: `+=` would take the 9 correctly-authored units from -90
+	// to -180 and regress the whole fleet. Overwriting is the ONLY formulation that is a no-op for
+	// the 9 (they measure exactly (0,-90,0), so this writes back a component-wise identical rotator
+	// — no transform delta, no render/bounds/attachment change) AND the fix for the 3. Only .Yaw is
+	// touched; authored pitch/roll are read and written back untouched, exactly as the grounding
+	// block preserves authored X/Y. Un-rigged units returned at the guards above (:266 / :279) and
+	// never reach here; SkeletalVisualMesh is non-null by construction past the :266 guard. The
+	// static VisualMesh, the lunge base (VisualMeshBaseRelativeLocation), the juice/flash paths and
+	// the placement ghost are all deliberately untouched.
+	FRotator FacingRot = SkeletalVisualMesh->GetRelativeRotation(); // keep authored pitch/roll
+	FacingRot.Yaw = SkeletalVisualYawOffset;                        // fleet forward: mesh +Y → actor +X
+	SkeletalVisualMesh->SetRelativeRotation(FacingRot);
+
 	// AnimClass resolution (TASK-159 + shared-ABP fallback, TASK-165 rig-import chain):
```

**Post-edit line numbers for QA:** header property at `SummonedUnit.h:345-367` (`float SkeletalVisualYawOffset = -90.f;` on `:367`); body block at `SummonedUnit.cpp:327-353` (the three executable statements on `:351`, `:352`, `:353`). The grounding block is unchanged at `:317-325`; the AnimClass block now starts at `:355`.

---

## 2. THE ASSIGNMENT IS ABSOLUTE, NOT ADDITIVE — confirmed

The only mutation of the yaw is:

```cpp
FacingRot.Yaw = SkeletalVisualYawOffset;
```

A plain `operator=` on the `.Yaw` member of a local rotator. There is **no `+=`, no `-=`, no `+`, no
`FRotator` composition, no `AddLocalRotation`, no `AddRelativeRotation`, no quaternion multiply** anywhere
in the added code — grep `SkeletalVisualYawOffset` returns exactly two hits in the module: the header
declaration and this one assignment. The result is idempotent: running `ResolveSkeletalVisual` twice (it
cannot — `bUsingSkeletalVisual` latches at `:391` and the `:266` guard early-returns — but even so) yields
the identical yaw. Additive would have produced −180 on the 9 good units; this cannot.

---

## 3. THE NO-OP ARGUMENT — restated for line-by-line checking

**Claim: for the 9 correctly-authored units this fix produces a component-wise identical rotator and therefore
zero observable change; for the 3 broken units it is the entire fix; for un-rigged units the code never runs.**

**(a) The 9 correct units — input rotator == output rotator, component-wise.**
TASK-326 measured every `BP_Unit_<U>.Default__BP_Unit_<U>_C:SkeletalVisualMesh` CDO template directly
(§1 table). Footman, Knight, Miner, Cleric, Sapper, Pikeman, Cavalry, MilitiaMob, Longbowman all read
**pitch 0, yaw exactly −90, roll 0** — the literal value `-90`, not `270` and not `-90.0001`, so there is no
normalization or float-equality ambiguity. Walk the three statements against that input:

| stmt | effect on a `(0, −90, 0)` unit |
|---|---|
| `FRotator FacingRot = ...GetRelativeRotation();` | `FacingRot == (0, −90, 0)` |
| `FacingRot.Yaw = SkeletalVisualYawOffset;` | writes `−90.f` over `−90` ⇒ still `(0, −90, 0)` |
| `...SetRelativeRotation(FacingRot);` | sets the component to the value it already holds |

Pitch and roll are never assigned — they survive by being carried in the copied rotator, structurally the
same preservation the grounding block gives authored X/Y (`GroundedLoc` copy, `.Z` only). Setting a relative
rotation to the value already held produces no transform delta, hence no render change, no shift of anything
attached to the component, no bounds/physics change. **QA can verify this against §1 of the TASK-326 handoff
row by row — that table is the input to this argument.**

**(b) Un-rigged / static-fallback units never reach the block.** Two early returns sit above it, both
unchanged: `:266` (`bUsingSkeletalVisual || !SkeletalVisualMesh || CardID.IsNone()` → return) and `:279-282`
(`!SkeletalAsset` → return). A unit with no `/Game/Characters/SK_<CardID>` returns before the grounding block
and therefore before the new block — byte-for-byte today's behavior. Corollary for null-safety: past `:266`,
`SkeletalVisualMesh` is non-null by construction, so the three dereferences are safe; they are also the
4th–6th dereferences of that same pointer in the same scope (`:287`, `:321`, `:322`, `:324` precede them).

**(c) The 3 broken units are the only behavioral change.** Archer, Ogre and Wizard move from yaw 0 (the C++
constructor default — `SummonedUnit.cpp:130-135` creates the component and sets collision/visibility flags
only, never a relative rotation, which is exactly why an un-authored component reads `(0,0,0)`) to −90.
That is the fix.

**(d) Nothing downstream is coupled to this rotation.**
- The **lunge** caches `VisualMeshBaseRelativeLocation` from the **static** `VisualMesh` at BeginPlay
  (`:186-190`) — a *location*, on a *different component*. Untouched.
- `USiegeMeshJuiceComponent` / `USiegeHitFlashComponent` contain zero `FRotator`/`Rotation` references
  (juice is scale + location; flash is an overlay material). `MeshJuiceComponent->SetTargetMesh(...)` runs
  after `ResolveSkeletalVisual()` in `LoadStatsAndStart`, so ordering is unchanged.
- The **placement ghost** is a separate `AStaticMeshActor` owned by `ASiegePlayerController` with its own
  `GhostYawOffset` (`SiegePlayerController.h:702`, applied at `.cpp:1470`). Not read, not written.
- The **static `VisualMesh`** path, the melee/siege/suicide paths, `FaceTarget`, the death-anim hold, and
  the animation resolution are all untouched.
- `AMinerUnit` inherits the base path unchanged (Miner is a −90 unit ⇒ no-op for it too).

**(e) Reflection / build hygiene.** `SkeletalVisualYawOffset` is a new name — repo-wide grep shows no
existing member of that name on `ASummonedUnit`, `AMinerUnit`, `ACharacter`, `APawn` or `AActor`, so no
inherited reflected member is shadowed (the UHT law). No new type is dereferenced or `Cast<>`ed, so no new
include is required: `FRotator` comes with `CoreMinimal.h`, and `USkeletalMeshComponent` already has its
complete-type include at `SummonedUnit.cpp:10`. `SetRelativeRotation`/`GetRelativeRotation` are the current
non-deprecated `USceneComponent` API in UE5 (same family as the `SetRelativeLocation` the grounding block
already uses one line above).

---

## 4. Deliberately NOT done (per the TASK-326 spec)

- **The three stale BP values are left at 0.** `BP_Unit_{Archer,Ogre,Wizard}` are not opened. C++ now
  overwrites the yaw at swap time; re-authoring per-BP is the exact trap this closes, and editing them
  would make the fix untestable (it would become a no-op for all 12 and prove nothing).
- **Grounding math untouched** (`:317-325`) — TASK-307's fix is not re-opened.
- **No compile, no editor, no Git** — build-master's lane, after QA passes.
- **`CONVENTIONS.md` not edited** — the manager is ratifying the drafted "Unit mesh facing" law
  (TASK-326 handoff §10) in parallel. This code matches that draft: absolute assignment,
  `EditDefaultsOnly` default −90, documented exception hatch.

## 5. Out-of-scope finding carried forward (NOT fixed here)

`BP_Unit_Wizard`'s **static** `VisualMesh` is yaw 0 **and** Z 0, where all 11 other units carry −90 /
−HalfHeight. It is **latent only**: the static component is hidden the moment `SK_Wizard` resolves (it
does), and the placement ghost uses the controller's own correctly-offset actor. It would surface only if
`SK_Wizard` ever failed to load, in which case the Wizard would be both mis-faced and floating ~88 cm (the
runtime Z derivation is on the SK path only). It is **authored BP data, not C++** — out of scope for
TASK-327, unchanged, and the manager is ruling on it separately.

---

## 6. What QA should scrutinise

1. **The assignment is absolute, never additive** — `SummonedUnit.cpp:352`. An additive form silently −180s
   nine units. This is the single highest-value check.
2. **The no-op claim for all 9** measured `(0, −90, 0)` units — §2(a) above against the TASK-326 §1 table.
3. **Pitch/roll preservation is structural** (read the rotator, write only `.Yaw`), not incidental.
4. **The constant is `-90.f` on an `EditDefaultsOnly` property**, not a magic literal at the call site, and
   the value matches what TASK-326 measured.
5. **Placement:** the block sits *after* the grounding `if` closes (`:325`) and *before* the AnimClass
   resolution (`:355`) — inside the same single swap site, so it runs exactly once per rigged unit.
6. **The early-return guards** (`:266`, `:279-282`) genuinely protect the un-rigged path and make the
   dereference null-safe.
7. **Blast radius:** only `SummonedUnit.{h,cpp}` changed; grounding, lunge, juice/flash, static-mesh,
   ghost/`GhostYawOffset`, and all Blueprint/asset data are untouched.

## 7. Files / references

- Edited: `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`, `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp`
- Read-only reference: `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h:700-702` (`GhostYawOffset` precedent)
- Upstream evidence: `.claude/pipeline/handoffs/TASK-326-programmer.md` (§1 yaw table, §2 baked-forward proof, §5 fix spec)
- Assets referenced by the code (unchanged): `/Game/Characters/SK_<CardID>`, `/Game/Characters/SK_Footman_Skeleton`, `/Game/Blueprints/Units/BP_Unit_<Unit>`
