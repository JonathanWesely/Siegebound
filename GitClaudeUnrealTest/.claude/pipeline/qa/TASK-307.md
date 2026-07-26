# QA Report — TASK-307
Verdict: PASS

Pre-compile review of the systemic feet-grounding fix in
`Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp`,
`ASummonedUnit::ResolveSkeletalVisual()` (edited region :289-325). `.h` untouched. Read-only review, no compile.

## Verdict basis (the load-bearing no-regression claim)
The formula grounds the mesh's LOWEST local point at the capsule bottom:
`GroundedLoc.Z = -HalfHeight - MeshMinZ` ⇒ mesh-min in capsule space
`= GroundedLoc.Z + MeshMinZ = -HalfHeight` = the actor-relative Z where CharacterMovement floors the capsule bottom. **Sign is correct** (a flipped sign would put the mesh at `-HalfHeight - 2·MeshMinZ`, or push it up by `+HalfHeight`; neither is present).

Cross-checked against the DIAG SESSION-2 live-MCP bounds table — all three re-derive byte-equivalently to the existing authored offsets:

| Unit | HalfHeight | SkBounds origin.z / extent.z | MeshMinZ = o−e | v2 GroundedLoc.Z | Authored VisualMesh.Z | Δ |
|---|---|---|---|---|---|---|
| Footman | 90 | 89.85 / 89.83 | +0.02 | −90.02 | −90 | 0.02 uu |
| Archer | 90 | 90.00 / 89.95 | +0.05 | −90.05 | −90 | 0.05 uu |
| Ogre | 145 | 143.93 / 144.09 | −0.16 | −144.84 | −145 | 0.16 uu |
| Knight/Longbowman/Cavalry/Pikeman/MilitiaMob/Sapper/Cleric | ~90–95 | feet-origin (≈0) | ≈0 | ≈ −HalfHeight | −90 | sub-uu |

All Δ < 0.2 uu (< 2 mm), sub-perceptual ⇒ genuine no-op for every already-grounded unit. Wizard + the 11 feet-origin Meshy rebuilds (`MeshMinZ ≈ 0`) ground at `−HalfHeight` automatically with zero per-BP authoring — recurrence closed.

## Findings
- [WARN] SummonedUnit.cpp:319-323 — The derivation assumes root/actor world scale = 1. `GetScaledCapsuleHalfHeight()` folds in the capsule component's WORLD scale, whereas `GroundedLoc.Z` is a relative Z in the (scale-1) capsule-local frame; under a non-unit ROOT actor scale these two would mismatch (double-scale). Not a blocker: every rostered unit is a standard `ACharacter` with root scale 1 (per-mesh scale lives on the component and IS correctly handled via `RelativeScale3D.Z`), and the pre-existing static −90 offset made the identical assumption. Flag only for a hypothetical future actor-scaled unit.
- [NIT] SummonedUnit.cpp:317-325 — Offset is derived ONCE at `ResolveSkeletalVisual` (post-BeginPlay). A BP that resized its capsule at RUNTIME after the swap would not re-derive. No unit does this (capsule size is fixed at construction), so it is inert today.

## Checks cleared
- **Systemic / derived, not a fixed constant** — offset comes from capsule half-height + the mesh's OWN `GetBounds()`, grounds any feet-origin mesh. Confirmed.
- **Null-safety** — `SkeletalAsset` non-null (guard + early return :279-282); `SkeletalVisualMesh` non-null (early return :266-269); `Capsule` scoped/guarded by `if (UCapsuleComponent* Capsule = GetCapsuleComponent())` :317. No deref risk.
- **Includes** — `Components/CapsuleComponent.h` (:8) and `Engine/SkeletalMesh.h` (:15) already in the TU; `GetScaledCapsuleHalfHeight` / `GetBounds` / `GetRelativeScale3D` / `SetRelativeLocation` all reachable with complete types. No new include needed; `.h` correctly untouched.
- **Bounds semantics — no double-count** — uses `SkeletalAsset->GetBounds()` (the ASSET's ref-pose LOCAL bounds), NOT the component/world `Bounds` (which would already fold in `RelativeLocation` and double-count). Correct choice; matches the DIAG MCP `get_bounds` reads.
- **No shadowing** — `Capsule`, `HalfHeight`, `SkBounds`, `MeshMinZ`, `GroundedLoc` are new function-locals; none appear as members in `SummonedUnit.h` (grep clean) and none shadow an inherited reflected member.
- **No deprecated UE 5.8 API** — `SetSkeletalMeshAsset` is the current setter; `GetScaledCapsuleHalfHeight`, `USkeletalMesh::GetBounds`, `GetRelativeScale3D`, `SetRelativeLocation` are all current in 5.8.
- **Pre-compile catch class** — edited region has NO Printf (the two `FString::Printf` at :276/:336 are pre-existing, literal format + matching `%s` args, untouched); the :289-316 doc block is all `//` line comments, no stray/unterminated `/* */`. Clean.
- **Blast radius contained** — static `VisualMesh` path unchanged; the lunge (`StartAttackLunge`/`StopAttackLunge`/`UpdateLunge` :2190/:2203/:2232) writes only to the static `VisualMesh` via `VisualMeshBaseRelativeLocation`, NOT `SkeletalVisualMesh`, so the new grounding is never overridden by a lunge; blockout-fallback path is the early return at :279-282 (fix block never runs when no SK); placement ghost (`SiegePlayerController.cpp`) untouched. Confirmed.

## Notes for build-master (PASS)
- Compile TASK-307 (`SummonedUnit.cpp` only; hard gate — a build failure routes back to gameplay-programmer via this report, counts as a QA loop).
- PIE spot-check on `L_Arena`: (a) a currently-grounded unit (Footman) must STAY grounded — expect ≤ ~0.02 uu shift, visually identical; (b) a previously-floating unit (Wizard / Archer / Ogre) must now sit feet-on-floor. Un-rigged units take the early-return static path — grounding there is unchanged.
- Context (NOT a regression, pre-existing, unchanged by this task): for rigged units the attack LUNGE animates the hidden static `VisualMesh`, not the visible `SkeletalVisualMesh`, so a lunge translation may not be visible on rigged units. Do not attribute a "no visible lunge" observation to TASK-307 — the fix touches only the `SkeletalVisualMesh` Z once at swap time.
