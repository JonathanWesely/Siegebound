# DIAG — "Floating units" (W1 playtest, branch m7.6-arena10x)

**Agent:** gameplay-programmer · **Mode:** READ-ONLY diagnosis (no code/editor/MCP/PIE touched — Jonathan is live-playtesting) · **Date:** 2026-07-22

**Symptom:** units (with health bars) elevated well above the ground, "right next to trees," on the 10× arena. **New clue (load-bearing):** the bug correlates with the player SPAWNING a unit on a patch of GRASS.

---

## TL;DR verdict

The units are **not truly floating** — they are standing (correctly, per the M4.5 "terrain carry-in" placement law) on **hill crowns** that W1-PREP made ~1,000 uu tall, **camouflaged into the flat grass field**, and **topped with trees**. When the player clicks what looks like flat grass, the cursor's surface trace actually hits an (invisible-looking) hill crown and the unit is placed up there next to the trees that now grow on hills.

The **tree collision-proxy walkable-top theory is largely REFUTED** as the cause (see Hypothesis C). The **nav-projection-snap-up theory is REAL but is a bot/Barracks-only latent bug**, not the observed player symptom (see Hypothesis B).

Root drivers are the three W1-PREP hill changes: hill scale 0.9–1.3× → **0.4–2.5×** (TASK-251), the **M_HillGrass** material that "near-camouflages hills into the field" (TASK-249), and **trees/grass now placing on hills** (`bAllowOnHills=true`, TASK-249/250). This is a NEW regression on this branch.

---

## The placement Z chain (what actually sets a unit's spawn height)

| Spawn path | Where Z comes from | Vertical window | Snaps UP onto elevated surfaces? |
|---|---|---|---|
| **Player unit card** (single) | Cursor `ECC_Visibility` trace → `Hit.ImpactPoint` (`SiegePlayerController.cpp:1890-1896`, `:1712-1720`, `:1212`), spawned verbatim at `Center` (`:2162`, `:2179`) | n/a (world trace) | **YES — onto hills/rocks/slabs** (they block Visibility). NOT onto tree proxies (Pawn-only). |
| **Player swarm ring** (Militia Mob=4) | Nav projection of each ring point, `RingProjectExtent` Z=**200** (`:2158`, `:2170-2172`) | ±200 | Slightly (up to +200) |
| **Player placement VALIDATION** | `IsPointOnNavmesh`, `NavProjectionExtent` Z=**50** (`SiegePlayerController.h:551`, `:2247-2248`) | ±50 | Tight — refuses roofs by design (does NOT set spawn Z) |
| **Bot ring-search** (buildings/defensive) | Nav projection, `NavProjectionExtent` Z=**1000**, `OutPoint = Projected.Location` verbatim, **no ground re-trace** (`SiegeBotController.h:347`, `.cpp:1128/1134`) | ±1000 | **YES — up to ~+1090** |
| **Bot attack waves** | Fixed flat castle-front, projected on open ground | — | **No — log shows Z=20** (flat) |
| **Barracks** | Nav projection, `NavProjectionExtent` Z=**1000**, verbatim, no re-trace (`Barracks.h:94`, `.cpp:117-119`) | ±1000 | **YES** |

**Key law (`SiegePlayerController.cpp:1712-1713`):** *"the SURFACE under the cursor — flat floor, **hill crown**, or flank alike (M4.5 terrain carry-in LAW: **never the Z=0 plane**)."* Placing on a hill crown is a deliberate, existing behavior. W1-PREP just made hills tall + invisible.

---

## Ranked hypotheses

