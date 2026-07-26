# TASK-265 handoff — Bot spawn anchors: clamp into the spawn region + unit anti-stack clearance + spawn-Z diagnosis

Status: **ready-for-qa**
Agent: gameplay-programmer
Branch: m7.6-arena10x (base commit `933fee4`)
Date: 2026-07-23
Lane: branch (SiegeBotController frozen on main until the Phase-6 merge)
Code-only — NOT compiled, NOT committed (build-master owns both at TASK-266).

## Files edited (2, and only these 2)
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h`
- `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp`

`git status` confirms no other file in `Source/` is touched. `SiegePlayerController.{h,cpp}` was **not opened**, `SpawnUnitSwarm` untouched, `CaptureZone.{h,cpp}` consumed as-is, no cards.csv / L_Arena / Git.

---

## PART 1 — CONFIRM FIRST (the spec's gate). Diagnosis CONFIRMED by code reading.

Box bound with `SpawnBoxHalfExtent` (840,840) about Castle_Red (X ≈ 25,000; `CastleRedFallbackLocation` = (25000,0,0)):
**eligible X ∈ [24,160 … 25,840], Y ∈ [−840 … +840].** `RingRadii` tops out at **1,100** uu (`ComputeValidBotSpawnPoint`).

| # | Rule / site | Anchor as authored | In box? | Consequence |
|---|---|---|---|---|
| 1 | Rule 1 DEFEND, **tower** (`.cpp` ~:448–453) | `CastleRed + Dir2D * Standoff`, Standoff ∈ [570, 750] | **YES** (radius ≤ 750 < 840) | none — passes through untouched |
| 2 | Rule 1 DEFEND, **unit** (~:460) | `CastleRed + (−1750, rand ±900, 0)` ⇒ X ≈ **23,250** | **NO**, 910 uu outside | only the widest ring's +X sample clears ⇒ pile-up |
| 3 | Rule 2a **MINER** (~:545–554) | mine + 400 toward castle, X clamped ≥ 0 ⇒ X ≈ 1,900–22,000 | **NO**, thousands outside | `ComputeValidBotSpawnPoint` fails ⇒ **`return` at :577 stalls the ladder** |
| 4 | Rule 2b **DEEP MINE** (~:596–612) | mine location (X clamped ≥ 0), else castle-front X ≈ 23,250 | **NO** | same — `return` at :637 stalls the ladder |
| 5 | Rule 4 **ATTACK** (~:769) | `CastleRed + (−1750, rand ±900, 0)` ⇒ X ≈ **23,250** | **NO**, 910 uu outside | pile-up (build-master's X 24,032–24,448 cluster) |

**Stacking arithmetic (matches the observation exactly).** From X = 23,250 the only ring samples that reach X ≥ 24,160 are on the **radius-1,100 ring**, and only near angle 0: radius 1,100 @ 0° ⇒ X = 24,350 ✓; @ ±45° ⇒ X = 24,028 ✗; radius 800 @ 0° ⇒ X = 24,050 ✗. So **one** candidate direction survives for every wave, every time — X is pinned at ~24,350 (then nav-projected, which snaps neighbouring candidates onto the same navmesh polygon point → the observed *identical* XY 24192,−192). The float is consistent with capsules collision-adjusting off each other once two units are served the same point.

**The rule-2 economy stall is REAL (manager's prediction confirmed at the code level), and worse than "a stalled tick":**
`SiegeBotController.cpp:577` (`return; // rule 2 fired (Miner)`) sits **inside** `if (CardIndex != INDEX_NONE)` and **outside** the `if (ComputeValidBotSpawnPoint(...))` success branch. So a bot that (a) holds a Miner card, (b) can afford it, (c) has `AliveMinerCount < TargetMinerCount`, and (d) has a `FindBestMineFor` result, **returns every single decision tick without evaluating rules 3/4/5**. And because the spawn never happens, `ConfirmPlayFromHand` is never called — the Miner **stays in hand forever** and `AliveMinerCount` **stays 0 forever**, so conditions (a)+(c) never clear. Once a Miner reaches the bot's hand the ladder is stalled *for the rest of the match*. Identical structure for 2b at :637 with a Deep Mine.
This also explains why TASK-264's PIE still saw rule-4 attack waves: that run's bot simply had no affordable Miner in hand at those ticks (or `FindBestMineFor` was null). **PIE confirmation is TASK-266 acceptance (d)** — grep `LogSiegeBot` for a `Rule 2 (Economy)` line; it should now appear.

No evidence contradicted the diagnosis, so I proceeded to the fix.

---

## PART 2 — The anchor clamp

