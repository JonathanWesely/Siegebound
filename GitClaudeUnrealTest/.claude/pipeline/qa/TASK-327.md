# QA Report — TASK-327 (systemic unit mesh-facing fix)

**Verdict: PASS** · **BLOCKERS: 0** · WARN: 2 · NIT: 4
Reviewed: 2026-07-27 · Reviewer: qa-reviewer · Lane: read-only (no edits, no engine, no MCP, no Git)

Files reviewed (post-edit, read directly — the handoff was NOT taken on trust):
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h:345-367`
- `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp:327-353` (inside `ResolveSkeletalVisual`)

Cross-checked against `.claude/pipeline/handoffs/TASK-326-programmer.md` §1 (the 12-unit measured yaw
table), §2 (baked-forward derivation) and §5 (fix spec), plus the surrounding call graph.

**The handoff's post-edit line numbers are accurate** (property at `.h:345-367`, float on `:367`;
block at `.cpp:327-353`, statements on `:351/:352/:353`; grounding unchanged at `:317-325`; AnimClass
now starts at `:355`). Build-master can navigate by them.

---

## Per-item verification (the 7 requested checks)

### 1. ABSOLUTE, not additive — **VERIFIED, read from source**

`SummonedUnit.cpp:351-353` reads verbatim:

```cpp
FRotator FacingRot = SkeletalVisualMesh->GetRelativeRotation(); // keep authored pitch/roll
FacingRot.Yaw = SkeletalVisualYawOffset;                        // fleet forward: mesh +Y → actor +X
SkeletalVisualMesh->SetRelativeRotation(FacingRot);
```

`:352` is a plain `operator=` on the `.Yaw` member of a **local** rotator. There is no `+=`, `-=`,
binary `+`, `FRotator` composition, `AddLocalRotation`, `AddRelativeRotation`, `SetWorldRotation`, or
quaternion multiply anywhere in the added code. Independently confirmed by grep across
`Source/`: `SkeletalVisualYawOffset` has exactly **two** hits in the module (`.h:367` declaration,
`.cpp:352` assignment — no other reader/writer), and `SetRelativeRotation|SetWorldRotation|
AddLocalRotation|AddRelativeRotation` across the whole module returns exactly **two** hits:
`CaptureZone.cpp:41` (a decal, pre-existing) and `SummonedUnit.cpp:353` (this one). The −180
fleet-regression failure mode is structurally impossible here.

**Idempotency — verified two ways.** (a) The statement is a pure absolute write, so N applications ==
1 application regardless of entry state. (b) Re-entry cannot even occur: `bUsingSkeletalVisual` is set
true at `:391` and is **never reset anywhere in the module** (grep: written only at `:391`), and the
`:266` guard early-returns on it. Independent of the latch, the yaw converges to −90 on any path.

### 2. Genuine no-op for the 9 already-correct units — **VERIFIED against the TASK-326 §1 table**

Walked row by row. Footman, Knight, Miner, Cleric, Sapper, Pikeman, Cavalry, MilitiaMob, Longbowman all
measure `SkeletalVisualMesh` = pitch 0 / **yaw exactly −90** / roll 0 (§1 states pitch and roll are 0 on
all 24 measured components; the yaw column is the literal `-90`).

- **No normalization ambiguity.** The measured value is `-90`, not `270`. `USceneComponent` stores
  `RelativeRotation` as an `FRotator` field and `GetRelativeRotation()` returns it directly — there is no
  quaternion round-trip on the READ, so `FacingRot` starts at literally `(0, -90, 0)`.
- **No float/precision ambiguity.** `SkeletalVisualYawOffset` is a `float` `-90.f` widened to the
  LWC `double` `FRotator::Yaw`. −90 is exactly representable in both, and float→double is exact
  widening (no narrowing, no C4244). The write is bit-identical to the value already held.
- **Write-back is a true no-op.** `SetRelativeRotation(FRotator)` forwards to
  `SetRelativeLocationAndRotation(GetRelativeLocation(), NewRotation, ...)` — the *current* relative
  location is re-used, so the grounded Z set 28 lines above at `:324` is preserved, and an unchanged
  transform produces no delta, no bounds/render change, and no attachment shift.
- **Pitch/roll preservation is structural, not incidental** — `:351` copies the whole rotator and only
  `.Yaw` is ever assigned. Exactly the same shape as the grounding block's `GroundedLoc` copy +
  `.Z`-only write at `:322-323`.
- **Nothing is attached to `SkeletalVisualMesh`.** Full-module grep of every `SkeletalVisualMesh`
  reference: it is never an attach parent (HP bar attaches to the capsule at `:123`), so the rotation
  moves no child component even for the 3 units it does change.

Claim (a) of the handoff holds as written.

### 3. Null-safety and reachability — **VERIFIED**

Two early returns sit above the block and are unchanged:
- `:266` — `if (bUsingSkeletalVisual || !SkeletalVisualMesh || CardID.IsNone()) return;`
- `:279-282` — `if (!SkeletalAsset) return;`

Past `:266`, `SkeletalVisualMesh` is non-null on every path (nothing between `:266` and `:353` nulls it),
so the three dereferences at `:351/:353` are safe — they are also the 5th–7th dereferences of the same
pointer in the same scope (`:287`, `:321`, `:322`, `:324` precede them). A unit with no
`/Game/Characters/SK_<CardID>` returns at `:281` **before** the grounding block and therefore before the
new block: the un-rigged / static-fallback path is byte-for-byte unchanged. Every other
`SkeletalVisualMesh` site in the class (`:254`, `:429`, `:452`, `:468`) remains independently guarded.

### 4. Placement in the function — **VERIFIED**

The block opens at `:327`, i.e. after the grounding `if` closes at `:325`, and the AnimClass resolution
begins at `:355`. Neither is disturbed:
- Grounding → facing: `SetRelativeRotation` re-uses the current relative location, so the derived
  `GroundedLoc.Z` survives intact.
- Facing → grounding (order-independence check): the grounding math uses ref-pose **local** bounds and
  the component's `RelativeScale3D.Z`; a yaw-only rotation changes neither, so the two blocks are
  genuinely uncoupled and the chosen order is not load-bearing (good).
- It runs exactly once per rigged unit, inside the single swap site called from `LoadStatsAndStart:1048`.

### 5. No collateral coupling — **VERIFIED (each path inspected, not assumed)**

- **Lunge** (`:2218`, `:2231`, `:2260`): writes `VisualMesh->SetRelativeLocation(VisualMeshBaseRelative
  Location [+ offset])` — a *location*, on the **static** component, cached at `:186-190`. Untouched.
- **Juice** (`SiegeMeshJuiceComponent.cpp`): `SetTargetMesh` captures **scale + relative location only**
  (`:24-25`) — no rotation is captured or restored. The squash writes `SetRelativeScale3D` in the
  component's own local axes; an XY-squash/Z-stretch is invariant under a yaw change, so even the 3
  fixed units squash identically. `PlayRecoil` converts the world direction into the **parent's**
  space (`:61-63`, the capsule), *not* the mesh's own space — so the mesh's relative yaw provably does
  not steer recoil. Ordering unchanged (`ResolveSkeletalVisual` `:1048` → `SetTargetMesh` `:1058`).
- **Hit flash**: overlay-material only, no transform.
- **Placement ghost**: a separate `AStaticMeshActor` on `ASiegePlayerController` using its own
  `GhostYawOffset` (`SiegePlayerController.h:702`, applied `.cpp:1470`). Not read, not written here.
- **Projectiles** (`FireProjectileAt:2144-2187`): muzzle is `GetActorLocation()` and aim is
  `ToTarget.Rotation()` — derived from the **actor**, never from a mesh socket. Module-wide grep shows
  **zero** `GetSocketLocation`/`GetSocketTransform` calls in `Siegebound/` (the only hits are in the
  untouched `Variant_Combat` template). Wizard/Archer/Longbowman firing behaviour is unchanged.
- **Melee / siege / suicide / `FaceTarget` / death-hold / anim resolution**: no rotation reads on this
  component anywhere (the full `SkeletalVisualMesh` grep confirms the only rotation call is `:353`).
- **`AMinerUnit`**: declares no member of this name, no `ResolveSkeletalVisual` override; inherits the
  base path. Miner measures −90 ⇒ no-op for it.

### 6. Coding laws — **VERIFIED**

- **No shadowing.** Repo-wide grep: `SkeletalVisualYawOffset` exists only on `ASummonedUnit`. `YawOffset`
  grep finds only the unrelated `ASiegePlayerController::GhostYawOffset`. No inherited reflected member
  is hidden (the UHT law).
- **No BP-side name collision.** Binary scan of `Content/**/*.uasset` for `SkeletalVisualYawOffset`
  returns 0 matches — no existing Blueprint variable will conflict with the new C++ property on
  BP recompile.
- **Includes.** No new type is dereferenced or `Cast<>`ed. `FRotator` arrives with `CoreMinimal.h`;
  `USkeletalMeshComponent` already has its complete-type include at `SummonedUnit.cpp:10`.
- **API currency (UE 5.8).** `USceneComponent::GetRelativeRotation()` / `SetRelativeRotation(FRotator)`
  are current and non-deprecated — the same family as the `SetRelativeLocation` the adjacent grounding
  block already uses. Nothing deprecated is introduced.
- **UPROPERTY specifiers.** `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Siegebound|Unit")`
  on a `protected` member is valid and correct for an exception hatch: no `meta=(AllowPrivateAccess)`
  is needed (protected, not private), and it byte-matches the precedent two properties down
  (`ProjectileClass`, `.h:396-397`) and `GhostYawOffset`. Default `-90.f` matches TASK-326's measurement;
  the constant is **not** a magic literal at the call site.