### RANK 1 — MOST LIKELY: terrain carry-in onto a camouflaged, over-tall hill (fits the grass/spawn clue)
- Player places a unit → cursor `ECC_Visibility` trace hits a **hill** (real-geometry blockers block Visibility: `BattlefieldScatter.cpp:510-517`, esp. `:514`). Tree proxies do **not** block Visibility (`:596`), so the cursor passes through them.
- `TargetingLocation`/`PlacementLocation = Hit.ImpactPoint` = the hill-crown Z (`:1720`, `:1212`); single-unit spawn uses it verbatim (`:2162/2179`). Validation (`IsPointOnNavmesh`, Z=50) PASSES because the hill has walkable climbable-terrain nav on top.
- W1-PREP made the crown: **tall** (donor Hill_02 = 400 uu × up to 2.5× = **~1,000 uu**; TASK-251 lines 9/16/20), **camouflaged** (M_HillGrass "near-camouflages into the field … only shading reveals them"; TASK-249 line 38), and **tree-/grass-topped** (`bAllowOnHills=true` on Trees/Rocks/Grass/Plants; TASK-249 line 10; cross-layer props land on hill XYs freely, TASK-250 line 13).
- Net: a unit correctly on a ~1,000-uu camouflaged hill crown, beside trees that grew on the same hill → reads as "floating on grass next to a tree."
- **NEW regression:** pre-W1-PREP hills were 0.9–1.3× (crown ≤ ~520 uu) and **gray/obvious** (TASK-249 calls the old look "the gray 'before'"). Short + obvious → nobody read it as floating.
- **Evidence:** code lines above; TASK-249/250/251 handoffs; git (`f49d8b1` W1-prep fold). Note precedent commit `96ed0b3` ("hero rises on spawn" from unexpected collision geometry) — same class of bug.

### RANK 2 — SECONDARY (bot/Barracks only, latent): nav-projection snap-up with no ground re-trace
- `ComputeValidBotSpawnPoint` (Z-extent **1000**) and Barracks (Z-extent **1000**) take `Projected.Location` **verbatim** as spawn Z. A desired ground point near/under a hill can project UP onto the hill-crown nav (or, narrowly, a small tree-proxy top ≤ ~1090) → the spawned actor floats.
- **Why it's not the observed symptom:** bot **attack waves spawn at the fixed flat castle-front** — every log line reads `castle-front (23250, Y, 20)` (Z=20), and miners at `(…, 20)`. The Z=1000 ring-search only runs for open-field building/defensive placement, which the log doesn't show floating. Still a real latent bug worth hardening.
- **Evidence:** `SiegeBotController.cpp:1128/1134`, `Barracks.cpp:117-119`; log lines 2682/2698/…/2920 (all Z=20).

### RANK 3 — NARROW / mostly REFUTED: tree collision-proxy generates a walkable nav top
- The proxy IS a nav-carving solid: `SetCanEverAffectNavigation(true)` + `bFillCollisionUnderneathForNavmesh=true` on a tall Pawn-blocking cylinder (`BattlefieldScatter.cpp:591-598`). In principle its flat top cap becomes a walkable Recast poly (same mechanism that makes hills climbable).
- **Refuted as the cause because:**
  1. The player's placement Z is a **Visibility** trace; the proxy blocks **Pawn only** (`:596`) → the cursor never snaps to it.
  2. A normal/tall tree's proxy top sits at ≈ `CollisionProxyZOffset + halfHeight` ≈ **~1,700 uu × instance-scale** above ground (`:375-378`), which is **above the ±1,200 nav-bounds Z ceiling** (TASK-251 line 18; NavMeshBounds scale (280,125,12), TASK-218 line 18). Above the ceiling ⇒ Recast makes **no** walkable poly there. Self-limiting.
  3. It survives only as a knife-edge case: a **small** tree (scale ≲ 0.6) whose proxy top is BOTH under the +1,200 ceiling AND inside the bot's ±1,000 window could snap a **bot/Barracks** spawn onto an invisible cylinder top (a true no-support float) — but that path isn't the player-on-grass symptom.
- Worth fixing as cheap defense-in-depth regardless.

### RANK 4 — RULED OUT (as the mechanism): not-settled navmesh / CrowdManager
- The `Navigation still building after 10.0 s` cap fired only on the **first** PIE match (log 2692). Every subsequent match logged `Traversability CONFIRMED … after 0 cull(s)` in ~6 s (log 3075/3916/5638/20346/21296). Floating persists in settled matches → nav-settle is not the mechanism.
- `Unable to find RecastNavMesh instance while trying to create UCrowdManager` (log 20259, 21358) occurs at **Play-Again resets** (transient nav teardown/rebuild), not during steady play. It can produce bad spawns only in the first ~10 s window; not the root cause.