New private const helper, applied in **exactly one place** — the top of `ComputeValidBotSpawnPoint`, right after the world null-check:

```cpp
const FVector Anchor = ClampAnchorToBotSpawnRegion(Desired);
```

`Anchor` then feeds **both** the degraded no-nav path and the ring walk. No call site does anchor math; all five anchor sites are covered by that one line.

### ⚠ The carve-out conditions, explicitly (this is the load-bearing part)

`ClampAnchorToBotSpawnRegion` returns `Desired` **completely untouched** when **either** of these holds:

1. `IsPointInBotSpawnBox(Desired)` — the anchor is already inside the Red castle box, **or**
2. `IsPointInCapturedZone(Desired)` — the anchor is already inside a **Red-OWNED** `CaptureZone_Center` (this call is `Zone->CanTeamSpawnHere(ETeamId::Red, Point)`, the TASK-260 seam, which folds "inside the zone" **and** "Red owns it"; a Neutral or Blue-owned zone returns false).

That pair is **the same pair the spawn gate itself uses** in `IsBotHalfPointClear`, so "clamped" and "eligible" can never disagree. The clamp only ever moves a point the gate would have rejected anyway. Rule-1 **tower** anchors are the concrete beneficiary today (radius ≤ 750 < 840 ⇒ always pass-through, byte-identical behavior).

### The clamp itself (when neither carve-out fires)
Per-axis clamp of the castle-relative delta into ±(`SpawnBoxHalfExtent` − `SpawnBoxAnchorInset`), i.e. ±800 by default. `Desired.Z` is **preserved** (nav projection owns the final Z). The plinth is deliberately **not** special-cased — the existing ring walk-out owns it, and at ΔX = −800 the clamped anchor is already outside the 420 plinth keep-out anyway.

New UPROPERTY: `float SpawnBoxAnchorInset = 40.f` (EditDefaultsOnly, `Siegebound|Bot|Placement`, ClampMin 0). Its job is to keep the anchor off the exact edge so the ring has room on **both** sides — an anchor pinned on the boundary throws half its candidate ring out of the box, which is the pile-up mechanism itself.

### 🚩 FLAGGED DEVIATION from the board's literal wording — manager/QA ruling requested

The board says "*else per-axis clamps ΔX/ΔY about `GetCastleRedLocation()`*" — i.e. **always** the castle box. I implemented the clamp to target **the nearer of the two eligible regions** (castle box, or a Red-owned capture zone). Why:

- The board's own stated intent for the pass-through is "*it keeps the bot able to stage inside a Red-owned CaptureZone_Center, TASK-264 PIE result (f)*". **The specified mechanism cannot deliver that intent**: *no anchor in this class is ever computed inside the mid zone* (castle-front anchors sit at X ≈ 23,250; mine anchors at |X| ≥ ~1,100). The pass-through therefore can never fire for the zone on its own.
- TASK-264 result (f) — "*the bot demonstrably staged 2 units mid-field only while Red held the zone*" — was produced by the **rule-2 mine anchors' ring-search reaching into the zone** (a mirrored mine at X ≈ 1,500 gives an anchor at X ≈ 1,900; the 1,100 ring reaches X ≈ 800, inside the ±840 zone). A castle-box-only clamp pulls those anchors to X ≈ 24,200 and **deletes that behavior**, which would fail TASK-266 acceptance (e).
- It would also leave the bot **structurally unable to ever spawn in a zone it owns**, while the player can — contradicting Jonathan's directive verbatim ("*when captured, you can spawn units there*").

Properties of what I shipped:
- **No new identifier** beyond the four the board names — it reuses `CanTeamSpawnHere` / `GetZoneHalfExtent` / `GetActorLocation` from the TASK-260 API, exactly the path the task brief pointed at.
- **M7.6 ruling #1 is untouched.** Castle-relative anchors (rule-1 unit, rule-4 attack) are always nearer the castle box than to mid, so attack waves still spawn castle-front and march. The bot never gets a free forward spawn for its army; only the far-flung rule-2 **economy** anchors can prefer the mid zone, and only while Red holds it.
- **Inert by default.** With no zone placed, a Neutral zone, or a Blue-owned zone, the block cannot fire and behavior is byte-identical to the board's literal spec.
- **Cheap to revert**: delete the block marked `--- FLAGGED DEVIATION ---` in `ClampAnchorToBotSpawnRegion` (and the matching header paragraph) and rename `BoxClamped` → the returned value. ~10 lines.

