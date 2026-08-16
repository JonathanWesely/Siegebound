# TASK-575 — [WR-21] THE BOT'S CASTLE-DERIVED GEOMETRY — `BotCastleSpawnOffset` + `TowerDefenseStandoff` + the `570` floor

**Agent:** gameplay-programmer · **Date:** 2026-08-15 · **Status → `ready-for-qa`**
**Law:** CONVENTIONS `WR-§2b` rows C + F + G · rulings **`W2-R2`** / **`W2-R3`** · `WR-§9` outcome 9 · `SC-§34` · `SC-§22` · `SC-§18c` · `SC-§15` · `SC-§33` + "Castle 3× HOLLOW" (plinth dead-zone RETIRED) + the bot decision-trace law
**Gate:** `qa/TASK-565.md` · **Compile:** TASK-566 (the batch's only) · **PIE:** TASK-569 rows (n)(o)(p) · **Commit:** TASK-570

⛔ No compile, no editor, no MCP, no PIE, no Git, no `Content/`, no `.csv`, no `Tests/`. **Two files, both mine alone.**
⛔ **NO TOKEN FIGURE IS QUOTED ANYWHERE IN THIS HANDOFF** (batch-wide ban).
✅ **TASK-557's diff is INTACT** — verified by diffing against `HEAD`: its `SpawnBoxHalfExtent (2460,2460) → (7380,7380)` line still shows as *its* uncommitted change alongside mine, unmodified. I read `handoffs/TASK-557-programmer.md` in full before typing; this task is the ruling its rows S3 and S7 were flagged **for**.

**Files touched — exactly the two in my `names:` block:**
`Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h` · `Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp`
**NOT touched:** `SpawnBoxHalfExtent` · `SpawnBoxAnchorInset` · `ClampAnchorToBotSpawnRegion` (body) · `BotSpawnLaneSpread` · `CastleRedFallbackLocation` · `Castle.{h,cpp}` · `SiegePlayerController.{h,cpp}` · `SiegeGameMode.{h,cpp}` · `SummonedUnit.{h,cpp}` · `ScatterConfig.h` · `BattlefieldScatter.{h,cpp}` · any `Content/` · any `.csv` · `Tests/`.

---

## 0. `SC-§33` — THE MECHANICAL PROOF, NOT THE SENTENCE

**Command, run over the two owned files, comment lines filtered out:**

```
git diff -U0 -- Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.h \
                Source/GitClaudeUnrealTest/Siegebound/SiegeBotController.cpp \
  | grep -E "^[+-]" | grep -vE "^(\+\+\+|---)" \
  | grep -vE "^[+-]\s*(\*|//|/\*\*|\*/|~)" | grep -vE "^[+-]\s*$"
```

**Result: THREE new functions, ZERO defaulted parameters, ZERO changed signatures on anything that already had call sites.**

```
+ const ACastle* GetCastleRedActor() const;                          // NEW, 2 call sites, no default args
+ float ResolveCastleFaceDistance(const FVector2D& Direction2D);     // NEW, 2 call sites, no default args
+ float ResolveCastleFrontAnchorOffset();                            // NEW, 3 call sites, no default args
```

`SC-§33` binds **a defaulted parameter added to a function that already has call sites.** All three additions are brand-new functions with every parameter **required**, and `GetCastleRedLocation`'s signature is byte-identical (8 call sites, none edited). ⇒ **The law does not fire, and that is proven mechanically rather than asserted.** ✅ **The gate can re-run the command above verbatim.**

**The hazard this task DOES carry is the neighbouring class — a SEMANTIC change to two tunables with no signature change at all** (no compiler diagnostic, no test, `SC-§34`'s "green and subtly wrong"). So I ran the equivalent enumeration for it in §4 and it is the thing to re-run at the gate.

---

## 1. WHAT CHANGED, IN ONE PARAGRAPH

Both constants stop being **distances from the castle CENTRE** and become **bands past the castle's WALL FACE**, added at runtime to `Castle_Red`'s live colliding bounds. One new primitive, `ResolveCastleFaceDistance(Direction2D)`, is the only place in the controller that measures the castle; `ResolveCastleFrontAnchorOffset()` wraps it for the three castle-front anchor sites so the band can never be added at two of three. `GetCastleRedLocation()` was refactored through a new `GetCastleRedActor()` so bounds and location are always read off the **same** actor. `BotCastleSpawnOffset` `1750 → 1343.15` (the band, not a new total). `TowerDefenseStandoff` **stays 750** — only its meaning changed. The `MinStandoff = 570.f` literal is re-derived as `max(570, face + 150)`; 570 survives **only** as the degenerate-bounds floor, and the tombstone comment that promised byte-identity now says the promise was deliberately given up and why.

---

## 2. ⚖️ RULING `W2-R2` — `BotCastleSpawnOffset`, AND WHY IT IS **NOT** `5250`

Implemented exactly as ruled, with the reasoning recorded **in the header** so it cannot be re-litigated from the code:

- **Neither prior ruling is retired.** M7.6 #1 is a **DIRECTIVE** (materialize in front, then march); CASTLE-3X #3 is a **PERMISSION** (an anchor landing inside the hollow castle is no longer *rejected*). What is retired is **the READING** that #3 authorised leaving `1750` alone. That sentence is now in the property's doc block.
- **`5250` refused.** It preserves the **531-uu** post-CASTLE-3X clearance, and nobody ever chose 531 — it is what `1750` decayed into when CASTLE-3X failed to re-derive it. Multiplying an accident by three preserves the accident.
- **`1343.15` is the human-chosen quantity restored**, and it is expressed **structurally** — as the band, never as a new total, so the row cannot rot a third time.

### The before/after table the spec asked for — BOTH castles, plus M1 for lineage

Half-extents are the law's own recorded bounds (`WR-§0`: CASTLE-3X `2437.9 × 2461.5`, 9× `7313.7 × 7384.5`) ⇒ `Ex` = 1218.95 / 3656.85. ⚠️ These are **mesh** bounds; the code reads **colliding** bounds (`bOnlyCollidingComponents = true`), which the 22-hull `UCX` set can make a few uu tighter — so treat the resolved figures as the arithmetic, and TASK-569's log readback as the measurement.

**Resolved anchor offset from the castle centre (rule-1 unit · rule-2b Deep Mine fallback · rule-4 wave):**

| castle | `Ex` | BEFORE | clearance past the wall face BEFORE | AFTER | clearance AFTER |
|---|---|---|---|---|---|
| M1 (~814 wide) | ~407 | 1750 | **+1,343** (what Jonathan chose) | **1,750.15** | **+1,343.15** |
| CASTLE-3X (shipped today) | 1,218.95 | 1750 | **+531.05** (nobody chose this) | **2,562.10** | **+1,343.15** |
| 9× (TASK-555's target) | 3,656.85 | 1750 | **−1,906.85 ⇒ INSIDE THE HALL** | **5,000.00** | **+1,343.15** |

⭐ **The M1 row is the argument.** The new mechanism reproduces Jonathan's own `1750` to within **0.15 uu** at the castle he authored it against, and holds the same clearance at both later castles. A ×3 could not do that at any of the three.

### ✅ (4) RE-CHECKED AGAINST THE ANCHOR-CLAMP LAW — AND I CHECKED IT BOTH WAYS

Red castle at `+25,000`, `SpawnBoxHalfExtent (7380,7380)` ⇒ box `[17,620, 32,380]`, clamp limit `7,340` (`7380 − SpawnBoxAnchorInset 40`).

| | resolved anchor world X | in the box? | clamp behaviour |
|---|---|---|---|
| CASTLE-3X | 25,000 − 2,562.10 = **22,437.90** | ✅ `[17,620, 32,380]` | `IsPointInBotSpawnBox` **passes ⇒ pass-through** |
| 9× | 25,000 − 5,000.00 = **20,000.00** | ✅ `[17,620, 32,380]` | `IsPointInBotSpawnBox` **passes ⇒ pass-through** |

**And the belt to that brace:** both offsets (2,562.10 · 5,000.00) are also **below the 7,340 clamp limit**, so even if the pass-through carve-out did not exist the clamped path would be a **no-op**. ⇒ ⛔ **`ClampAnchorToBotSpawnRegion`, `SpawnBoxAnchorInset` (40), `BotSpawnLaneSpread` and the two `IsOnOwnHalf` clamps are UNEDITED** — TASK-557 was right that they are structurally sound because they read the tunables.

**The tower anchor clears the same test:** at 9× the standoff resolves 4,406.85 from the castle centre in any direction ⇒ `|ΔX|, |ΔY| ≤ 4,406.85 < 7,380` ⇒ inside the box, pass-through, no clamp pull.

---

## 3. ⚖️ RULING `W2-R3` — `TowerDefenseStandoff` + THE `570` FLOOR

Fixed here even though it rotted **at CASTLE-3X, not at this batch** — the new standing rule (*a batch that makes a latent defect three times worse owns it*) is recorded in the property's doc block so the next reader knows why a nine-times-scale task touched a three-times-scale bug.

**Resolved standoff from the castle centre, intruder due-centerline (unclamped by the "short of the intruder" term):**

| castle | `Ex` | BEFORE | vs the wall face | AFTER | vs the wall face | floor BEFORE (`570`) | floor AFTER |
|---|---|---|---|---|---|---|---|
| M1 | ~407 | 750 | +343 (as designed) | **1,157** | **+750** | 570 → +163 | `max(570, 557)` = **570** |
| CASTLE-3X | 1,218.95 | 750 | **−468.95 INSIDE** | **1,968.95** | **+750** | 570 → **−648.95 INSIDE** | **1,368.95** (+150) |
| 9× | 3,656.85 | 750 | **−2,906.85 INSIDE** | **4,406.85** | **+750** | 570 → **−3,086.85 INSIDE** | **3,806.85** (+150) |

⛔ **The ×3 ban re-derived independently, as ordered:** `2,250 − 3,656.85 = −1,406.85` ⇒ **a ×3 is still 1,407 uu inside the castle.** Confirmed; the multiply is not merely inelegant here, it is arithmetically incapable of fixing the row.

**The `570` literal (spec items 5 + 6).** Its provenance is the retired `CastlePlinthClearance` **420 + 150**, sized against a ~814-uu castle. I split it the way it was always built:

- **150 survives untouched** — it is the body/margin half of `420 + 150`, and `WR-§1` forbids scaling a body quantity. It is now `constexpr float TowerStandoffFaceMargin = 150.f` and is added to the **measured** face instead of to a transcribed 420.
- **570 survives ONLY as `TowerStandoffDegenerateFloor`** — the `FMath::Max` floor that wins when the castle is unresolvable, where it reproduces today's behaviour exactly. Every live path derives.
- ⛔ **`CastlePlinthClearance` is NOT resurrected.** Grep confirms zero live symbols; the tombstone is still a comment.
- ✅ **The tombstone at `SiegeBotController.h` is UPDATED, not deleted (spec item 6).** It now quotes its own retired sentence, then states that the byte-identity guarantee **was deliberately given up here** — because the guarantee itself became the defect: it faithfully preserved a number that sits 3,087 uu inside the 9× footprint.

⭐ **The upper clamp is untouched:** `FMath::Clamp(face + 750, MinStandoff, FMath::Max(MinStandoff, IntruderDist − 100))`. The "stay 100 uu short of the intruder" behaviour and its `Max` guard are byte-identical; only the two inputs are now face-relative. A close-in intruder still pulls the tower inward, and the derived floor is what stops it being pulled into the keep.

---

## 4. ⛔ THE ENUMERATION THAT ACTUALLY MATTERS HERE — EVERY READ OF A SEMANTICALLY CHANGED TUNABLE

A semantic change with an unchanged type is the hazard `SC-§33` is shaped for, so I ran its enumeration by hand.

```
grep -rn "BotCastleSpawnOffset" Source/ Content/ Config/     →  13 hits
grep -rn "TowerDefenseStandoff" Source/ Content/ Config/     →   3 hits
```

| symbol | CODE reads | disposition |
|---|---|---|
| `BotCastleSpawnOffset` | **1** — `ResolveCastleFrontAnchorOffset` (`.cpp:1268`) | **(i)** the single place the band is added. The three former direct reads (rule-1 unit `.cpp:501`, rule-2b Deep Mine `.cpp:688`, rule-4 wave `.cpp:860`) now call the helper. **3 of 3 converted.** |
| `TowerDefenseStandoff` | **1** — rule-1 building branch (`.cpp:488`) | **(i)** `FaceDistance + TowerDefenseStandoff`. |
| both | the remaining hits | comments/doc, all updated or deliberately historical (§6). |

⛔ **The rule-2b Deep Mine site is the one that would have been missed and is the reason this table exists.** It is not named in the spec's prose, but leaving it reading `BotCastleSpawnOffset` as a bare centre offset would have put the Deep Mine at **1,343** from the 9× castle centre — **deeper inside the keep than the stale 1,750 ever was.** A partial semantic change is strictly worse than the stale constant.

**Zero hits in `Tests/`, `Content/`, `Config/`.** Both symbols live entirely inside `SiegeBotController.{h,cpp}`.

---

## 5. ⛔ THE OVERRIDE-PATH DIAGNOSIS — DONE, AND IT COMES OUT THE OPPOSITE WAY FROM TASK-576's

TASK-576 has to fight a saved DataAsset. **I checked whether I have the same problem and I do not — reported because a check that finds nothing is a RESULT (`SC-§22`).**

| probe | result |
|---|---|
| `find Content -iname "*bot*"` | one unrelated texture. **No `BP_SiegeBotController` asset exists anywhere in `Content/`.** |
| `grep -rl "BotCastleSpawnOffset\|TowerDefenseStandoff" Content/` | **0 files** — no saved asset serialises either property. |
| who spawns the bot | `ASiegeGameMode.cpp:50` `BotControllerClass = ASiegeBotController::StaticClass()` (C++ ctor default), spawned at `:1105`. |
| the GameMode itself | `Config/DefaultEngine.ini:4` `GlobalDefaultGameMode=/Script/GitClaudeUnrealTest.SiegeGameMode` — **the C++ class, not a BP.** The only BP game mode in the project is `BP_MenuGameMode`. |

⇒ ✅ **The C++ header defaults ARE the live values. This edit is NOT inert and needs no editor step.** ⚠️ Note for TASK-569: **do not add a DataAsset/BP chore for this task by analogy with TASK-576** — there is no asset to edit, and inventing one would be a no-op.

---

## 6. `SC-§22` — THE SWEEP OF THE TWO FILES I OWN

The spec's item (7) named three comment sites. Treating that citation as a **lower bound**, I swept both files for every claim my change could falsify.

| site | claim | disposition |
|---|---|---|
| `.cpp` rule-2a `IsOnOwnHalf` note | *"…dominates this half clamp at an **840** box"* | **(i)** FIXED. Lineage recorded (840 → 2460 TASK-349 → 7380 TASK-557) and the claim **re-checked, not assumed**: the box clamp limit is 7,340 about `+25,000`, so a box-clamped X never drops below **17,660** — far own-half of `BotHalfBoundaryX` (0). Also recorded that it is **not dead code**: it still binds on the capture-zone pass-through path. |
| `.cpp` rule-2b `IsOnOwnHalf` note | same *"840 box"* | **(i)** FIXED, same correction and same re-check. |
| `.cpp` `ComputeValidBotSpawnPoint` preamble | *"ineligible ones (castle-front at BotCastleSpawnOffset 1,750 ⇒ ~910 uu outside an 840 box…)"* | **(i)** FIXED — **both halves were dead.** Rewritten to state what is now true: the castle-front anchor **takes the pass-through**, and what the clamp still owns is the far-flung rule-2 mine anchors. |
| `.h` `SwarmSpawnRadius` doc | *"with the castle-relative spawn (**~23,250** from the centerline)"* | **(i)** FIXED — ⚠️ **not in the spec's list; my change is what falsified it** (25,000 − 1,750). Now ~20,000 at 9×, and restated as a **relationship** so it cannot rot again. The 300-radius claim itself is unaffected. |
| `.h` `ClampAnchorToBotSpawnRegion` "Why it is needed" | quoted the 840 box and a flat 1,750 | **(i)** re-framed as explicit **HISTORY** + a "where that stands today" line. ⚠️ Also not in the spec's list — declared. |
| `.h:142` rule-1 summary — *"BotCastleSpawnOffset in front of Castle_Red"* | **(ii)** LEFT — it became **more** true, not less. |
| `.h` `BotSpawnLaneSpread` 900 · `SwarmSpawnRadius` 300 · `UnitSpawnClearance` 150 · `BuildingClearance` 200 · `MinerNodeApproachOffset` 400 · `NavProjectionExtent (200,200,1000)` · `SpawnBoxAnchorInset` 40 · `BotHalfBoundaryX` 0 · `CastleRedFallbackLocation` | **(ii)** none is castle-derived — formation width, body clearances, mine-relative approach, nav-snap extent, algorithm margin, arena coordinates. `NavProjectionExtent.Z` 1000 was checked against the 9× interior floor (z ≈ 174) and still reaches it. |
| `.cpp` `RingRadii[] = {0, 250, 500, 800, 1100}` | **(ii)** NOT castle-derived (anchor-local anti-stack walk-out, TASK-265) and deliberately **not** enlarged — see §7, where it is the evidence instead. |

---

## 7. ⚠️ THE MECHANISM BEHIND `WR-§9` OUTCOME 9 — WHY A WAVE IN THE RED HALL WAS **INEVITABLE**, NOT OCCASIONAL

Worth stating because it makes the acceptance unambiguous. `ComputeValidBotSpawnPoint`'s widening ring tops out at **1,100 uu** (`.cpp` `RingRadii`). At the 9× castle the old anchor sat **1,906.85 uu inside the wall face** ⇒ ⛔ **not one ring sample could reach outside the wall.** Every rule-1 unit and every rule-4 wave that placed at all placed **inside the keep**; the same arithmetic kills the rule-1 tower (old floor 570 ⇒ 3,086.85 inside, ring reach 1,100). ⇒ **This was not a probabilistic wrinkle. It was a guaranteed outcome, and `WR-§9` outcome 9 is right to call it a DEFECT.** The repair is at the anchor, which is why the ring stays at 1,100.

---

## 8. ⚠️ DEPARTURES DECLARED (`SC-§15`) — TWO, BOTH ON CHECKABLE SPEC LINES

**D1 — I used the EXACT per-direction face distance for the tower, NOT `WR-§2b` row B's `FMath::Max(BoxExtent.X, BoxExtent.Y)`.**
Row B prescribes `max(Ex, Ey)` for `ASummonedUnit::DefendRadius`, and TASK-574 is landing that. **I did not copy it, and the reason is that row B's constant is a DISC RADIUS — one scalar for every direction, so `max` is the only option available to it.** The tower has a **known** direction (toward the intruder), so the exact face is available and is strictly better. **The arithmetic, so the gate can rule:** the bounds are a world AABB, so the boundary along a unit direction is the nearest axis crossing, `min(Ex/|dx|, Ey/|dy|)`. At 9× on a 45° approach, `max(Ex,Ey) + 750 = 4,442.25`, while the true boundary that way is `min(3656.85, 3692.25)/0.7071 = 5,171.9` ⇒ ⛔ **`max` would place the tower ~730 uu INSIDE the footprint on a corner approach.** `max` is the *inscribed* square, not the circumscribed one. Along ±X (the common case and the only direction the castle-front anchor uses) the two are identical, so the departure costs nothing where they agree.
✅ **If QA prefers batch-wide idiom consistency over corner correctness, this is a two-line revert** — but I judged a defensive tower placed inside the keep to be the exact defect this ruling exists to end.

**D2 — the `570` literal is RE-DERIVED-AND-DEMOTED rather than deleted.**
The spec allowed *"re-derived from the same bounds **or** RETIRED with the reason recorded in place."* I did **both halves of the first option**: the live path derives (`face + 150`), and 570 survives only as the `FMath::Max` floor for unresolvable bounds, where deleting it would have **changed degenerate-path behaviour for no benefit** (the floor would have dropped 570 → 150). This is `WR-§2b`'s own governing sentence — *"the authored value demoted to a floor"* — applied literally. **If QA reads the ruling as requiring outright deletion, say so and it is a one-line change.**

📌 **Also declared, though not a departure:** two comment sites outside the spec's three (`.h` `SwarmSpawnRadius`, `.h` `ClampAnchorToBotSpawnRegion`) were fixed because **my own change falsified them** — the "while you are in the file" rule (`WR-§2b` row G), not scope creep.

---

## 9. ⚠️ WHAT A HUMAN SHOULD OBSERVE — THE ACCEPTANCE INSTRUMENT IS TASK-569 ROWS (n)(o)(p)

TASK-564 will **not** unit-test this (the manager ruled such a test would assert `GetActorBounds`, i.e. the engine, not our logic). ⇒ **Everything below is read off the existing decision-trace lines with no editor probe and no new logging.**

**(n) Rule-4 attack wave** — log line `[Bot …] Rule 4 (Attack): played unit '…' castle-front (X, Y, Z) — marching`.
- ✅ **PASS:** `X ≈ 20,000` for Red — i.e. **~5,000 in front of the castle centre**, ~1,343 uu clear of the wall face. Tolerances that are NOT failures: `Y` anywhere in ±900 (the lane spread) and up to ~1,100 uu of ring walk-out in any direction if the first samples are blocked.
- ⛔ **FAIL:** `|X − 25,000| < 3,657` — that is **inside the footprint**, i.e. `WR-§9` outcome 9's defect. **Visually: the wave must appear on the open field OUTSIDE the front wall and then march. A wave appearing in the Red hall is a FAIL, not a designed outcome.**

**(o) Rule-1 defensive UNIT** — `Rule 1 (Defend): played unit …`. Same anchor, same pass/fail band as (n).

**(p) Rule-1 defensive TOWER** — `Rule 1 (Defend): played building … at (X, Y, Z) vs intruder '…'`.
- ✅ **PASS:** the tower stands **outside the wall, between the intruder and the castle** — distance from the castle centre ≈ face + 750 (**≈4,407** at 9×), pulled inward toward the intruder only when the intruder is closer than that, and never nearer than face + 150 (**≈3,807**).
- ⛔ **FAIL:** a tower that materialises inside the keep, or one that appears *behind* the castle relative to the intruder.

**(extra, cheap, and worth one grep) — the log must NOT contain:**
`ASiegeBotController '…': no usable Castle_Red colliding bounds …`
If that Warning appears, **the derivation degraded** and both constants silently reverted to bands-used-as-bare-offsets — everything above becomes invalid and the real bug is a missing/unloaded `ACastle`, not this arithmetic. It is emitted **once**, at `Warning`, on `LogGitClaudeUnrealTest`.

⭐ **The resolved figures follow TASK-555's delivered mesh automatically.** If the artist's 9× castle lands anywhere inside `WR-§0`'s ±10 % band, **no code change is owed** — that is the entire point of the structural escape, and it is the property TASK-569 should confirm rather than the specific number 5,000.

---

## 10. THE BOT DECISION-TRACE LAW + M8

- ⚠️ **DECISION-TRACE LAW HONOURED:** still exactly **one `LogSiegeBot` line per FIRED rule**. ⛔ **No new per-tick logging.** The one line I added is a **one-shot** `LogGitClaudeUnrealTest` `Warning` behind the `bWarnedNoCastleBounds` latch — the same posture and the same category as the shipped `bWarnedNoNavData`, and deliberately **off** `LogSiegeBot` because it is a diagnostic, not a decision. Not cleared in `ResetBot` (an unresolvable castle is an environment fault, not per-match state) — same choice `bWarnedNoNavData` already makes.
- 📌 **M8 DECLARED:** **no replicated property, no new replicated class, no new relevancy tier, no RPC.** `BotCastleSpawnOffset` / `TowerDefenseStandoff` remain `EditDefaultsOnly` design-time data, identical on both machines by construction; `bWarnedNoCastleBounds` is a plain transient bool, **not** a `UPROPERTY`; the three new functions are private, non-`UFUNCTION`, server-side-only (the bot controller exists only on the authority). **No seeded stream is touched** — the pre-existing `FMath::FRandRange` lane spread is unchanged in count and order.

---

## 11. WHAT QA (TASK-565) SHOULD SCRUTINISE HARDEST

1. ⭐ **§8 D1 — exact per-direction face vs row B's `max(Ex, Ey)`.** This is my one deliberate divergence from a sibling row's idiom. **Re-run the 45° arithmetic yourself.** If you rule for consistency, it is two lines.
2. ⭐ **§8 D2 — is keeping `570` as a degenerate floor "re-derived", or does the ruling demand deletion?** Rule on it explicitly; I read `WR-§2b`'s "demoted to a floor" as authorising it.
3. ⛔ **§4 — the rule-2b Deep Mine site.** It is not named in the spec. Confirm all **3 of 3** castle-front reads went through the helper, and that no fourth read exists. A partial semantic change is worse than the stale constant.
4. **The value `1343.15`, not `1343`.** I chose the manager's parenthetical (`5,000 − 3,656.85`) over the headline so the 9× resolve is exactly `5,000.00`. If that reads as false precision against a `~407` approximation, say so — it is a one-token change and I have no attachment to the hundredths.
5. **`GetCastleRedLocation`'s refactor must be behaviour-identical.** Same iterator, same `IsValid` + team filter, same first-match-wins, same `CastleRedFallbackLocation` fallback. 8 call sites, none edited. **Diff it line by line — this is the one place I touched shipped behaviour that was not supposed to change.**
6. **The centre I measure from.** I add the derived distance to `GetCastleRedLocation()` (the actor origin) while the extent comes from `GetActorBounds`, whose origin can in principle differ. **This mirrors `ASiegeGameMode::ResolveHeroStart` branch 3 exactly, deliberately** — and it is required, because the spawn box is centred on the actor location too, so using the bounds origin would desync the anchor from the box test in §2's table. Confirm that is the right call.
7. **§5 — the override-path diagnosis.** Re-run the four probes; if a BP subclass of `ASiegeBotController` exists anywhere I missed, this edit is inert and the finding is a blocker.
8. **§7's ring arithmetic** (1,100 reach vs 1,906.85 inside) — it is the claim that turns `WR-§9` outcome 9 from "sometimes" into "always". Check it.

---

## 12. NOT DONE, BY DESIGN

⛔ No compile · no editor/MCP/PIE · no Git · no `Content/` · no `.csv` · no `Build.cs` · no `Tests/` · no `L_Arena` · no navmesh claim (`WR-§3` — I made **no** prediction about interior navigability; TASK-569 measures it) · **no new trailing defaulted parameter** (§0) · **no token figure quoted** · **no new UPROPERTY, no renamed property** (both keep their names so the `names:` block, the board and Jonathan's editor bookmarks all still resolve — the semantic change is documented at the property instead) · ⛔ **`CastlePlinthClearance` NOT resurrected** · ⛔ **TASK-557's eight initialiser lines NOT touched.**