### RANK 5 — RULED OUT: mine hill-injection (TASK-255)
- Observed seeds log every mine `Z=0 hill=no inj=none` (MinesPass lines). No hill injection occurred; floaters are by trees, not mines. Not involved.

**Separate, unrelated issue seen in the log (not floating):** `BP_Unit_Miner_C_3 is stuck and failed to move … Z=88.65 … Actor:BP_Building_DeepMine_C_1` (log 21180-21183) — a miner jammed against a DeepMine building at ground level. This is the pre-existing "DeepMine+Wall retry" item, not the float bug.

---

## Fix options (re-ordered to hit the verdict)

### For RANK 1 (the primary — units on camouflaged tall hills)
- **Option A — ART (art-director lane; RECOMMENDED first):** restore visual distinctness to hills so a crown reads as a hill, not flat grass. M_HillGrass currently *intentionally* camouflages them (TASK-249). Levers: stronger slope/height shading, a rim/AO darkening, or a subtly different macro tint on hills. *Tradeoff:* softens the "continuous terrain" look TASK-249 was chasing.
- **Option B — DESIGN/DATA (art-director/manager lane):** lower the Hill `ScaleRange` from 0.4–2.5 back toward ~0.6–1.5 (crown ~600 instead of ~1,000). *Tradeoff:* loses the "landmark giant hills" TASK-251 added.
- **Option C — GAMEPLAY (my lane; needs a manager ruling first):** guard unit placement against tall hill crowns — compare `Hit.ImpactPoint.Z` to a downward floor re-trace and REFUSE (with card-refused feedback) when the surface is elevated beyond a threshold, OR clamp the placed unit to the flat floor. *Tradeoff:* directly contradicts the M4.5 terrain carry-in LAW (`:1712`) — units are *currently allowed* on flanks/crowns on purpose. Do NOT implement without a manager decision on whether on-hill unit placement is still wanted.

### For RANK 2 (bot/Barracks projection snap-up — my lane, safe hardening)
- **Option D (RECOMMENDED):** shrink `NavProjectionExtent.Z` on `SiegeBotController` (1000→~90) and `Barracks` (1000→~90), matching the player's tight Z=50 placement extent, so projection can't grab a nav poly ~900 uu above the desired ground point. Keep XY. *Tradeoff:* a point whose only nearby nav is >90 up now fails projection → falls back to the raw point / ring-widening (acceptable; those are pathological spots).
- **Option E:** after `OutPoint = Projected.Location`, do a downward `ECC_WorldStatic` ground re-trace and set `OutPoint.Z` to the true floor (or reject if the projected point is > threshold above the floor). *Tradeoff:* a spawn legitimately meant for a hill flank drops to the floor — fine, the bot never needs hill spawns.

### For RANK 3 (tree-proxy island — my lane, defense-in-depth)
- **Option F:** stop the proxy contributing a walkable top while still carving the trunk. Best form: keep `SetCanEverAffectNavigation(true)` but give the proxy HISM a **Null nav-area** (obstacle carve, no walkable surface) instead of raw solid geometry. *Tradeoffs:* `SetCanEverAffectNavigation(false)` alone would regress the M6.6 "units route around the trunk" contract (units would path through trunks); shortening the proxy would stop it blocking pawns for the full tree height. Note: Option D already removes the only realistic way to *reach* these tops, so F is optional.

---

## What a runtime check (Jonathan / build-master, not me — editor is his right now) would confirm
1. In PIE, `show Navigation` — do the hill crowns carry walkable nav islands, and do any **tree** locations carry an *isolated* island below +1,200? (Confirms Rank 1 vs Rank 3.)
2. Select a floating unit and read its Z + what's directly beneath it — a hill mesh (Rank 1, perception/art) vs empty air over a trunk (Rank 3, true float).
3. Whether the floaters are **player-placed** (Rank 1) or **bot open-field** units (Rank 2).

---
---

