# TASK-307 handoff — Systemic feet-grounding for SkeletalVisualMesh (gameplay-programmer)

**Status:** ready-for-qa · FILE-ONLY (no compile, no editor, no Git, no MCP)
**Files touched:** `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` — ONLY. `SummonedUnit.h` UNCHANGED.
**Date:** 2026-07-26 · **Branch:** m7.6-arena10x
**Diagnosis:** TASK-306 / `handoffs/DIAG-floating-units.md` SESSION-3.

## The one change — `ASummonedUnit::ResolveSkeletalVisual()`

Single site, immediately after `SkeletalVisualMesh->SetSkeletalMeshAsset(SkeletalAsset)` (~SummonedUnit.cpp:287). Replaced the v1 fixed-offset copy with a capsule-derived offset. Nothing else in the function or the TU changed. No signature change, no new member, no new include.

### BEFORE (v1, TASK-259 — the code that RECURRED on the Wizard)
```cpp
// FLOAT-FIX (DIAG-floating-units SESSION-2): ground the skeletal mesh to the SAME
// authored offset the static VisualMesh already carries. ...
if (bVisualMeshBaseCached)
{
    SkeletalVisualMesh->SetRelativeLocation(VisualMeshBaseRelativeLocation);
}
```
`VisualMeshBaseRelativeLocation` is cached in `BeginPlay` (`:188`) from `VisualMesh->GetRelativeLocation()` — the **static VisualMesh's BP-authored Z**. So v1 only relocated the "each BP must hand-author `Z = −CapsuleHalfHeight`" dependency from the *SkeletalVisualMesh* component onto the *static VisualMesh* component. `BP_Unit_Wizard`'s static VisualMesh.Z was never offset (the TASK-304 authoring spec omits it; TASK-302 §5/§6 explicitly assumed the fix was automatic), so the copy propagated `0` → float ≈ capsule half-height.

### AFTER (v2, systemic — derive from the capsule + the mesh's own bounds)
```cpp
if (UCapsuleComponent* Capsule = GetCapsuleComponent())
{
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    const FBoxSphereBounds SkBounds = SkeletalAsset->GetBounds(); // ref-pose local bounds
    const float MeshMinZ = (SkBounds.Origin.Z - SkBounds.BoxExtent.Z) * SkeletalVisualMesh->GetRelativeScale3D().Z;
    FVector GroundedLoc = SkeletalVisualMesh->GetRelativeLocation(); // keep authored X/Y
    GroundedLoc.Z = -HalfHeight - MeshMinZ; // mesh's lowest point → capsule bottom (= floor)
    SkeletalVisualMesh->SetRelativeLocation(GroundedLoc);
}
```
(Plus an expanded comment block documenting the mechanism, the v1→v2 rationale, and the no-op argument — see the file, ~:289-311.)

### Identifier wiring (as requested — wired to the real in-scope locals)
- **`SkeletalAsset`** — the resolved `USkeletalMesh*` from `SkSoft.LoadSynchronous()` earlier in the function (`:278`), guaranteed non-null (the `if (!SkeletalAsset) return;` guard at `:279-282`). This is the correct var at this site (NOT `SkeletalVisualMesh->GetSkeletalMeshAsset()`, though that would be equivalent post-swap).
- **`GetCapsuleComponent()`** — `ACharacter` root, never null; the `if` is defense-in-depth (and scopes `Capsule`).
- **`SkeletalVisualMesh`** — already non-null (early return at the top of the function, `:266-269`).
- **`GetRelativeLocation()`** read back to preserve the BP-authored **X/Y** (only Z is a grounding concern); the lunge writes X anyway, so X/Y here is cosmetic-neutral but kept faithful.

## No-regression argument (the load-bearing claim)

The formula grounds the mesh's LOWEST point at the capsule bottom. CharacterMovement floors the capsule (bottom at actor-Z `−HalfHeight`), so mesh-feet land on the floor:
```
mesh feet (actor space) = GroundedLoc.Z + MeshMinZ = (−HalfHeight − MeshMinZ) + MeshMinZ = −HalfHeight = capsule bottom ✓
```

**Every current rigged fleet unit is feet-origin** ⇒ `MeshMinZ ≈ 0` ⇒ `GroundedLoc.Z = −HalfHeight`, which equals the value those BPs already author. Hard numbers from the DIAG SESSION-2 live-MCP table (`handoffs/DIAG-floating-units.md`):

