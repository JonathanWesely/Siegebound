# TASK-358 — [AG-T0] 180° rotational symmetry rewrite of the scatter + delete the dead hill-parity rollback

**Agent:** gameplay-programmer · **Date:** 2026-08-01 · **Status:** ready-for-qa
**Law:** CONVENTIONS "Ancient Grounds + Sorcerer + 180° terrain symmetry (2026-08-01)" §1 (+ the amended `:130` placement-symmetry law and the amended `:177` mines law) · Plan §1
**Scope discipline:** files only. **NO compile, NO Git, NO editor, NO MCP.** `SummonedUnit.{h,cpp}` NOT touched (TASK-360 owns it).

## Files touched (exclusive ownership for this batch — TASK-361 serializes behind me)

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h` | NEW `EScatterSymmetryMode` UENUM; `bool bMirrorSymmetric` **DELETED** → `EScatterSymmetryMode SymmetryMode = Rotational180`; class + mines doc rewritten to the rotational law |
| `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.h` | Class doc gains the three binding rules of the law; `ScatterLayer` / `PlaceMines` / `FindHillSurfaceAt` / `RemoveBlockingInstancesInDisc` docs rewritten |
| `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` | The transform, the Blue-half draw, the inline twin, the mines conversion, the rollback deletion, the escape logs |

Assets referenced: none new. No DataAsset edit required (see "Build-master note").

---

## 1. The transform

`ScatterLayer` — primary X draw narrowed to the Blue half, twin emitted **inline**:

```cpp
const float DrawMaxX = bRotSym ? 0.f : HalfX;
const float X = Stream.FRandRange(-HalfX, DrawMaxX);   // Blue half: X ∈ [−HalfX, 0]
...
const FVector2D TwinPoint(-X, -Y);                     // was (-X, Y)
const float TwinYaw = FMath::Fmod(Yaw + 180.f, 360.f);
const FTransform TwinXf(FRotator(0.f, TwinYaw, 0.f), FVector(-X, -Y, TwinZ), FVector(Scale));
```

- **Blue half, not Red** — `PlayerStart` exists only at (−23800, 0, 100); the Red half would rotate a legal prop onto the hero spawn.
- **ZERO RNG draws in the rotation step.** Only the primary X draw's *range* moved; the number and order of draws per attempt is byte-identical (X → biased-Y (2 draws) → mesh → scale → yaw).
- **Inline, not a post-pass.** All three invariants preserved and commented at the site: `VisualToProxy` index parallelism (twin visual + twin proxy added in lockstep), `SpacingGrid.Add(TwinPoint, …)` so later primaries respect the twin, and `HillSurfaceComponents` completeness for pass-1 blockers.
- **Twin Z is RE-TRACED** via `GroundZAt(-X, -Y)` + the hill upgrade, never copied — and the result is now **asserted** against the primary's (tolerance 1 uu). Mismatch ⇒ still placed at the honest traced Z, `Warning` logged (first 3 per layer), total on the layer line as `zMismatch=`.

## 2. ⚠️ THE PLAN'S "FIVE SITES" FOR THE MINES WAS SIX — QA please confirm this

The plan (§1 point 2) and the task spec both list `:1192`, `:1331`, `:1379`, `:1391`, `:1425`. **They omit the twin `SpawnActor` call** (originally `:1409`), which is the line that actually moves the shipped mine actor:

```cpp
// BEFORE
AGoldNode* Twin = World->SpawnActor<AGoldNode>(ResolvedMineClass, FVector(-P.X,  P.Y, ZM), FRotator(0.f, 180.f, 0.f), SpawnParams);
// AFTER
AGoldNode* Twin = World->SpawnActor<AGoldNode>(ResolvedMineClass, FVector(-P.X, -P.Y, ZM), FRotator(0.f, 180.f, 0.f), SpawnParams);
```

Had I converted only the five named sites, every keep-clear test, ground trace, clearance cull and log line would have described a rotated twin while the mine itself still sat at the X-mirror position. **All six are converted.** Twin yaw left at 180 as specced (it was already the law's yaw).

## 3. The deleted hill-parity rollback — the unreachability argument

Deleted from `TryResolveMinePair`: the clone / re-trace / rollback / un-bury block (~55 lines), plus its two out-params `OutInjectedSide` and `OutFootprintCulls` (it was their only producer) and their local mirrors in the mine loop. `TryResolveMinePair` keeps everything still live: the two floor traces, the two slope-gated `FindHillSurfaceAt` calls with the reject-on-over-slope early return, and `bOutOnHill`.

**Why it is provably unreachable (this is the argument QA checks):**

1. `P′ = (−P.X, −P.Y)` is the exact 180° rotation of `P` about the map center, and **the hill field is itself rotationally symmetric**: `ScatterLayer` now emits every hill instance's `(−X, −Y, yaw+180)` twin on the **same HISM**, inline, during **pass 1** — i.e. before `PlaceMines` runs. (Pass-1 layers have `bAllowOnHills=false`, so hills never trace against themselves, and `HillSurfaceComponents` is frozen after pass 1.)
2. A 180° yaw rotation about the world Z axis through the origin is a **proper rigid motion** mapping each hill instance `H` exactly onto its twin `H′`. The down-trace at `P′` therefore strikes `H′` at precisely the rotated image of the point the trace at `P` strikes on `H`.
3. That rotation leaves **Z invariant**, and leaves the surface normal's **Z component** invariant — and `N.Z` is the *only* quantity the slope gate reads (`acos(Clamp(BestNormal.Z))`, `BattlefieldScatter.cpp` `FindHillSurfaceAt`). So `FindHillSurfaceAt` returns the **same hit/miss verdict and the same Z at both points**: `bHillP == bHillM` and `OutZP == OutZM`, always. "Either-side-has ⇒ both-have" holds **by construction** — there is never a bare side to clone onto, so `if (bHillP == bHillM) return true;` was the only reachable exit.
4. The deleted path existed **only** as a workaround for the retired X-mirror, which was a *fake* reflection — the old code says so at its own rollback site: *"yaw+180 mirrors the mesh's LOCAL Y, so an edge-of-hill primary can mirror off the clone's footprint."* A true rotation cannot miss, so the rollback has nothing to roll back.

**Defensive residual (deliberate, please do not file):** the `bHillP != bHillM` branch is kept as a **loud Warning**, not a reject. If a future level edit ever de-symmetrized the field, both ends already carry their own honestly traced Z (`FindHillSurfaceAt` yields the floor Z off-hill), so nothing floats and nothing is buried — the pair just ships with one end on a mound. Loud, not fatal.

## 4. Config + the grep-able log token

`bool bMirrorSymmetric` → `EScatterSymmetryMode SymmetryMode`, values `Rotational180` (**default = the law**) and `Asymmetric` (the retired M6.5 organic-random field, kept only as an explicit off-state; selecting it needs a **new Jonathan ruling**). The X-mirror mode is **not** re-offered — CONVENTIONS retires it outright.

**The `mirror=` KEY is preserved** so QA's existing grep keeps matching; only the value changed:

```
[BattlefieldScatter 'BattlefieldScatter'] GenerateScatter seed=123456 mirror=rot180 layers=4 corridorHalfY=1000
```

`mirror=rot180` (default) | `mirror=asymmetric`.

## 5. Log-format changes QA should note

| Line | Change |
|---|---|
| `GenerateScatter … mirror=%s` | value `true/false` → `rot180`/`asymmetric`. Key unchanged. |
| `Layer '%s': placed …` | **added** `sym=`, `pairs `, `twinSkipped=`, `zMismatch=`. All pre-existing keys (`placed`/`target`/`blocking`/`meshVariants`) untouched. |
| `MinesPass … [i] P=… M=… hill=… fb=… culls=…` | **`inj=` token REMOVED** (the injection it reported no longer exists); `culls=` is now exactly `CullsP + CullsM`. Every other token and the overall shape is unchanged, so the TASK-258 same-seed⇒identical-line criterion still holds. |

New grep tokens: **`SymmetryEscape`** (a local symmetry break fired) and **`SymmetryAssert`** (a should-be-impossible invariant broke). **On a healthy run neither should ever appear.**

## 6. Asymmetry escapes — logged, per CONVENTIONS §1

- `CullCorridorBlockers` — logs `SymmetryEscape` at `Warning` when it removes anything. Only ever called from `ValidateTraversability`'s failure path. *(Analysis note: the band `|Y| ≤ B` is itself a rotation-symmetric region, so in practice the rotational pairs fall together — logged anyway, because "in practice" is not a guarantee.)*
- `RemoveBlockingInstancesInDisc` — logs its removals at `Log` (center/radius/count), because the **verdict is the caller's**: `PlaceMines` calls it at `P` **and** at its exact rotation `−P`, so those two calls delete rotational **pairs** and are symmetry-*preserving* (the expected every-match case). The genuine escape is `ValidateTraversability`'s single-mine repair disc, which now logs its own `Warning`-level `SymmetryEscape` naming exactly that.
- `ScatterLayer` twin skip — counted per layer, `Warning` `SymmetryEscape` at the end of the layer if non-zero.

## 7. Two deliberate behavior decisions QA should rule on rather than assume are bugs

**(a) Twin yaw is now `Fmod(Yaw+180, 360)` UNCONDITIONALLY.** The shipped mirror block wrote `Layer.bRandomYaw ? Fmod(Yaw+180,360) : 0.f` — i.e. a fixed-yaw layer's twin kept yaw 0. That was a fake-reflection artifact. CONVENTIONS §1 states the transform with no `bRandomYaw` exception, and a proper rigid rotation rotates the mesh too, so a fixed-yaw twin at yaw 0 would face the wrong way. **Changed on purpose.**

**(b) The outer loop budget halves under the law.** `InstanceCount` counts **primary + twin** (CONVENTIONS §1: "target 340 becomes ~170 pairs"), so the loop now runs `DivideAndRoundUp(InstanceCount, 2)` iterations in `Rotational180` and the full `InstanceCount` in `Asymmetric`. Each iteration still gets the full `MaxPlacementAttemptsPerInstance` budget. **This is required** — the old `bMirrorSymmetric` path ran `InstanceCount` iterations *and* emitted a twin from each, i.e. it silently **doubled** the instance count (a latent bug in a mode that never shipped). With the halved budget the AddInstance/perf budget (~15,000) is identical to the old asymmetric field, which is the plan's explicit claim. `DivideAndRoundUp` means an odd target rounds **up** (341 ⇒ 171 pairs ⇒ 342 placed) — so `placed` may read 1 over `target` on odd layers. Expected, commented.

## 8. ⚠️ EXISTING SEEDS NOW PRODUCE NEW LAYOUTS — EXPECTED, NOT A REGRESSION

Ruling 8 / CONVENTIONS §1. Documented precedent: **TASK-140** did exactly this (the draw-order change) and recorded it the same way. The contract that **must** hold and that I have preserved is:

- **Intra-build reproducibility** — same binary + same seed ⇒ identical layout and byte-identical log lines. Guaranteed by zero draws in the rotation step and by no change to the per-attempt draw count/order.
- **host == client** — `RunScatterPasses(Seed, bAuthoritativeGenerate)` is untouched; the client's `OnRep_GenerationIndex` path runs the same deterministic passes off the replicated `ChosenSeed`. The rotation is pure arithmetic, so it agrees on both machines.

Cross-*build* layout stability was never a contract. Please do not file the new layouts as a defect.

## 9. What I deliberately did NOT do

- **No `L_Arena`/DataAsset/editor work.** Files only.
- **Castle yaw is untouched** — both castles read yaw 0 so both gate blockers face world −Y. That is CONVENTIONS §1's flagged residual (i), needs a fresh Jonathan ruling, and is an `L_Arena` save. Out of scope.
- **`FindHillSurfaceAt`'s signature is unchanged** even though `OutInstanceIndex`'s only consumer (the parity clone) is gone. `OutSurfaceComp` is still live (the on-a-hill predicate) and **TASK-361 needs this function** for its slope gate — I left the signature stable rather than churn it under the next task. Documented as informational in the header.
- **`MaxMineAttempts` left at 2×** even though one reject cause disappeared — it is a tunable, not a derived number, and shrinking it would move the fallback rate for no gain.

## 10. Things QA should scrutinize hardest

1. **The unreachability argument in §3** — it is the load-bearing justification for deleting shipped safety code. If step 1 (hill field is rotationally symmetric before `PlaceMines`) is wrong, the deletion is wrong.
2. **All six mine Y sites** (§2) — grep `-P.X` in `PlaceMines` and confirm every one is paired with `-P.Y`.
3. **Zero draws in the twin block** — grep for `Stream.` / `MineStream.` inside the `if (bRotSym)` block and inside `TryResolveMinePair`. Should be none.
4. **The halved outer loop** (§7b) — confirm you agree `InstanceCount` means total-including-twins.
5. **Visual/proxy lockstep** for the twin — the twin visual and twin proxy must be added adjacently with no early-out between them.
6. **Pre-existing hazard I did NOT change or fix** (recording it so it is not read as new): if `bUsesProxy` is true but `ResolveProxyForVisual` returns nullptr, the visual gains an instance and the proxy does not, desyncing the indices. This is shipped behavior on both the primary and (now) the twin path; `bUsesProxy` already guards on `!CollisionProxyMesh.IsNull()`, so it needs an unresolvable soft pointer to trigger. Out of scope — flagging, not fixing.

## Build-master note (for whoever integrates)

`DA_BattlefieldScatter` serialized `bMirrorSymmetric = false`. That property no longer exists, so it is dropped silently on load and the asset takes `SymmetryMode`'s default `Rotational180`. **That default flip IS the behavior change this task ships — no DataAsset edit is required.** If you want the retired asymmetric field back for an A/B, set `SymmetryMode = Asymmetric` on the DA (and note that needs a Jonathan ruling to ship).

## Multiplayer reality (per board ruling 13)

Unchanged by this task and restated so it is not re-litigated: in M8 P1 units are server-only. The Red client will see the rotated terrain and mines at bit-identical positions (they ride the Tier-A replicated seed) but no units. This feature is play-verifiable single-player/host only until M8 P2.