# DIAG SESSION-2 — LIVE editor diagnosis (Jonathan authorized + paused playtest)

**Agent:** gameplay-programmer · **Mode:** READ-ONLY via Unreal MCP (get_properties / get_bounds only — NO sets, NO saves, NO PIE, editor left running + unsaved) · **Date:** 2026-07-23 · **Branch:** m7.6-arena10x

## SESSION-1 is SUPERSEDED — this is a TRUE float, not a perception/hill artifact

Jonathan's new load-bearing evidence killed SESSION-1's Rank-1 (camouflaged-hill perception) verdict:
1. `show Navigation` confirms the navmesh is ON THE GROUND and the units hover ABOVE it (not standing on a hidden crown).
2. The bug is **unit-type-specific** — ONLY Archer + Ogre float; every other unit is fine. A terrain/perception cause would hit all unit types equally. Unit-type specificity ⇒ per-unit **asset/BP** cause.

Both facts point at the two units first-imported in TASK-242/243. Root cause found and **confirmed with hard numbers**.

## ROOT CAUSE (definitive): `SkeletalVisualMesh` was never given its −HalfHeight Z offset in BP_Unit_Archer / BP_Unit_Ogre

`ASummonedUnit` creates `SkeletalVisualMesh` in C++ at RelativeLocation (0,0,0) (SummonedUnit.cpp:119-124). The M7 skeletal-swap task (TASK-159) hand-authored a `SkeletalVisualMesh.RelativeLocation.Z = −CapsuleHalfHeight` into every unit BP that existed **at that time**, mirroring the static `VisualMesh` offset (comment SummonedUnit.cpp:148-149: "BP_Unit_Footman offsets the mesh down by the capsule half-height"). `ResolveSkeletalVisual()` (SummonedUnit.cpp:229-298) swaps the SK asset in via `SetSkeletalMeshAsset` but **never sets the component's Z** — it relies entirely on the BP having authored the offset. Archer + Ogre were first-imported LATER (TASK-242/243, "ZERO code/BP changes"), so their `SkeletalVisualMesh` kept the C++ default Z=0 and never got the offset. Their SK feet sit at the mesh pivot (Z=0), so with the component at capsule-center the whole body renders one full capsule-half-height above the grounded capsule.

### The comparison table (live MCP reads)

| Unit | Capsule HalfHeight | static VisualMesh Z | **SkeletalVisualMesh Z** | SK feet-at-pivot (bounds origin.z − extent.z) | Predicted mesh float |
|---|---|---|---|---|---|
| **Footman** (control) | 90 | −90 ✓ | **−90 ✓** | +0.03 (feet @0) | 0 — grounded ✓ |
| Knight | 95 | −90 | −90 | — | ~0 (−5 sink, invisible) |
| Longbowman | 92 | −90 | −90 | — | ~0 |
| Cavalry / Pikeman / MilitiaMob / Sapper / Cleric | ~90 | −90 | −90 | — | ~0 |
| **ARCHER** | 90 | −90 ✓ | **0 ✗** (should be −90) | +0.05 (feet @0) | **+90 float** |
| **OGRE** | 145 | −145 ✓ | **0 ✗** (should be −145) | −0.16 (feet @0) | **+145 float** |

SK bounds (local ref-pose): Footman origin.z 89.85 / extent.z 89.83; Archer 90.00 / 89.95; Ogre 143.93 / 144.09. All three have the mesh bottom at ~Z=0 ⇒ **the SK import origins are CORRECT** — this is NOT an asset re-import problem.

### Why the float matches the symptom exactly
- **Triggered on spawn, MAINTAINED on flat terrain, constant height:** a fixed component RelativeLocation offset is rock-constant — it never drifts, exactly matching "same height as it walks." (A capsule-not-grounded bug would bob/fall — ruled out.)
- **Capsule is grounded normally** (CharacterMovement floor-finds the capsule; RelLoc 0, standard config), the **MESH renders above it.** This is the task's "fix #2 (asset/BP-alignment)" mechanism, localized to the **BP component transform**, NOT the SK asset and NOT the spawn code.
- **Magnitude:** Archer +90, Ogre +145 — each equals its own capsule half-height, the signature of "mesh pivot pinned to capsule center instead of capsule bottom."