`BotCastleSpawnOffset` (1,750) is **KEPT, not deleted** — its header comment now records that it is inert at an 840 box and re-activates untouched if the box grows. `BotSpawnLaneSpread`, and the two `IsOnOwnHalf` anchor clamps at ~:551/:602, are code-unchanged (each got **one comment line** noting the box clamp now dominates, which the spec explicitly permits). `IsOnOwnHalf`'s TARGET/APPROACH callers (~:877, ~:900) and its definition are **byte-unchanged**.

---

## PART 3 — Anti-stack clearance

New UPROPERTY: `float UnitSpawnClearance = 150.f` (EditDefaultsOnly, `Siegebound|Bot|Placement`, ClampMin 0; **0 disables**).

Enforced in `IsBotHalfPointClear`, **for `!bIsBuilding` only**, placed between the plinth keep-out and the existing `BuildingClearance` block, and mirroring that block's shape exactly (precomputed squared distance, `DistSquared2D`, `TActorIterator`):

```cpp
if (!bIsBuilding && UnitSpawnClearance > 0.f)
{
    const double UnitClearanceSq = FMath::Square(static_cast<double>(UnitSpawnClearance));
    for (TActorIterator<ASummonedUnit> It(World); It; ++It)
    {
        const ASummonedUnit* Unit = *It;
        if (!IsValid(Unit) || Unit->IsUnitDead()) { continue; }
        if (FVector::DistSquared2D(Unit->GetActorLocation(), Point) < UnitClearanceSq) { return false; }
    }
}
```

Both teams count (a Blue unit standing in the Red box is just as much an obstacle). Buildings keep the building rule and are **not** subject to this one. `TActorIterator` + `SummonedUnit.h` were already in the file — no new include needed for this part.

Effect: unit N takes the clamped anchor; unit N+1 is refused there and the deterministic ring walks it to the next free sample (33 candidates: 1 + 4×8). Combined with the per-wave random lane Y (`FRandRange(±BotSpawnLaneSpread)` → clamped to ±800), successive waves also start from different anchors. Capacity sanity: the box is 1,680×1,680 minus the 840×840 plinth ≈ 2.1 M uu²; at 150 uu spacing that is far more slots than a wave needs, and units march away.

**Player side deliberately NOT given this rule** (spec + CONVENTIONS: Jonathan just approved the player behavior; a player-side stack is a separate, unreported concern).

---

## PART 4 — Spawn-Z: what I found, and what I did NOT do

**I did NOT add a ground snap.** `SnapPointToGround` and `MaxGroundSnapDrop` are **not** in the code — correctly, per the spec's "confirm then fix, in that order".

**My read: the float is a de-stacking SYMPTOM, not an independent bug.** The evidence for that is circumstantial but strong and all points the same way:
- The two floating units were at **identical XY** (24192, −192). Two capsules spawned at the same point cannot both occupy it; `SpawnUnitSwarm`'s spawn-collision handling resolves the encroachment by displacing one, and the vertical axis is the cheapest escape → a lift.
- ~215–232 uu is the right *order* for a capsule-height escape on these units, and it is a **range**, not a constant — a nav-projection Z error would be far more uniform across a wave.
- The chosen point is `FNavLocation::Location` straight from `ProjectPointToNavigation`, which sits on the navmesh, i.e. on the ground surface by construction. There is no term in the bot's path that adds a couple of hundred uu of Z.

But I did not *assume* it — I added the measurement the spec asked for.

New UPROPERTY: `bool bLogSpawnZDiagnostic = true` (EditDefaultsOnly). When true, **one** line per bot UNIT spawn, on **`LogGitClaudeUnrealTest`** (never `LogSiegeBot` — the one-line-per-FIRED-rule decision-trace law is untouched; I verified no `LogSiegeBot` string in this file changed). Emitted in `SpawnBotCardActor`'s unit branch, after the gold gate succeeds, so unwound spawns never log:

```
ASiegeBotController 'X': [SpawnZ] unit 'Ogre' x1 — chosen Z 12.0, ground Z 0.0 (trace hit), chosen-vs-ground delta 12.0;
actor Z 88.0, capsule half-height 88.0, FLOAT above ground 0.0 (TASK-265 diagnostic — bLogSpawnZDiagnostic).
```

The ground trace is the `BattlefieldScatter::GroundZAt` recipe — downward `ECC_WorldStatic` line trace, `bTraceComplex=false`, bounded to `[chosen Z + 1000 … chosen Z − 5000]`, **ignoring the units just spawned** so a fresh capsule cannot be mistaken for the floor (Pawn capsules block the WorldStatic trace channel, so this guard is load-bearing).