- **Performance.** One extra `SetRelativeRotation` per unit, once, at swap time. Nothing per-tick,
  no `FindObject`/`LoadObject` added.

### 7. Scope discipline — **VERIFIED**

`TASK-32[67]` markers appear in exactly three places in `Source/` — `.h:347` (doc block) and `.cpp:327`,
`:337` (the new comment) — i.e. only the two declared files carry this change. No Blueprint, asset,
`CONVENTIONS.md` or `TASKBOARD.md` edit is present in the reviewed change. `BP_Unit_Wizard`'s static
`VisualMesh` (yaw 0 / Z 0) is correctly left alone, and the three stale BP yaw values (Archer, Ogre,
Wizard) are correctly left at 0 — re-authoring them would both re-open the trap and make TASK-328's PIE
test vacuous.

---

## Findings

### BLOCKERS
*(none)*

### WARN
- **[WARN-1]** `SummonedUnit.cpp:279-282` (context) — **the fix only covers the SK path.** If
  `SK_<CardID>` ever fails to load, the function returns before both the grounding and the facing block
  and the unit falls back to the static `VisualMesh` with its per-BP authored transform. Per TASK-326
  §7.1, `BP_Unit_Wizard`'s static `VisualMesh` is yaw **0** and Z **0** where all 11 others are −90 /
  −HalfHeight, so the Wizard's fallback would be *both* mis-faced *and* floating ~88 cm. Correctly
  **out of scope** for TASK-327 (authored BP data, separate manager ruling) and **not a blocker** —
  `SK_Wizard` resolves today and the static component is hidden at `:389`. Carried forward so it is not
  lost: it wants its own ticket.