### Frequency clue reconciled
- Archer floats +90 **every time the skeletal swap takes** (deterministic) — matches "almost every time."
- Ogre floats +145 deterministically too; **"worse near a hill"** is the SESSION-1 Rank-2 secondary stacking on top: the Ogre's oversized capsule (radius 60, HalfHeight 145) + its huge mesh bounds (half-extent 110×114) get nudged UP by hill collision / the `AdjustIfPossibleButAlwaysSpawn` + LiftZ spawn path (SiegePlayerController.cpp:2179-2185) and the nav-projection Z-extents (SESSION-1 Rank 2). That extra lift is additive to the base +145, making the Ogre's float more egregious near elevation. The base +145 is the primary; the hill amplification is secondary.

## VERDICT: fix #2 (alignment), at the BP-component layer — best fixed in CODE. NOT fix #1 (spawn/movement) and NOT an SK re-import.

### Fix A — RECOMMENDED, robust, gameplay-programmer lane (C++)
In `ASummonedUnit::ResolveSkeletalVisual()`, right after `SetSkeletalMeshAsset`, align the skeletal component to the SAME known-good offset the static mesh already uses:
```cpp
// Ground the skeletal mesh: its feet are at the mesh pivot (Z=0), so pin the
// component to the static VisualMesh's authored offset (−CapsuleHalfHeight) —
// don't depend on each BP having hand-authored the SkeletalVisualMesh Z.
if (bVisualMeshBaseCached)
{
    SkeletalVisualMesh->SetRelativeLocation(VisualMeshBaseRelativeLocation);
}
```
`VisualMeshBaseRelativeLocation` is already cached in BeginPlay (SummonedUnit.cpp:154-157) BEFORE LoadStatsAndStart→ResolveSkeletalVisual runs, and the static `VisualMesh` offset is correct in EVERY BP (−90/−90/−145 verified). This fixes Archer + Ogre, is byte-identical for every already-correct unit (their SkeletalVisualMesh already equals their VisualMesh offset), AND permanently closes the "next first-import forgets the offset" trap. (Alt: derive `−GetCapsuleComponent()->GetScaledCapsuleHalfHeight()`; slightly changes Knight/Longbowman by 2-5uu — invisible but not byte-identical, so prefer the VisualMesh-base copy. Implementer: confirm each existing unit's static VisualMesh.Z == its SkeletalVisualMesh.Z so Fix A is a true no-op for them — spot-checked Footman/Archer/Ogre; recommend a quick sweep of the other 8.) Requires a compile (build-master) + QA.

### Fix B — IMMEDIATE hotfix, no compile, art-director / build-master lane (BP data only)
Set `SkeletalVisualMesh.RelativeLocation.Z` = **−90** in BP_Unit_Archer and **−145** in BP_Unit_Ogre (+ save both BPs). Restores parity with the other 9 units with no code change. Downside: leaves the latent trap — every future first-imported rigged unit will float again until Fix A lands. Good as a play-now unblock; Fix A should still follow.

**Owner recommendation:** Fix A is mine (gameplay-programmer) and is the durable answer. If Jonathan wants to keep playtesting THIS session without a compile, hand Fix B to art-director/build-master now and queue Fix A behind it. Either way this is NOT an SK asset re-import (assets are clean) and NOT a spawn/movement-C++ change (capsule + spawn are correct).

### Ruled out this session
- **SK asset import origin** — all three feet-at-pivot Z≈0 (bounds). Clean.
- **Capsule mis-size / capsule off ground (fix #1)** — capsules are standard (RelLoc 0), correctly sized per unit; the mesh, not the capsule, is displaced. MovementMode is irrelevant to a component-offset float (offset applies in any mode).
- **Shared SK_Footman_Skeleton per-mesh offset inheritance** — no; the offset that's wrong lives on the BP component, and every unit shares the skeleton yet only the two late-imports float.
- **SESSION-1 Rank-1 (hills/perception)** — refuted by unit-type specificity + `show Navigation`.