**`FLOAT above ground` is the number that decides the question.** A grounded character capsule sits exactly its half-height above the floor, so **≈ 0 means no float**; a value near 215–232 means a real residual. Reading it needs no knowledge of `SpawnUnitSwarm`'s internals, which is why I measured it this way rather than opening that file.

**Decision rule for TASK-266:** if `FLOAT above ground` ≈ 0 → de-stacking fixed it, change nothing. If a > ~50 uu residual survives → route back to me and I add the capped `SnapPointToGround` (`MaxGroundSnapDrop = 120`, may only LOWER). **Caveat build-master must apply when reading the numbers:** scatter hills leave `ECC_WorldStatic` on **Ignore**, so on a hill this trace reports the floor *under* the hill and will show a large false "float". A residual that only appears near hills is the existing **Ogre-near-hill spawn-lift WATCH**, not this task — and it is exactly why any future snap must stay capped at 120.

---

## What QA should scrutinize

1. **The carve-out** — `ClampAnchorToBotSpawnRegion` returns `Desired` unchanged on `IsPointInBotSpawnBox(Desired) || IsPointInCapturedZone(Desired)`; the clamp is unreachable for eligible anchors. Confirm it is not unconditional.
2. **The flagged deviation** (nearest-eligible-region vs board's castle-box-only) — this is the one judgement call in the pass and it is deliberately isolated in a commented block. Please rule on it explicitly rather than silently accepting.
3. **M3 trace intact** — no `LogSiegeBot` line, format or placement changed; the new diagnostic is on `LogGitClaudeUnrealTest`. `EvaluateDecisions`'s only diff is two pure comment lines.
4. **Include / complete-type law** — added `CollisionQueryParams.h` (FCollisionQueryParams), `Engine/HitResult.h` (FHitResult), `Components/CapsuleComponent.h` (`GetScaledCapsuleHalfHeight` dereferenced). All three follow existing in-module precedent (`BattlefieldScatter.cpp`, `SummonedUnit.cpp`). `ACaptureZone` / `ASummonedUnit` / `TActorIterator` were already included.
5. **Shadow law** — new locals: `Anchor`, `CastleRed`, `BoxLimitX/Y`, `BoxClamped`, `MidZone`, `ZoneIt`, `ZoneOrigin`, `ZoneHalf`, `ZoneLimitX/Y`, `ZoneClamped`, `UnitClearanceSq`, `Unit`, `TraceStart/End`, `GroundParams`, `SpawnedUnit`, `GroundHit`, `bHitGround`, `GroundZ`, `Representative`, `Capsule`, `ActorZ`, `CapsuleHalfHeight`. None shadow `Owner` / `Instigator` / `Controller` / `PlayerState` / `Slot`. (`Anchor` also exists as a local in `FindFireballClusterTarget` — different function, no shadow.) New members: `SpawnBoxAnchorInset`, `UnitSpawnClearance`, `bLogSpawnZDiagnostic` — no `Owner`-named member.
6. **Null-safety** — `MidZone` guarded; `Representative`/`Capsule` `IsValid`-guarded with sane fallbacks; `AddIgnoredActor` only for valid units; trace miss falls back to the chosen Z and says `TRACE MISS` in the line; `GetCastleRedLocation` keeps its +25,000 fallback; the whole diagnostic block is behind `bLogSpawnZDiagnostic`.
7. **Numeric types** — `FVector2D`/`FVector` components are double in UE5; every mixed-type expression is explicitly `static_cast<double>`-ed so `FMath::Max`/`FMath::Clamp` template deduction cannot fail.
8. **Format string** — 10 specifiers / 10 arguments in the new `UE_LOG`, verified by hand.

## Watch item for build-master (not a defect, worth a glance at TASK-266)
Rule-2a/2b anchors preserve `Desired.Z` (the **mine's** Z) through the clamp, as the spec requires. `NavProjectionExtent.Z` is 1,000, so a mine sitting more than ~1,000 uu of Z away from the castle box floor would still fail projection. Hill Z is capped under the ±1,200 nav volume and mines are placed on ≤30° ground, so this should have ample headroom — but if a `Rule 2 (Economy)` line still never appears in PIE, this is the next thing to check (fix would be one line: anchor Z from the castle instead of the mine).

## NOT in scope / untouched
`SiegePlayerController.{h,cpp}` (not opened), `SpawnUnitSwarm`, `CaptureZone.{h,cpp}`, `SpawnBoxHalfExtent`'s value, TASK-262's box gate early-out, `IsOnOwnHalf`'s definition and its TARGET/APPROACH callers, cards.csv, L_Arena, compile, Git.