- **[WARN-2]** `SummonedUnit.cpp:352` — the `EditDefaultsOnly` hatch means a future BP override to `0`
  would silently re-create the exact bug this closes, with no runtime signal. The doc block requires a
  manager ruling, which is the right policy, but policy is what failed for Archer/Ogre/Wizard for
  months. **Do not re-open this task for it**; suggest the manager add the CONVENTIONS clause (already
  drafted, TASK-326 §10) and consider a one-time `UE_LOG` on `!FMath::IsNearlyEqual(SkeletalVisual
  YawOffset, -90.f)` in a *future* touch of this function.

### NIT
- **[NIT-1]** `SummonedUnit.cpp:317-325` (pre-existing) — the grounding math ignores component rotation
  (ref-pose local min-Z only). It is correct only while pitch/roll are 0, which they are on all 24
  measured components. This fix *preserves* authored pitch/roll, so a future BP that authors them would
  silently break grounding. Pre-existing, out of scope, noted for the record.
- **[NIT-2]** `SummonedUnit.h:367` — `float` assigned into a LWC `double` `FRotator::Yaw`. Exact and
  warning-free; `double` would be marginally more consistent, but `GhostYawOffset` is `float` and
  matching the shipped precedent is worth more than the type purity. No change requested.
- **[NIT-3]** `SummonedUnit.h:366` — no `meta = (UIMin/UIMax)` on the new float, where several
  neighbouring floats carry `ClampMin`. A yaw is legitimately unbounded; cosmetic only.
- **[NIT-4]** `SummonedUnit.cpp:327-350` — 24 comment lines for 3 statements is heavy, but it matches
  the density of the adjacent TASK-307 grounding block and this is a defect that already recurred once.
  Keep it. (Non-ASCII glyphs `θ · ⇒ ° →` in the new comment are the same class already present in the
  pre-existing block at `:291`/`:310`/`:323`, which compiles today — no new risk; UTF-8 continuation
  bytes are never `0x5C`, so no line-splice hazard.)

---

## Notes for build-master (TASK-328)

**Compile:** header change adds a `UPROPERTY` to a class with 12 Blueprint children ⇒ expect a full
module rebuild + BP recompile on editor load. No new include, no new module dependency.

**PIE verification — the 12-unit sweep, in priority order:**
1. **The 3 targets:** Archer, Ogre, Wizard march with their **front along the travel direction** (+X),
   not shoulder-first. This is the whole fix.
2. **The 9 controls:** Footman, Knight, Miner, Cleric, Sapper, Pikeman, Cavalry, MilitiaMob, Longbowman
   must look **exactly as before**. *If any of them moon-walks (180° reversed), stop — that is the
   additive-assignment signature and the fix was mis-applied.* Static review says it cannot happen; this
   is the empirical backstop.
3. **Grounding regression check:** no unit floats or sinks after the rotation write (the new block is
   adjacent to TASK-307's; `SetRelativeRotation` preserves relative location, but confirm visually).
4. **All states, not just marching:** attack and death poses for Archer/Ogre/Wizard should now aim at
   the target — the old 90° error was rigid across marching/attacking/dying.
5. **Sanity, expected unchanged:** Wizard fireball spawns at the actor and flies to target (actor-
   transform based, `:2165-2167`); placement ghost unchanged; spawn squash unchanged.

**Commit hygiene:** stage **only** `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.h`,
`SummonedUnit.cpp` (+ the pipeline docs the orchestrator directs). `git status` currently carries a
large set of **unrelated** pre-existing dirty files (`Content/RawAssets/**` FBX + PNG, the `Tools`
submodule pointer, `CONVENTIONS.md`, `TASKBOARD.md`) — do not sweep them into this commit. Commit on
`main`, **no push**. **Do not save `L_Arena`** — TASK-326 left it dirty-but-content-identical.

**Verdict handoff:** `qa-passed` — TASK-328 may proceed.