| Unit | Capsule HalfHeight | SK bounds origin.z / extent.z | MeshMinZ = origin.z − extent.z | v2 GroundedLoc.Z | Existing authored VisualMesh.Z | Match |
|---|---|---|---|---|---|---|
| Footman | 90 | 89.85 / 89.83 | +0.02 | −90.02 | −90 | ✓ (Δ 0.02 uu) |
| Archer | 90 | 90.00 / 89.95 | +0.05 | −90.05 | −90 (after v1) | ✓ (Δ 0.05 uu) |
| Ogre | 145 | 143.93 / 144.09 | −0.16 | −144.84 | −145 (after v1) | ✓ (Δ 0.16 uu) |
| Knight / Longbowman / Cavalry / Pikeman / MilitiaMob / Sapper / Cleric | ~90–95 | ~feet-origin | ≈ 0 | ≈ −HalfHeight | −90 (authored) | ✓ (sub-uu) |

All deltas are **< 0.2 uu (< 2 mm)** — visually byte-equivalent, no perceptible shift. So it is a genuine NO-OP for correctly-grounded units.

**The Wizard + the 11 Meshy rebuilds:** all feet-origin (`SM_Wizard` min_z 0.065, TASK-301; the rebuild batch is the same Meshy feet-origin pipeline) ⇒ `MeshMinZ ≈ 0` ⇒ grounded at `−HalfHeight` **automatically, with ZERO per-BP capsule/Z authoring** — even if a BP never resizes the capsule (feet still land on the floored capsule bottom; capsule size then only affects collision, not the visual float). The recurrence is closed permanently.

**Non-feet-origin future meshes** are handled for free (the formula uses the actual bounds, not an assumption of pivot-at-feet). **Per-BP mesh scale** is handled via `RelativeScale3D.Z`.

## Scope / blast radius (per the spec guardrails)
- **Untouched:** the static-VisualMesh path, `VisualMeshBaseRelativeLocation` (still cached in `BeginPlay` and still the lunge rest-pose base at `:2172/:2185/:2214` — the lunge is byte-for-byte unchanged), the melee / Siege / suicide paths, and the placement ghost (a separate `AStaticMeshActor` placed at the ground point, `SiegePlayerController.cpp:1443` — never used this offset). Minimal blast radius: one component's Z at one call site.
- The static VisualMesh is hidden the moment this SK swap takes, so its own BP-authored Z is now visually irrelevant for rigged units (the ghost/blockout-fallback still use it, unchanged).

## Coding-law checks
- **Includes (complete types) — already present, no new include:** `Components/CapsuleComponent.h` (`:8`, for `GetScaledCapsuleHalfHeight`) and `Engine/SkeletalMesh.h` (`:15`, for `USkeletalMesh::GetBounds`). `GameFramework/Character.h` (via the header) provides `GetCapsuleComponent()`.
- **No shadowing:** `Capsule`, `HalfHeight`, `SkBounds`, `MeshMinZ`, `GroundedLoc` are new function-local names; none shadow an inherited reflected member or an existing local in this function.
- **Null-safety:** `SkeletalAsset` non-null (guarded `:279`), `SkeletalVisualMesh` non-null (guarded `:266`), `GetCapsuleComponent()` guarded by the `if`.
- **`GetBounds()` semantics:** `USkeletalMesh::GetBounds()` returns the imported ref-pose `FBoxSphereBounds` in local space — exactly what the DIAG table read via MCP; `.Origin` / `.BoxExtent` are the local-space center/half-extent.

## What QA should scrutinize (TASK-307-QA)
1. Confirm the offset is now capsule-relative/systemic (derived), not a fixed member/constant — and grounds any feet-origin mesh.
2. Confirm the NO-OP: `MeshMinZ ≈ 0` for the current fleet ⇒ `Z = −HalfHeight` = existing authored offsets (table above; deltas < 0.2 uu). Flag any unit where feet-origin would NOT hold — I found none in the roster (all are feet-origin per the SK-import convention + DIAG bounds).
3. Confirm the static-fallback path, the lunge (`VisualMeshBaseRelativeLocation`), and the ghost are untouched and null-safe.
4. Coding laws: includes present (no new one needed), no shadow.

## Downstream
- TASK-307-QA (qa-reviewer) → TASK-308 (build-master: compile + PIE spot-check grounding on `L_Arena` + commit on main, no push). This UNBLOCKS every per-unit `-verify` in FLEET-REMASTER (the grounding gate).
- **Secondary Ogre-near-hill capsule-lift watch (TASK-259 WATCH):** OUT OF SCOPE for this fix and left as-is. It is a spawn-time capsule LIFT (nav-projection / `AdjustIfPossibleButAlwaysSpawn` Z-extents near elevation, DIAG SESSION-1 Rank-2), additive on top of the base mesh offset — a separate hardening item (DIAG "Fix option D/E"), not a component-offset concern. This fix removes the BASE float entirely; the hill-amplification path is unchanged.
