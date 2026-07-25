# QA Report — TASK-292c
Verdict: PASS

Branch: `m7.6-arena10x` · Reviewed 2026-07-24 · File under review: `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` (code-only; NO compile/PIE/Git in this lane).
Consumes: handoffs/TASK-292c.md, diagnosis handoffs/TASK-292.md, board spec TASK-292c.

## Verification against the 5 checkpoints

1. **Both flags at BOTH sites — CONFIRMED.**
   - Site 1 VISUAL HISM `ResolveComponentForMesh()`: `Comp->bAffectDistanceFieldLighting = false;` (cpp:650) + `Comp->bAffectDynamicIndirectLighting = false;` (cpp:651).
   - Site 2 PROXY HISM `ResolveProxyForVisual()`: `Proxy->bAffectDistanceFieldLighting = false;` (cpp:740) + `Proxy->bAffectDynamicIndirectLighting = false;` (cpp:741).
   The invisible collision-proxy correctly gets the same pair — it is still a `UPrimitiveComponent` that would otherwise be inserted into the DF/Lumen scene and generate a mesh distance field.

2. **Pre-Register ordering — CONFIRMED at both sites.**
   - Site 1: flags (650-651) sit AFTER the TASK-284 `SetCullDistances` (636) / `SetCastShadow` (637) and BEFORE `Comp->RegisterComponent()` (653).
   - Site 2: flags (740-741) sit AFTER the collision/nav block (`SetCollisionResponseToChannel(ECC_Pawn…)` 729 / `bFillCollisionUnderneathForNavmesh` 730 / `SetCanEverAffectNavigation(true)` 731) and BEFORE `Proxy->RegisterComponent()` (743).
   Both match the TASK-284 pre-Register cull wiring exactly — render state does not exist until registration, so the flags are read at first DF/Lumen-scene insertion. No `MarkRenderStateDirty` needed; the clean path.

3. **API correctness (UE 5.8) — CONFIRMED (with one record-correction, non-blocking).**
   - `bAffectDynamicIndirectLighting:1` = `PrimitiveComponent.h:556`; `bAffectDistanceFieldLighting:1` = `PrimitiveComponent.h:564`. Both are `uint8:1` UPROPERTY bitfields on `UPrimitiveComponent`.
   - **Both are PUBLIC:** the governing access specifier is `public:` at `PrimitiveComponent.h:431`; the next specifier (`protected:`) is at `:732`, so lines 556/564 fall inside the public block. Direct assignment of `false` to a public `uint8:1` bitfield from external code (via a `UHierarchicalInstancedStaticMeshComponent*`) compiles cleanly.
   - **Complete-type / include:** `Components/HierarchicalInstancedStaticMeshComponent.h` is included (cpp:6) and pulls in the `UPrimitiveComponent` base declaration — the member access has a complete type. No new include needed (complete-type include law satisfied).
   - RECORD-CORRECTION (NIT, does not affect verdict): the handoff/board claim that `bAffectDistanceFieldLighting` "has NO setter" is inaccurate for UE 5.8 — `SetAffectDistanceFieldLighting(bool)` exists at `PrimitiveComponent.h:2005` (and `SetAffectDynamicIndirectLighting` at :2017). This changes nothing about correctness: direct assignment is valid because both members are public, and pre-Register there is no render state to dirty, so direct-assignment ≡ setter here. The symmetric direct-assignment choice is fine.

4. **Blanket, nothing-else-changed — CONFIRMED.**
   - The diff is exactly the two additive flag-pair blocks (+ their explanatory comments) at cpp:639-651 and cpp:733-741.
   - TASK-284's work intact: `SetCullDistances` (636, 725) and `SetCastShadow` (637) untouched; the proxy `SetCastShadow(false)` invariant (717) untouched.
   - No new `FScatterLayer` field: `BattlefieldScatter.h` not touched (no DF/GI member added).
   - Density / `InstanceCount`, grid-hash placement (`FScatterSpacingGrid`), seed order (`FRandomStream`), mines, collision/nav profile, material override — all in code paths untouched by these two edits. No regression to TASK-284.

5. **Braces / shadowing / include — CLEAN.** Both functions are well-formed and close correctly (`ResolveComponentForMesh` at 656, `ResolveProxyForVisual` at 747). No new locals introduced → no inherited-reflected-member shadowing. No new include required.

## Non-blocking judgment (blanket-off correctness)
AGREE — blanket DF/GI-off is the correct treatment for decorative scatter at ~15,000 instances on the 10× field. It removes BOTH the residual singular-matrix `InverseFast` NaN source AND a needless DF/Lumen cost. No layer genuinely wants DF: the hills keep their real dynamic/CSM shadows via `CastShadow=true` (separate subsystem from DF); losing DFAO / DF-shadow contribution from stylized decorative scatter is imperceptible and is standard practice for backdrop/scatter geometry. Engine note reinforces this — `bAffectDistanceFieldLighting` is "only used if CastShadow is true" (`PrimitiveComponent.h:562`), so clearing it is *active* exactly on the shadow-casting hills (the likely residual contributor) and a harmless no-op elsewhere. This correctly mirrors the TASK-292/292b vista treatment onto the scatter, in code.

## Findings
- [NIT] handoffs/TASK-292c.md / TASKBOARD.md:2168,2185 — rationale states `bAffectDistanceFieldLighting` has "NO setter"; UE 5.8 does expose `SetAffectDistanceFieldLighting(bool)` (`PrimitiveComponent.h:2005`). No code impact — direct assignment is valid (member is public) and equivalent pre-Register. Record-correction only.

## Notes for build-master (if PASS)
- Compile with the standard Build.bat, then PIE-verify on L_Arena that BOTH `EnsureFailed: OriginX <= OriginMax` (`DoubleFloat.cpp:19`) AND `InverseFast … non-invertible → NaN` (`Matrix.h:468`) + the per-frame `LogUnrealMath` NaN spam are GONE.
- **The residual was SEED-DEPENDENT** (TASK-292b: gen-1 seed 1857084801 was already clean, gen-2 seed 1529263745 reproduced it). A single clean seed is NOT sufficient — run SEVERAL generates (Play Again re-rolls via `bReRandomizeOnMatchReset=true`) and confirm clean across MULTIPLE seeds.
- Editor RESTART recommended before verifying — these fire during render init and ensures self-suppress within a session (per TASK-292b).
- Confirm traversability (Blue→Red + the 6 mine paths, 0 culls) unregressed. Removing meshes from DF/Lumen only reduces cost; no perf regression expected.
- Then commit `BattlefieldScatter.cpp` + the handoff + board. No push.
