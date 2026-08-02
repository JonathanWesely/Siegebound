# TASK-361 — [AG-T3] `PlaceAncientGrounds` scatter pass + `USiegeScatterConfig` ancient-ground fields

**Agent:** gameplay-programmer · **Date:** 2026-08-01 · **Status:** ready-for-qa
**Law:** CONVENTIONS "Ancient Grounds + Sorcerer + 180° terrain symmetry (2026-08-01)" §2 "Placement" (+ §1, conformed to, not amended) · Plan §2 · Board `#### TASK-361`
**Scope discipline:** files only. **NO compile, NO Git, NO editor, NO MCP.** `AncientGround.{h,cpp}`, `SummonedUnit.{h,cpp}` and every health-bar file are UNTOUCHED.
**Serialized behind TASK-358** — I edited the file in the state 358 handed over, not the pre-358 state.

## Files touched (3 — exactly my allowed set)

| File | Change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/ScatterConfig.h` | NEW `Scatter|AncientGrounds` block: 6 fields + a class-doc paragraph |
| `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.h` | `AAncientGround` fwd-decl · `SpawnedAncientGrounds` array · `PlaceAncientGrounds` decl + doc · class doc + `ClearScatter` doc |
| `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` | `AncientGround.h` include · the `RunScatterPasses` call site · the `ClearScatter` destroy loop · the ~290-line pass itself |

Assets referenced: none directly. The pair's decal material `/Game/Materials/M_AncientGround` is soft-loaded by `AAncientGround` itself (TASK-359 sets the path in its constructor), so spawning the plain C++ class is sufficient — no BP, no `AncientGroundClass` config field.

---

## 1. ⚠️ THE AUTHORITY THREADING — the one thing to check first

**Both** grounds get the push, from `RunScatterPasses`' own parameter:

```cpp
Primary->InitAncientGround(bAuthoritativeGenerate);   // BattlefieldScatter.cpp:1825
Twin->InitAncientGround(bAuthoritativeGenerate);      // BattlefieldScatter.cpp:1835
```

3-second greps for QA:
```
$ grep -c "InitAncientGround(bAuthoritativeGenerate)" BattlefieldScatter.cpp   →  2
$ grep -c "HasAuthority"                              BattlefieldScatter.cpp   →  0
$ grep -n  "InitAncientGround("                       BattlefieldScatter.cpp   →  only those 2 lines (+ comments)
```
There is **no** `HasAuthority()` anywhere in `BattlefieldScatter.cpp` — not before this task, not after. The flag reaches the pass because I **added a second parameter**: see §2.

**Why the call site cannot be skipped on the client:** `PlaceAncientGrounds(Seed, bAuthoritativeGenerate)` is placed **before** `RunScatterPasses`' `if (!bAuthoritativeGenerate) { … return; }` early-out, so the client runs the pass, spawns its own local pair at the identical deterministic position, and pushes `false` into both. TASK-359's diagnostic then prints, per ground:
```
[AncientGround_0] AncientGroundInit authoritativeBoost=false P=(…) halfExtent=(840, 840)
```
**On a client, both lines must read `false`; on the host/standalone, both must read `true`.** A client printing `true` is a regression in *my* threading, exactly as TASK-359 predicted.

## 2. ⚠️ DELIBERATE SIGNATURE DEVIATION — `PlaceAncientGrounds(int32 Seed, bool bAuthoritativeGenerate)`

CONVENTIONS §2 and the board both write the pass as **`PlaceAncientGrounds(int32 Seed)`** — one parameter — while the *same clause* requires `InitAncientGround(bAuthoritativeGenerate)` to be called with the flag `RunScatterPasses` carries. **Those two sentences cannot both be satisfied literally.** The alternatives were:

1. **A second parameter** (what I did) — explicit, order-independent, matches `RunScatterPasses(Seed, bAuthoritativeGenerate)`'s own shape, and the parameter is *visible at the call site* so a future reader cannot lose it.
2. Stash the flag in a member during `RunScatterPasses` — hidden state, order-coupled, and one refactor away from a stale `true` on a client.
3. Read `HasAuthority()` on the scatter inside the pass. **Precisely stated, because the distinction matters:** unlike `AAncientGround`, this actor *is* replicated, so its client copy really is a `ROLE_SimulatedProxy` and the read would return the right answer today — the objection is not that it lies, it is that it **re-derives a decision `RunScatterPasses` already made and is holding in its hand**, splitting one authority decision into two sources that can drift (and modelling exactly the pattern the ground must never copy). The flag is the contract; threading it is the contract being honored.

`PlaceAncientGrounds` is a **private** method that appears **nowhere** in CONVENTIONS §7's pinned cross-task signature registry (only `InitAncientGround(bool)` does), and no other task calls it — so the extra parameter has zero cross-task compile surface. **QA: please rule explicitly** so the deviation is recorded rather than assumed.

## 3. The draw discipline (the hard QA criterion)

```cpp
FRandomStream GroundStream(Seed ^ 0x41474E44);   // "AGND"
...
const float X = -GroundStream.FRandRange(MinAbsX, MaxAbsX);  // draw 1
const float Y =  GroundStream.FRandRange(-MaxAbsY, MaxAbsY); // draw 2
```
- **Exactly two draws per attempt, X then Y**, and they are the **only** two `GroundStream.` occurrences in the whole pass (`grep -c "GroundStream\." → 3`: the constructor + the two draws).
- **Everything downstream is draw-free**: mine clearance, the hill rejection, both ground traces, the rotation, both clearance culls, the fallback, both spawns, every log. Total draws = `2 × attempts-consumed`, so no rejection path can desync the sequence.
- **No yaw roll.** Primary yaw 0, twin yaw a fixed `FRotator(0, 180, 0)` — a cosmetic yaw draw would silently shift every later draw for zero gain.
- **The layer stream and the mine stream gain ZERO draws** — this feature moves no existing layout. Every seed keeps its exact layer field and its exact mine pair; only the new pass's own output is new.
- **Blue half:** `-FRandRange(MinAbsX, MaxAbsX)` ⇒ `X ∈ [−21000, −4000]`; twin at `(−X, −Y)`. Never Red — `PlayerStart` is Blue-side only.

## 4. Sampling band + the new config fields

`Scatter|AncientGrounds`, all six defaults exactly as the law states them:
`AncientGroundHalfExtent (840,840)` · `AncientGroundMinAbsX 4000` · `AncientGroundMaxAbsX 21000` · `AncientGroundMaxAbsY 10800` · `AncientGroundMineClear 1800` · `AncientGroundClearRadius 1200`.

- **Mine clearance is tested at BOTH `P` and `P′`** against the live `SpawnedMines` set (which is why the pass must run *after* `PlaceMines`, which it does — immediately after). Honest at both points even though the mine field is symmetric, because a failed `SpawnActor` would leave the set asymmetric.
- **`RemoveBlockingInstancesInDisc` at BOTH `P` and `P′`**, r = 1200 (circumscribes the 840² box, half-diagonal 1188). Because the two discs are exact rotations of each other over a rotationally symmetric field, this cull deletes **rotational pairs** — symmetry-*preserving*, the same standing TASK-358 gave the mines' aprons, so it is **not** a logged asymmetry escape.
- **Hills REJECTED OUTRIGHT** (`ResolveSurfaceZ` returns "is this point flat?"): `FindHillSurfaceAt` with a **90° gate**, so it can never reject on slope and a non-null `OutSurfaceComp` is exactly the predicate "over a hill". Rejected at either end ⇒ re-roll. **No parity clone, no rollback** — deliberately not resurrecting what TASK-358 deleted.
- **The reserved corridor (`|Y| ≤ 1000`) is NOT excluded** — the mines' ruling, restated in the code comment so nobody "fixes" it.
- **48 attempts**, written as a literal because CONVENTIONS states it as one (it coincidentally equals the mines' `2 × MaxPlacementAttemptsPerInstance` at the shipped 24).
- **Deterministic fallback `(−12000, +6000)`** at `Error`, and deliberately **not** clamped into the configured band: the only property that slot must have is being the same point every time.

## 5. Three additions beyond the literal spec — flagged, not smuggled

1. **A margin clamp on the band:** `MaxAbsX/MaxAbsY` are additionally `Min()`'d against `ArenaHalfExtent − AncientGroundHalfExtent`. At the shipped defaults this is a **provable no-op** (25,160 > 21,000 and 11,160 > 10,800), so the specced band is untouched; it exists only so a mis-tuned DataAsset cannot hang the 840-half footprint over the boundary wall.
2. **A paired-tunable divergence guard:** after spawn, the config's `AncientGroundHalfExtent` is compared against `Primary->GetZoneHalfExtent()` (the public BlueprintPure getter — I did **not** touch `AncientGround`), and a mismatch logs a Warning. This is what makes the deliberate `ACaptureZone` 2-mirror duplication safe to live with: the placement margin and the boost box can no longer drift apart silently. Draw-free, once per generate.
3. **Two free symmetry assertions** (the TASK-358 idiom): `FlatParityBreaks` counts candidates where the flat/hill verdict disagreed between `P` and `P′` (**provably 0** under §1 — a Z-axis rotation leaves both `Z` and `N.Z` invariant), reported **once** after the loop rather than per attempt so 48 attempts cannot spam; and a `|ZP − ZM| > 1` twin-Z mismatch Warning, since the twin Z is **re-traced via `GroundZAt(−X, −Y)`, never copied**. Both emit the existing grep token `SymmetryAssert`. **Neither should ever appear on a healthy run.**

## 6. The log line

```
[BattlefieldScatter 'BattlefieldScatter'] AncientGroundsPass seed=123456 P=(-12873,-4410,0) M=(12873,4410,0) fb=no culls=7
```
Token set is **exactly** the specced `seed / P / M / fb / culls` — I deliberately did **not** add a `groundStream=` token (the mines line has `mineStream=`) so a strict regex on the specced shape cannot break; the stream seed is `Seed ^ 0x41474E44` and is documented in the code. Same binary + same seed ⇒ identical line, and identical on host and client. New non-steady-state lines: the `Error` fallback line, the degenerate-band `Warning`, the `SpawnActor failed` `Warning`, the two `SymmetryAssert` Warnings, the half-extent divergence Warning.

## 7. Lifecycle

`SpawnedAncientGrounds` (`UPROPERTY(Transient) TArray<TObjectPtr<AAncientGround>>`, [P, P′]) is destroyed in `ClearScatter` **exactly like `SpawnedMines`** — destroyed, never pooled, so every re-scatter re-places the pair *and* re-runs the authority push on brand-new actors (a pooled ground could carry a stale `bAuthoritativeBoost` across a Play Again). **No `SiegeGameMode` edit**: Play Again step 7 already calls `ClearScatter()` + `GenerateScatter()`.

## 8. Correct by ABSENCE — please do not file these

- **No keep-clear disc test.** Not specced, and adding it would *tighten* the law rather than enforce it: the `|X| ≤ 21000` ceiling **is** the castle/spawn-box exclusion in closed form (the plan derived it from those geometries, with the r=4500 figure in hand — note `25000 − 21000 = 4000`, so a disc test would actively contradict the specced band edge). The ground has no collision and no nav, so proximity is cosmetic. **Argued in the code comment; QA please rule.**
- **No `RegroundAncientGrounds`.** `IsPointInZone` is a 2D XY box that **ignores Z**, and the decal projects `DecalProjectionDepth` both ways — so even if a defensive cull deleted a hill under a *fallback* ground, neither mechanic nor visual would change. Z is cosmetic-only here, deliberately unlike a mine (miners must walk onto those).
- **No `AncientGroundClass` config field** (unlike `MineClass`) — not specced, no BP variant exists, and the material soft-path lives in the actor's constructor.
- **The pass ignores `SymmetryMode`.** It always emits the rotational pair, even if a designer selected `Asymmetric`. "One per side, placed symmetrically" is a **fairness** requirement from Jonathan's directive, not a terrain-aesthetics toggle; inventing an asymmetric single-ground mode would be unspecced. **Flagged for a QA ruling** — it is the one place I did not branch on `SymmetryMode`.
- **`ScatterConfig.h`'s retired `bMirrorSymmetric` / `SymmetryMode` block is untouched** — that is TASK-358's, already landed.

## 9. Carry-forward from TASK-374 — ALREADY DONE, no action needed

The dispatch warned that `SortOrder = 10` is a `UDecalComponent` property (not a material one) and that TASK-359's agent was being asked to add it **in parallel**. **It is already present**: `AncientGround.h:184-191` declares `UPROPERTY(EditDefaultsOnly) int32 DecalSortOrder = 10;` with the CONVENTIONS §2 rationale, and `ApplyDecalFootprint()` applies it. **Nothing is missing, and I did not edit `AncientGround.{h,cpp}`.**

## 10. Things QA should scrutinize hardest

1. **The two `InitAncientGround(bAuthoritativeGenerate)` calls** — §1. Both present, both from the parameter, neither derived.
2. **The signature deviation** — §2. Needs an explicit ruling.
3. **Zero draws outside the two-per-attempt pair** — grep `GroundStream.` inside `PlaceAncientGrounds`; expect 3 hits total (ctor + 2 draws) and nothing inside `ResolveSurfaceZ` / `ClearsEveryMine` / the fallback / the spawn block.
4. **The no-keep-clear-test decision** — §8, first bullet.
5. **The `SymmetryMode`-independence decision** — §8, last-but-one bullet.
6. **The 90°-gate trick** in `ResolveSurfaceZ` — it relies on `FindHillSurfaceAt` never rejecting at a 90° gate, so `OutSurfaceComp != nullptr` is the whole hill predicate. I still check the return value (`!bResolved` is reachable only if a hit normal had `N.Z < 0`), so an underside hit is treated as "not flat" and rejected — fail-safe, not fail-open.
7. **LWC float/double**: `AncientGroundHalfExtent.X/.Y` are `double` (FVector2D), explicitly `static_cast<float>`-ed before `FMath::Max`, and the divergence compare casts before `FMath::IsNearlyEqual` to avoid mixed-type deduction. The `Candidate.X → float` implicit narrowings match the shipped `FindHillSurfaceAt(Pt.X, …)` / `GroundZAt(P.X, P.Y)` precedent in `PlaceMines`.

## Compile / build-master notes

- **Not compiled** (TASK-366 compiles the batch). This file needs `AncientGround.{h,cpp}` from TASK-359 to exist — it does.
- **No DataAsset edit required.** `DA_BattlefieldScatter` simply gains six properties at their C++ defaults, which **are** the law. Jonathan can tune any of them in the editor without a code change.
- The six new `UPROPERTY`s add no cost to a config that never sets them.

## Multiplayer reality (unchanged, restated so it is not re-litigated)

The pair is **Tier C — not replicated** (TASK-359's declaration): both machines build it locally from the Tier-A replicated `ChosenSeed`/`GenerationIndex`, so it is bit-identical on host and client, and only the host's copies run the boost sim. In M8 P1 units are server-only, so the *boost* is host-verifiable only until P2 — the *placement* is verifiable on both machines right now by diffing the `AncientGroundsPass` line.
