# TASK-578 handoff — [WR-24] comment-only hygiene, wave 3 (gameplay-programmer, 2026-08-15)

- **Gate:** `.claude/pipeline/qa/TASK-565.md` — this task names TASK-565 as its gate (roster 16).
- **Compile:** TASK-566. **Commit:** TASK-570. This task opened **NO compile of its own**.
- **Status:** `ready-for-qa`.
- **Discipline honoured:** no compile · no editor · no MCP · no PIE · no `Content/` · no `.csv` write · no `Tests/` · no `Build.cs` · no `.uasset` · no Git mutation. **No token figure is quoted, derived or reasoned from anywhere in this handoff** (batch-wide ban).
- **Read-only tools I used and declare (`SC-§15`):**
  - **Git, READ-ONLY:** `git status --porcelain`, `git show HEAD:<path>`. Three read commands, **zero mutation** — no `add`, no `commit`, no `checkout`, nothing staged. TASK-570 remains the only commit.
  - **`Docs/Data/cards.csv`, READ-ONLY.** Item (6) required the *shipped* card costs and the spec explicitly refused to supply them. The CSV is the §3.0 single-source-of-truth. **It was read, parsed and never written** — `git status` shows it clean.
- **Files touched — exactly the ten in `names:`:** `SiegeBotController.h` · `SiegeBotController.cpp` · `SummonedUnit.h` · `SummonedUnit.cpp` · `BattlefieldScatter.cpp` · `Projectile.h` · `Projectile.cpp` · `Castle.h` · `UnitCommand.h` · `SorcererUnit.cpp` (all under `Source/GitClaudeUnrealTest/Siegebound/`).
- **NOT touched:** `SiegeGameMode.{h,cpp}` (TASK-573 — read as the reference only; **it had no phantom, it was never the problem**) · `SiegePlayerState.cpp` · `CaptureZone.h` · `HeroCharacter.cpp` (TASK-577, already swept) · `SiegePlayerController.{h,cpp}` (563) · `WarMapWidget.{h,cpp}` (579) · `ScatterConfig.h` (576) · `Torch.{h,cpp}` · `CommanderNpc.{h,cpp}` · `Tests/` (564) · **every constant named in the spec** (`DefendRadius` · `SpawnBoxHalfExtent` · `AncientGroundMaxAbsX` · `TorchAnchors`) · any `Content/` asset · any `.csv`.

---

## 0. ⛔ THE NAMED POST-CONDITION, RUN AT THE END, PASTED RAW

```
$ grep -rn "ResolveHeroStart" Source/
$ grep -rn "ResolveHeroStart" Source/ | wc -l
0
```

**BEFORE count (my own grep, run first, ⛔ NOT trusted from the spec): 6.  AFTER count: 0.**

My before-list matched the manager's six exactly — but **located by symbol, not by the line numbers**, which the spec correctly withheld:

| # | file | symbol the comment sits in | line at my read |
|---|---|---|---|
| 1 | `SiegeBotController.h` | `BotCastleSpawnOffset` doc block | `:295` |
| 2 | `SiegeBotController.h` | `ResolveCastleFaceDistance` doc block | `:595` |
| 3 | `SiegeBotController.cpp` | `ASiegeBotController::ResolveCastleFaceDistance` body | `:1198` |
| 4 | `SummonedUnit.h` | `ResolveDefendEngagementRadius` doc block | `:1191` |
| 5 | `SummonedUnit.cpp` | `ASummonedUnit::ResolveDefendEngagementRadius`, CONTRACT 2 | `:2199` |
| 6 | `BattlefieldScatter.cpp` | the castle-face-distance derivation | `:2645` |

**Widened beyond the named post-condition (nothing found):**
```
$ grep -rn "ResolveHeroStart" --include=*.cpp --include=*.h --include=*.cs --include=*.py --include=*.ini --include=*.csv .
0
```
📌 **The phantom survives ONLY in pipeline prose, and deliberately so — ⛔ not mine to edit and ⛔ not a defect:** `.claude/pipeline/CONVENTIONS.md` and `TASKBOARD.md` (the manager's naming correction, which records the retired name so it stays recognisable) and the historical handoffs `TASK-557 / 573 / 575 / 576`. **A handoff is a dated record of what was believed then; rewriting one would destroy the evidence trail that makes this finding legible.** Reported, not touched.

✅ **The real symbol was verified at the artifact before a single edit (`SC-§20`), not taken from the spec:** `ASiegeGameMode::GetHeroStartTransform` — declared `SiegeGameMode.h:422`, defined `SiegeGameMode.cpp:642`, and called at `.cpp:282` and `.cpp:618`. **`branch 3` is also real and stays**: it is the `// 3)` block that reads `GetActorBounds(bOnlyCollidingComponents=true)` and takes `FMath::Max(AuthoredFloorX, CastleBoxExtent.X + HeroSpawnCastleClearance)`. **The six comments were citing correct reasoning under a name that never existed** — which is exactly why every implementer repeated it without suspicion.

---

## 1. ⛔ THE PROOF OBLIGATION — spec item (7)

⚠️ **A NECESSARY DIFFERENCE FROM TASK-577'S PROOF, STATED UP FRONT SO THE GATE DOES NOT RE-RUN THE WRONG COMPARISON.** TASK-577's three files were clean at `HEAD`, so `HEAD` was its baseline. **Six of my ten are already dirty from TASK-557/574/575/576, which DID change code.** A `HEAD` comparison on those six would show *their* diffs, not mine. ⇒ **My baseline is a byte snapshot of the working tree taken BEFORE my first edit**, and I give the `HEAD` proof as well for the four files where it is meaningful.

### Proof 1 — the comment-filtered diff

```
$ diff -U0 <pre-edit snapshot> <working tree>       # all 10 files, concatenated
payload +/- lines in the whole change:                      142
  …whole-line comment lines (// or * or /* or */):           98
  …lines carrying a code token:                              44
```

⚠️ **The 44 are NOT a violation, and I am not asking anyone to take that on trust — they are 22 `-`/`+` PAIRS of the deck-list lines, whose comment is a TRAILING comment on a live line.** `Entry(TEXT("Footman"), 12), // 9 x12 = 108` fails a whole-line comment test by construction. So the filter is sharpened rather than relaxed:

```
Proof 1b — strip each code-bearing changed line from '//' onward, sort both sides, diff:
  code-bearing changed lines:  minus=22  plus=22
  code prefixes IDENTICAL:     True          (diff is EMPTY)
```
⇒ **All 22 differ ONLY after the `//`.** The 22 code prefixes are the 22 `Entry(TEXT("<Card>"), <Count>),` statements, byte-identical on both sides **including the count argument** — which is the number that actually matters, since deck legality is counts.

### Proof 2 — the comment-STRIPPED code stream is byte-identical (sha256)

`/* … */` blocks and `// …`-to-EOL removed **with a string/char-literal-aware state machine** (so a `//` inside a `TEXT()` literal is never mistaken for a comment), blank lines dropped, hashed.

| file | code-only sha256 — PRE-EDIT | code-only sha256 — POST-EDIT | verdict |
|---|---|---|---|
| `SiegeBotController.h` | `e5ef9d28…c9102459` | `e5ef9d28…c9102459` | ✅ **IDENTICAL** |
| `SiegeBotController.cpp` | `0d440641…c3e0830d` | `0d440641…c3e0830d` | ✅ **IDENTICAL** |
| `SummonedUnit.h` | `95f92ed2…91661b5a` | `95f92ed2…91661b5a` | ✅ **IDENTICAL** |
| `SummonedUnit.cpp` | `9b57c04a…5bf88071` | `9b57c04a…5bf88071` | ✅ **IDENTICAL** |
| `BattlefieldScatter.cpp` | `6346982d…415a3e02` | `6346982d…415a3e02` | ✅ **IDENTICAL** |
| `Projectile.h` | `7828f382…f894a6e4` | `7828f382…f894a6e4` | ✅ **IDENTICAL** |
| `Projectile.cpp` | `88c350b3…df4cb60c` | `88c350b3…df4cb60c` | ✅ **IDENTICAL** |
| `Castle.h` | `854c281c…7e55c22a` | `854c281c…7e55c22a` | ✅ **IDENTICAL** |
| `UnitCommand.h` | `b194509d…7de58bcd` | `b194509d…7de58bcd` | ✅ **IDENTICAL** |
| `SorcererUnit.cpp` | `e563e0db…ce86a9ae` | `e563e0db…ce86a9ae` | ✅ **IDENTICAL** |

**Full hashes (post-edit), for the gate to re-run:**
```
e5ef9d280954a4fa5f9c2110a16fe5aab2af5999bc081a38566c26a8c9102459  SiegeBotController.h
0d44064188ceff5a30e94354f1e17a77093c981a03c85adf4328a018c3e0830d  SiegeBotController.cpp
95f92ed2ac9c6ffe8f7a8d12a82f0fe986ab1b901dbad8ba1b390b1a91661b5a  SummonedUnit.h
9b57c04a760147ffb665971b2bb6fc25fb5b5ddb477acf98ebaf59b35bf88071  SummonedUnit.cpp
6346982d5d35ea308144557f0281a437d8fec133ab6b18cbee31256f415a3e02  BattlefieldScatter.cpp
7828f382c07855a37b99420b3604184daea27a5ef43acebbe003c7adf894a6e4  Projectile.h
88c350b3e71b3a79490fa1cfa8aaa207cb6f6c13ff6be36745bda3a3df4cb60c  Projectile.cpp
854c281c6c2c9d48972bb366fcc2a58a85e52000e7e73c1460ae81ae7e55c22a  Castle.h
b194509d6800debe1cff9da703e3f6372ab3bab01ccd306c548827477de58bcd  UnitCommand.h
e563e0dba66b7e6517449d62b5884f47564c13c69521fa989f07a035ce86a9ae  SorcererUnit.cpp
```

### Proof 2b — the four clean files, proven against `HEAD` itself (TASK-577's exact standard)

| file | `HEAD` code-only sha256 | working code-only sha256 | verdict |
|---|---|---|---|
| `Projectile.h` | `7828f382…f894a6e4` | `7828f382…f894a6e4` | ✅ **IDENTICAL TO `HEAD`** |
| `Projectile.cpp` | `88c350b3…df4cb60c` | `88c350b3…df4cb60c` | ✅ **IDENTICAL TO `HEAD`** |
| `UnitCommand.h` | `b194509d…7de58bcd` | `b194509d…7de58bcd` | ✅ **IDENTICAL TO `HEAD`** |
| `SorcererUnit.cpp` | `e563e0db…ce86a9ae` | `e563e0db…ce86a9ae` | ✅ **IDENTICAL TO `HEAD`** |

### The stripper was validated before it was believed (`SC-§32`: green is not proof)

1. **Determinism/correctness:** its `HEAD` hash for each of the four clean files **equals** its pre-edit working hash. ✅
2. ⭐ **CODE-SENSITIVITY — the leg that matters, because a tool that always says "identical" proves nothing:** run against the four files where 557/574/575/576 changed real code, `HEAD` vs pre-edit **DIFFERS** for `SummonedUnit.cpp`, `SiegeBotController.h`, `Castle.h`, `BattlefieldScatter.cpp`. **The tool can see a code change; it did not see one of mine.**
3. **Comment-delimiter balance, pre vs post, all ten files:** `/*` and `*/` counts unchanged (e.g. `SummonedUnit.h` 204/204). ⚖️ **An unterminated block comment would have swallowed live code and MOVED the hash** — so proof 2 is also the structural check.

⛔ **No statement, expression, default value, `UPROPERTY` specifier, `meta =` clause, signature, parameter, `#include`, or whitespace on any live line was added, removed or modified.** ⛔ **Not one non-comment line moved** (`SC-§33`: no new trailing defaulted parameter, no signature change — none was added anywhere).

📌 `git status --porcelain` reconciled: the four previously-clean files (`Projectile.{h,cpp}`, `UnitCommand.h`, `SorcererUnit.cpp`) are now dirty and the other six were already dirty. **Every other dirty path belongs to 555/557/562/563/573/574/575/576 or is an untracked handoff. No stray edit. Nothing staged.**

### ⚠️ DECLARED EXPLICITLY, NOT SILENTLY — the UHT metadata consequence (TASK-554 / TASK-577 precedent)

**FOUR of my sixteen edits sit in doc blocks UHT parses**, so on TASK-566's compile the corresponding `.gen.cpp` will regenerate with new `Comment`/`ToolTip` metadata:

| header | block | reflected declaration it documents | regenerates |
|---|---|---|---|
| `Projectile.h` | class doc block | `UCLASS()` `AProjectile` | class ToolTip |
| `Castle.h` | ledger note | `UPROPERTY(EditDefaultsOnly …) FVector2D SpawnBoxHalfExtent` | property ToolTip |
| `UnitCommand.h` | enum doc block | `UENUM(BlueprintType) ESiegeUnitCommand` | enum ToolTip |
| `SiegeBotController.h` | site 1 | `UPROPERTY(EditDefaultsOnly …) float BotCastleSpawnOffset` | property ToolTip |

⚖️ **That is EDITOR-ONLY metadata — a Details-panel tooltip and two type tooltips. No gameplay byte, no behaviour, no replicated property, no prompt character.** The `UCLASS()`/`UENUM()`/`UPROPERTY(...)` lines and the defaults `SpawnBoxHalfExtent = FVector2D(7380.f, 7380.f)` and `BotCastleSpawnOffset = 1343.15f` are **byte-identical** (proof 2).
✅ **`git status` was checked for the STOP condition and it is CLEAN: zero `.gen.cpp`, zero `Intermediate/` paths.** `Intermediate/` is build output, not source, and is in neither this diff nor the index.
**The other twelve edits produce ZERO metadata change:** `SiegeBotController.h` site 2 and `SummonedUnit.h` document **private non-`UFUNCTION` methods** (`ResolveCastleFaceDistance`, `ResolveDefendEngagementRadius`), and the remaining ten are `//` comments **inside `.cpp` files**, which UHT does not parse for metadata at all.

📌 **M8 DECLARATION (spec item 9):** ⛔ **no replicated property, no new replicated class, no new relevancy tier, no RPC, nothing emitted, nothing replicated.** ✅ **And the reason is structural rather than a promise: proof 2 shows the code stream is byte-identical, so there is no mechanism by which anything could replicate differently.**

---

## 2. ⭐ ITEM (1) — THE PHANTOM, ALL SIX SITES. Before → after, verbatim.

⛔ **Only the NAME changed at every site.** The surrounding branch-3 reasoning is correct and is untouched, exactly as the spec required. **Three sites needed a re-wrap** because `GetHeroStartTransform` is 5 characters longer than the phantom and the blocks wrap at ~80 columns; **the re-wrap moved comment text only.**

**(1) `SiegeBotController.h` — `BotCastleSpawnOffset` doc block**
```
-	 *  escape; the same shape ASiegeGameMode::ResolveHeroStart branch 3 already ships).
+	 *  escape; the same shape ASiegeGameMode::GetHeroStartTransform branch 3
+	 *  already ships).
```
**(2) `SiegeBotController.h` — `ResolveCastleFaceDistance` doc block**
```
-	 *  reasoning as ASiegeGameMode::ResolveHeroStart branch 3, deliberately, so the
-	 *  project has ONE castle-measuring idiom.
+	 *  reasoning as ASiegeGameMode::GetHeroStartTransform branch 3, deliberately,
+	 *  so the project has ONE castle-measuring idiom.
```
**(3) `SiegeBotController.cpp` — `ResolveCastleFaceDistance` body**
```
-	// (SC-§34's structural escape; the shipped model is ASiegeGameMode::
-	// ResolveHeroStart branch 3, which has survived two castle resizes untouched).
+	// (SC-§34's structural escape; the shipped model is ASiegeGameMode::
+	// GetHeroStartTransform branch 3, which has survived two castle resizes
+	// untouched).
```
**(4) `SummonedUnit.h` — `ResolveDefendEngagementRadius` doc block**
```
-	 *  ASiegeGameMode::ResolveHeroStart branch 3 already ships).
+	 *  ASiegeGameMode::GetHeroStartTransform branch 3 already ships).
```
**(5) `SummonedUnit.cpp` — `ResolveDefendEngagementRadius`, CONTRACT 2**
```
-	//    reason, as ASiegeGameMode::ResolveHeroStart branch 3 (the shipped model for
+	//    reason, as ASiegeGameMode::GetHeroStartTransform branch 3 (the shipped model for
```
**(6) `BattlefieldScatter.cpp` — the castle-face-distance derivation**
```
-	// reason, as ASiegeGameMode::ResolveHeroStart branch 3.
+	// reason, as ASiegeGameMode::GetHeroStartTransform branch 3.
```

⚖️ **I deliberately did NOT add a "this used to say X" breadcrumb at these six sites,** although that is this batch's house style elsewhere. The spec says *"change ONLY the name and any surrounding words made false by it"*, and six copies of a note about a name that now appears nowhere in `Source/` would be six new maintenance surfaces documenting a non-event. **The record lives in `WR-§2b`, on the board, and here.** Raised so the omission reads as a decision, not an oversight.

---

## 3. ITEM (2) — `Projectile.{h,cpp}`: the `~800x800` claim. ⭐ AND WHY THIS FILE IS THE EVIDENCE.

**Claim traced at the artifact before typing (`SC-§20`).** `AProjectile::GetDistanceToTarget` calls
`InTarget->ActorGetDistanceToCollision(From, ECC_Pawn, OutClosestPoint)` — **it measures against the target's LIVE collision components**, with an actor-origin fallback only when no usable collision exists. ⇒ **bounds-aware and self-deriving; identical mechanism to `HeroCharacter.cpp:428`, which TASK-577 ruled the same way.** ⛔ **The MECHANISM was never stale. Only the transcribed number rotted, and it rotted twice** (M1 `~814x814` → Castle-3× `~2438x2462` → 9× `~7314x7384`, `WR-§0`).

**`Projectile.h` — class doc block. Before**
```
 *    ONLY actor it can damage. Large targets (the castle's ~800x800 base)
 *    impact at their walls, and the closest point doubles as the impact-VFX
 *    point (TASK-020 pattern).
```
**After**
```
 *    ONLY actor it can damage. Large targets — the castle above all — impact at
 *    their WALLS rather than their origin, and the closest point doubles as the
 *    impact-VFX point (TASK-020 pattern).
 *    ⭐ THE "~800x800 base" FIGURE THIS LINE USED TO QUOTE IS RETIRED AS A
 *    NUMBER, NOT AS A REASON: the castle measured ~814x814 at the M1 blockout,
 *    ~2438x2462 after Castle-3× and ~7314x7384 at the 9× castle (CONVENTIONS
 *    WR-§0), so the transcribed figure rotted across two resizes. NOTHING BROKE,
 *    and the reason is structural — GetDistanceToTarget calls
 *    ActorGetDistanceToCollision, which measures against the target's LIVE
 *    collision, so the reach test is bounds-aware and SELF-DERIVING and followed
 *    both resizes by itself. The MECHANISM was never the stale part. ⛔ Do not
 *    re-introduce a castle size here — it would rot again at the next resize
 *    and it is not read by anything.
```

**`Projectile.cpp` — the reach test in `Tick`. Before**
```
		// reach test against the CLOSEST POINT on the target's collision (house
		// pattern, TASK-003/004): the castle's ~800x800 base impacts at its
		// walls, not its origin — and the closest point is the impact-VFX point.
```
**After**
```
		// reach test against the CLOSEST POINT on the target's collision (house
		// pattern, TASK-003/004): a big target like the castle impacts at its
		// walls, not its origin — and the closest point is the impact-VFX point.
		// ⭐ This line used to quote "the castle's ~800x800 base", the M1 blockout
		// figure (~2438x2462 after Castle-3×, ~7314x7384 at the 9× castle —
		// CONVENTIONS WR-§0). It rotted through two resizes and NOTHING BROKE:
		// GetDistanceToTarget measures against the target's LIVE collision, so
		// the test is bounds-aware and self-derives at every scale. See the
		// class doc block in Projectile.h.
```

⭐ **AND THIS FILE IS THE EVIDENCE THE SPEC ASKED ME TO STATE PLAINLY: `Projectile.{h,cpp}` satisfied BOTH halves of row G's own test and got NO OWNER.** Row G's rule is *"a stale comment is fixed by the task already in that file; only the files no task owns get a dedicated hygiene task."* **Every `Projectile` reference on the board belongs to a `done` task (026/035/040/054/068), and no WAR-ROOM task's `names:` listed it.** ⇒ **Row G's enumeration was a lower bound on itself** — found by TASK-577 sweeping the CLAIM rather than the file list, and it is `SC-§22` proving itself on the artifact that cites `SC-§22`.

⚠️ **The same forward-dating judgement TASK-577 declared applies here and I made the same call, deliberately:** `TASK-555` rescaled the FBX **source**, but the `.uasset` import is TASK-566 and has not run, so the loaded `SM_Castle` is still the Castle-3× mesh at this instant. ✅ **Every figure I wrote is LABELLED WITH ITS SCALE GENERATION** rather than phrased as *"the castle is X"* — a generation-labelled lineage is true before AND after TASK-566, and the comment's actual claim is *"these numbers are history and the code does not depend on them."*

---

## 4. ITEM (3) — `Castle.h`: the retired `21,000`

**Traced first:** `ScatterConfig.h` now reads `float AncientGroundMaxAbsX = 16080.f;` (TASK-576 landed it), and the retired `21000` is recorded there as history. ⇒ **`Castle.h`'s cross-note was describing a LIVE FLAG that had already been decided** — worse than a stale number, because it invites someone to re-open a closed ruling.

**Before**
```
	 *  the spawn-box EDGE from |X| = 22,540 to |X| = 17,620, which is the figure
	 *  CONVENTIONS "Ancient Grounds …" names as THE binding constraint on
	 *  USiegeScatterConfig::AncientGroundMaxAbsX (21,000) — that constant is left at its
	 *  law value and FLAGGED to the manager, never silently retuned (WR-§2 row 7's
	 *  reasoning: this directive does not touch objective placement).
```
**After**
```
	 *  the spawn-box EDGE from |X| = 22,540 to |X| = 17,620, which is the figure
	 *  CONVENTIONS "Ancient Grounds …" names as THE binding constraint on
	 *  USiegeScatterConfig::AncientGroundMaxAbsX. ✅ THAT FLAG WAS ANSWERED THE SAME
	 *  DAY AND THIS NOTE IS THE RECORD OF IT: manager ruling W2-R1 (CONVENTIONS
	 *  WR-§2b row E) RE-DERIVED the ceiling 21,000 → 16,080, and TASK-576 landed it
	 *  in ScatterConfig.h. ⛔ IT IS NO LONGER "FLAGGED", IT IS RULED — do not re-open
	 *  a closed ruling. 16,080 is not a new margin: it re-solves the SAME relationship
	 *  against the 17,620 edge this note reports, preserving the original 1,540 centre
	 *  margin and the original 700 uu footprint-edge clearance exactly. ⚠️ Two
	 *  artifacts, and NEITHER is this header's: the C++ default lives in
	 *  ScatterConfig.h (TASK-576) and the saved DA_BattlefieldScatter is TASK-569's
	 *  editor step.
```
⛔ **`SpawnBoxHalfExtent` stays `(7380, 7380)`** — byte-identical (proof 2). ⛔ **`AncientGroundMaxAbsX` was not touched; it is not in this file.**
⚠️ **The retired `WR-§2 row 7` citation was DROPPED, not preserved as history, and that is a deliberate departure worth a QA eye.** Row 7 rules `ACaptureZone::ZoneHalfExtent` deliberately-unchanged; this note borrowed its *"this directive does not touch objective placement"* reasoning to justify leaving the ceiling alone. **W2-R1 decided the opposite** — the ceiling WAS retuned — so keeping the citation would preserve a *reason that was overruled*, which is the `SC-§22` "true conclusion resting on a dead mechanism" shape pointed the wrong way. **Row 7 itself is untouched and still governs the zone.**

---

## 5. ITEM (4) — `UnitCommand.h`: the DEFEND mechanism clause

**Before**
```
 *  - Defend (E): units fall back toward the own castle, fighting only the
 *    enemies attacking it (within DefendRadius).
```
**After**
```
 *  - Defend (E): units fall back toward the own castle, fighting only the
 *    enemies inside a disc centred on that castle.
 *    ⚠️ THE MECHANISM CHANGED IN TASK-574 (CONVENTIONS WR-§2b row B) AND THIS IS
 *    THE HEADER A READER OPENS TO LEARN WHAT DEFEND *MEANS*, so it is stated
 *    here rather than left to the unit: DefendRadius IS NO LONGER THAT DISC. It
 *    is the BAND PAST THE OWN CASTLE'S WALL FACE, and the acquisition radius is
 *    DERIVED at every decision — the castle's LIVE colliding half-width plus the
 *    band — by ASummonedUnit::ResolveDefendEngagementRadius, which is the ONLY
 *    supported reader of the value. ⛔ Never read DefendRadius as a centre
 *    radius: at the 9× castle the colliding half-width is ≈3,657 uu, so the old
 *    2,500 disc lay ENTIRELY INSIDE THE KEEP and DEFEND acquired nobody, ever.
```
**Every clause traced at the artifact:** `SummonedUnit.cpp:1660` calls `AcquireEnemyNearPoint(OwnCastle->GetActorLocation(), ResolveDefendEngagementRadius(OwnCastle))`; the resolver computes `max(CastleBoxExtent.X, CastleBoxExtent.Y) + DefendRadius` from a `bOnlyCollidingComponents = true` query; `SummonedUnit.h`'s own property doc states the ≈3,657 half-width and the 2,500-disc failure. ⛔ **`DefendRadius = 1281.f` untouched — it lives in `SummonedUnit.h` and is TASK-574's.**

---

## 6. ITEM (5) — `SorcererUnit.cpp`: the 0-seal clause. ⭐ AND THE TRAP I AVOIDED.

**The stated mechanism was false in the most dangerous possible direction.** The old text said the seal works *because* `AcquireEnemyNearPoint(own castle, DefendRadius)` is handed a 0 radius. **That is no longer the call.** Since TASK-574 the value goes through `ResolveDefendEngagementRadius`, and **without that function's CONTRACT 1 early-return a bare `max(BoxExtent) + 0` would resolve to ≈3,657 uu at the 9× castle** — the seal would silently become a live target disc. ⇒ **A reader trusting the old comment would conclude "0 is inherently safe" and would be wrong.**

**Before**
```
	// DefendRadius 0 — the Shield Wall DEFEND stance targets via
	// AcquireEnemyNearPoint(own castle, DefendRadius); at 0 the disc is empty, so a sorcerer
	// under DEFEND falls back to marching home instead of picking a fight it cannot finish.
	DefendRadius = 0.f;
```
**After**
```
	// DefendRadius 0 — the Shield Wall DEFEND stance acquires enemies near the own castle.
	// ⚠️ THE MECHANISM CHANGED IN TASK-574 (CONVENTIONS WR-§2b row B) AND THE OUTCOME DID
	// NOT: DefendRadius is no longer a centre radius handed straight to
	// AcquireEnemyNearPoint — it is the BAND PAST THE CASTLE'S WALL FACE, and
	// ASummonedUnit::ResolveDefendEngagementRadius is the one place it is converted.
	// ⛔ THAT CONVERSION IS WHAT KEEPS THIS ZERO QUIET: its CONTRACT 1 tests the band <= 0
	// and returns 0 BEFORE any geometry is queried, so acquisition still admits nobody and
	// a sorcerer under DEFEND still marches home instead of picking a fight it cannot
	// finish. Without that early-out, a bare max(colliding half-extent) + 0 would resolve
	// to ≈3,657 uu at the 9× castle and hand a unit that cannot attack a real target disc.
	// (HISTORY — this note's original wording, quoted verbatim inside that contract:
	// "at 0 the disc is empty, so a sorcerer under DEFEND falls back to marching home".
	// The OUTCOME is unchanged; only the reason for it moved.)
	// ⛔ THE 0 STAYS — TASK-574 kept the seal deliberately. Do not retune it to match a
	// comment; the number is the seal and the comment is what had to catch up.
	DefendRadius = 0.f;
```

### ⭐⭐ THE ONE THING IN THIS TASK I WANT QA TO LOOK AT HARDEST — **MY OWN EDIT WOULD HAVE BROKEN A CITATION, AND THAT IS THIS TASK'S OWN FAILURE MODE**

⛔ **`SummonedUnit.cpp` CONTRACT 1 QUOTES THIS SENTENCE VERBATIM, IN QUOTATION MARKS, AS THE CONTRACT IT MUST KEEP TRUE:**
> `//    half 1 of its never-attacks seal ("at 0 the disc is empty, so a sorcerer under`
> `//    DEFEND falls back to marching home"), and that sentence must stay true after the`
> `//    semantic change.`

⚖️ **Had I simply rewritten the sentence, that quotation would have become a quote of text that exists nowhere — a dangling citation manufactured by the very sweep sent to kill one.** ⇒ ✅ **I preserved the original sentence VERBATIM inside an explicitly-labelled HISTORY clause.** The quote still resolves, `SummonedUnit.cpp` needed **no edit for this**, and my change set stayed minimal. **This is the TASK-577 "preserve Jonathan's wording as history" precedent applied for a mechanical reason rather than a courteous one.**

⚠️ **AND A WORDING DIVERGENCE I FOUND, RULED **NOT** A DEFECT, REPORTED RATHER THAN "FIXED":** `SummonedUnit.{h,cpp}` call `DefendRadius = 0` ***"half 1 of its never-attacks seal"***, while `SorcererUnit.cpp`'s own block comment calls the radius zeros ***"behavioral quieting … These are NOT the seal — CanEverAttack() is"***. ⛔ **I did not reconcile them and I did not import either framing into the other file.** They describe **different layers** and both are true: `CanEverAttack()` is the per-CLASS seal enforced at three guard points, and the radius zeros are what stop the legacy bodies from *acting* like they want a target. **Harmonising them would mean editing a framing in a file I was not sent to change, on taste rather than on falsity** — `SC-§15`'s own test for what is a departure and what is a preference. **Flagged for a manager ruling; it costs nothing to leave.**

---

## 7. ITEM (6) — 📌 `handoffs/TASK-278.md` §114, SECOND LIMB. ✅ **§114 IS NOW CLOSED IN FULL.**

**I read §114 at the artifact.** It is item 5 of TASK-278's *"For QA to scrutinize"*, and it names two limbs:
1. ✅ **`SiegePlayerState.cpp:134` / `:178`** — **discharged by TASK-577** (drifted to `:210`/`:262`; and 577 closed three more sites in that file than §114 knew about).
2. ⛔ **`SiegeBotController.cpp` deck-composition cost annotations (`cpp:236-269`, e.g. `// 3 x12 = 36`, `avg cost ~4.72/~7.02`) quoting pre-TASK-278 costs.** ⇒ **THIS TASK. Owed since 2026-07-24. Now closed.**

### The costs were DERIVED, not assumed — and then verified mechanically

⛔ **The spec explicitly refused to supply the figures, so I derived them from the shipped `Docs/Data/cards.csv` `Cost` column (read-only) and then checked every annotation with a script rather than by eye.** Corroboration: TASK-278's own handoff table gives the ×3 mapping, and `SiegeBotController.h`'s `AttackBankThreshold = 36` documents the same scaling from the other side.

```
$ python — parse cards.csv, parse every Entry(TEXT("<Card>"), <Count>) + its // annotation,
          assert csv.Cost == annotated cost, code Count == annotated count, subtotal == product
22 annotations checked   MISMATCHES: 0
deck[0]  sum(Count)=50   sum(cost*count)=708    avg=14.16
deck[1]  sum(Count)=50   sum(cost*count)=1053   avg=21.06
```
⭐ **Both averages are exactly 3× the retired figures (`4.72 × 3 = 14.16`, `7.02 × 3 = 21.06`)** — an independent confirmation that the old numbers were the pre-triple set and that nothing else drifted.

**Deck [0] AGGRO RUSH — before → after** (`avg cost ~4.72` → `avg cost 14.16 = 708/50`; `Sum 50.` preserved)
```
-		Entry(TEXT("Footman"),    12), // 3 x12 = 36            +		Entry(TEXT("Footman"),    12), //  9 x12 = 108
-		Entry(TEXT("MilitiaMob"),  6), // 5 x6  = 30 (swarm…)    +		Entry(TEXT("MilitiaMob"),  6), // 15 x6  = 90 (swarm…)
-		Entry(TEXT("Pikeman"),     6), // 5 x6  = 30             +		Entry(TEXT("Pikeman"),     6), // 15 x6  = 90
-		Entry(TEXT("Knight"),      6), // 6 x6  = 36             +		Entry(TEXT("Knight"),      6), // 18 x6  = 108
-		Entry(TEXT("Archer"),      6), // 4 x6  = 24             +		Entry(TEXT("Archer"),      6), // 12 x6  = 72
-		Entry(TEXT("Cavalry"),     4), // 7 x4  = 28 (charge)    +		Entry(TEXT("Cavalry"),     4), // 21 x4  = 84 (charge)
-		Entry(TEXT("Sapper"),      4), // 5 x4  = 20 (siege…)    +		Entry(TEXT("Sapper"),      4), // 15 x4  = 60 (siege…)
-		Entry(TEXT("Wall"),        4), // 4 x4  = 16             +		Entry(TEXT("Wall"),        4), // 12 x4  = 48
-		Entry(TEXT("Miner"),       2), // 8 x2  = 16 (minimal…)  +		Entry(TEXT("Miner"),       2), // 24 x2  = 48 (minimal…)
```
**Deck [1] DEFENSIVE ECONOMY — before → after** (`avg cost ~7.02` → `avg cost 21.06 = 1053/50`; `Sum 50.` preserved)
```
-		Wall           8  // 4 x8  = 32    +  // 12 x8 = 96          -		Cleric        3  // 6 x3  = 18   +  // 18 x3 = 54
-		ArrowTower     8  // 5 x8  = 40    +  // 15 x8 = 120         -		Ogre          2  // 12 x2 = 24   +  // 36 x2 = 72
-		Knight         6  // 6 x6  = 36    +  // 18 x6 = 108         -		DeepMine      2  // 15 x2 = 30   +  // 45 x2 = 90
-		BombTower      4  // 8 x4  = 32    +  // 24 x4 = 96          -		Lightning     2  // 8 x2  = 16   +  // 24 x2 = 48
-		BallistaTower  4  // 7 x4  = 28    +  // 21 x4 = 84          -		Longbowman    1  // 6 x1  = 6    +  // 18 x1 = 18
-		Miner          4  // 8 x4  = 32    +  // 24 x4 = 96
-		Barracks       3  // 10 x3 = 30    +  // 30 x3 = 90
-		CrystalTower   3  // 9 x3  = 27    +  // 27 x3 = 81
```
⛔ **Every `Count` argument is byte-identical (proof 1b). Only the trailing comment moved.**

**One new note block was added above deck [0]** recording *why* these rotted and *why nothing broke* — the annotations are documentation only; **legality is `sum(Count) == 50` and `Count <= MaxCopies` (COUNTS, not costs)** and the logged average comes from `UDeckLibrary::GetDeckAverageCost` reading DT_Cards **live** (`CostSum += Row->Cost * Entry.Count` / `TotalCount()` — traced, not assumed). ⚖️ **It is the same self-deriving-vs-transcribed lesson as `Projectile`, in the economy lane instead of the geometry lane: nothing read these numbers, which is exactly why they rotted unnoticed for three weeks.**

⇒ ✅ **`handoffs/TASK-278.md` §114 IS NOW DISCHARGED IN FULL.** Limb 1 by TASK-577, limb 2 here. ⛔ **Do not re-board it.**

---

## 8. ⛔ THE `SC-§22` SWEEP — command, raw hit count, one line per hit

⛔ **Every named site was treated as a LOWER BOUND.** Legs run **post-edit** against the whole of `Source/` (not just my ten files) so the counts are auditable. **String literals are covered explicitly (leg E) because `SC-§22` names runtime log strings as the PRIORITY surface.**

| leg | command (repo root) | raw hits | result |
|---|---|---|---|
| **PHANTOM** | `grep -rn "ResolveHeroStart" Source/` | **6 → 0** | ✅ **THE POST-CONDITION. Six found by my own grep, six fixed, zero remain.** |
| **PHANTOM-wide** | same, `--include=*.cpp,*.h,*.cs,*.py,*.ini,*.csv` over the whole repo | **0** | ✅ Nothing outside `Source/` either. Pipeline prose only — reported in §0, ⛔ not mine. |
| **A** | `grep -rniE "800 ?x ?800" Source/` | **6** | ✅ **ZERO live stale claims.** 2 are my new `Projectile` history clauses; 3 are TASK-574's `SummonedUnit` history clauses; 1 is TASK-577's `HeroCharacter` history clause. **Every survivor is explicitly labelled as retired.** |
| **A2** | `grep -rniE "(~\|≈)? ?(800\|810\|814)[- ]?(uu\|unit\|x)" Source/` | **17** | ✅ all accounted: history clauses (mine, 574's, 577's), `SiegeGameMode`'s past-tense rot record (573), `BattlefieldScatter`'s (576), `Castle.h`'s live 1800-uu opening figure (correct, `WR-§1`), one unrelated `800x1200` panel case in `Tests/SiegeWarMapTest.cpp` (564's — ⛔ not mine, ⛔ not a castle claim). |
| **B** | `grep -rnE "21,000\|21000" Source/` | **5** | ✅ **ZERO live stale claims.** 1 is my corrected `Castle.h` ruling citation; 4 are `ScatterConfig.h` / `BattlefieldScatter.cpp` retired-value records (TASK-576's, deliberate). |
| **B2** | `grep -rnE "22,?540" Source/` | **7** | ✅ all are the retired-edge lineage, every one labelled; `Castle.h`'s own `22,540 → 17,620` sentence is the TASK-557 measurement and is **correct**. |
| **C** | `grep -rniE "within DefendRadius\|disc is empty" Source/` | **5 → 2** | ✅ `within DefendRadius` is **GONE** (it was `UnitCommand.h` only). The 2 `disc is empty` survivors are **my preserved-verbatim history quote** and **TASK-574's quotation of it** — ⭐ preserved on purpose so the quotation still resolves (§6). |
| **C2** | `grep -rniE "defend.{0,40}disc\|disc.{0,40}defend" Source/` | **5** | ⛔ **4 RULED CORRECT AND DELIBERATELY NOT EDITED — see the table below.** |
| **D** | `grep -rnE "4\.72\|7\.02" Source/` | **2 → 1** | ✅ The one survivor is **my own new history clause** naming them as the PRE-triple values. |
| **D2** | deck annotations parsed + arithmetic re-derived against `cards.csv` | **22** | ✅ **0 mismatches.** Sums 50/50; averages 14.16 / 21.06. |
| **D3** | `MaxCopies` claims in the `SiegeBotController.cpp` legality comment, checked against `cards.csv` | **19** | ✅⭐ **RESULT: every one already correct.** ⛔ **Nothing edited** — they are COUNTS, untouched by the ×3 cost triple. **A sweep that finds nothing is a result** (`SC-§22`'s closing rule). |
| **E** | `grep -nE 'TEXT\(' <10 files> \| grep -iE '800x800\|~800\|~810\|~814\|21,?000\|22,?540\|4\.72\|7\.02\|ResolveHeroStart\|within DefendRadius\|disc is empty\|x12 = 36\|avg cost'` | **1 of 216** | ✅⭐ **RESULT: not one runtime string literal in the file set asserts a stale claim.** The single hit is `SiegeBotController.cpp`'s deck-select log, whose `avg cost %.2f` is fed by `GetDeckAverageCost` reading DT_Cards **LIVE** ⇒ it has always printed the CURRENT average. ⚖️ **The runtime log was the one surface that never lied, while the comment beside it lied for three weeks.** |

### ⛔ RULED CORRECT AND DELIBERATELY NOT EDITED — **editing any of these would INTRODUCE a falsehood** (the TASK-554 trap, re-verified independently here)

| site | text | why it is CORRECT |
|---|---|---|
| `SummonedUnit.cpp` (`UpdateStateStandardCommanded`, DEFEND branch) | *"fight only enemies within the defend disc of the OWN castle"* | ✅ **Still true.** The acquisition shape IS a disc; only its RADIUS became derived, and the very next comment says so in terms. ⛔ Not stale. |
| `SummonedUnit.h` (`UpdateStateStandardCommanded` doc) | *"DEFEND — target = AcquireEnemyNearPoint(own castle centre, ResolveDefendEngagementRadius(…))"* | ✅ **Matches the shipped call exactly**, and already carries the *"DefendRadius is the band PAST THE WALL FACE"* warning. ⛔ Not stale. |
| `SummonedUnit.h` (`bLoggedDefendBandDerived` doc) | *"the first time this unit resolves a DEFEND disc"* | ✅ Correct — and it is TASK-569's named acceptance instrument. ⛔ Not stale. |
| `SiegeBotController.cpp` (`ResolveCardChoice` doc) | *"an Ogre needs 36 gold post-TASK-278 ×3"* | ✅ **Verified live: `cards.csv` Ogre `Cost = 36`.** ⛔ Not a finding — TASK-278 already fixed this one. |
| `SiegeBotController.h` (`AttackBankThreshold` doc) | *"waves still grow toward Knight 18 / Cavalry 21 / Ogre 36"* | ✅ **All three verified against `cards.csv`.** Correct and current. |
| `SiegeBotController.cpp` (deck legality comment) | the 19 `MaxCopies` figures | ✅ **All 19 verified against `cards.csv` mechanically.** ⚠️ **This is the leg most likely to be mis-"fixed" by a keyword sweep** — they sit two lines above 22 annotations that WERE stale, and they are counts, not costs. |
| `Castle.h` | *"the 1800-uu clear opening"* | ✅ `WR-§1`'s own current figure (600 × 3). ⛔ Not stale. |
| `SiegeBotController.h` / `.cpp` | *"per 2 s tick"* (bot decision cadence) | ✅ **`DecisionIntervalSeconds = 2.f`. TASK-554 ruled this exact pair correct and TASK-577 re-confirmed it; I honour the ruling and did not touch it.** ⚠️ The standing false positive of this batch. |

---

## 9. Scope ledger — the sixteen edits, in full, and nothing else

| # | file | located by symbol | spec item | change | UHT? |
|---|---|---|---|---|---|
| 1 | `SiegeBotController.h` | `BotCastleSpawnOffset` doc | (1) | phantom → `GetHeroStartTransform` (+re-wrap) | ⚠️ **yes** (property ToolTip) |
| 2 | `SiegeBotController.h` | `ResolveCastleFaceDistance` doc | (1) | phantom → `GetHeroStartTransform` (+re-wrap) | no (private method) |
| 3 | `SiegeBotController.cpp` | `ResolveCastleFaceDistance` body | (1) | phantom → `GetHeroStartTransform` (+re-wrap) | no (`.cpp`) |
| 4 | `SummonedUnit.h` | `ResolveDefendEngagementRadius` doc | (1) | phantom → `GetHeroStartTransform` | no (private method) |
| 5 | `SummonedUnit.cpp` | `ResolveDefendEngagementRadius`, CONTRACT 2 | (1) | phantom → `GetHeroStartTransform` | no (`.cpp`) |
| 6 | `BattlefieldScatter.cpp` | the castle-face-distance derivation | (1) | phantom → `GetHeroStartTransform` | no (`.cpp`) |
| 7 | `Projectile.h` | `AProjectile` class doc block | (2) | `~800x800` → generation-labelled lineage + the self-deriving MECHANISM | ⚠️ **yes** (class ToolTip) |
| 8 | `Projectile.cpp` | `Tick`, the reach test | (2) | same, pointed at the header | no (`.cpp`) |
| 9 | `Castle.h` | `SpawnBoxHalfExtent` ledger note | (3) | *"left at its law value and FLAGGED"* → **ruling W2-R1, 21,000 → 16,080, RULED not flagged** | ⚠️ **yes** (property ToolTip) |
| 10 | `UnitCommand.h` | `ESiegeUnitCommand` enum doc | (4) | *"within DefendRadius"* → **the band-past-the-wall-face mechanism + the one supported reader** | ⚠️ **yes** (enum ToolTip) |
| 11 | `SorcererUnit.cpp` | constructor, `DefendRadius = 0.f` | (5) | dead disc mechanism → **CONTRACT 1's early-return**; original sentence preserved verbatim as history | no (`.cpp`) |
| 12 | `SiegeBotController.cpp` | above deck `[0]` | (6) | **new §114 note**: why they rotted, why nothing broke, what is authoritative | no (`.cpp`) |
| 13 | `SiegeBotController.cpp` | deck `[0]` header line | (6) | `avg cost ~4.72` → `14.16 = 708/50` | no (`.cpp`) |
| 14 | `SiegeBotController.cpp` | deck `[0]` entries | (6) | **9 trailing annotations** re-derived | no (`.cpp`) |
| 15 | `SiegeBotController.cpp` | deck `[1]` header line | (6) | `avg cost ~7.02` → `21.06 = 1053/50` | no (`.cpp`) |
| 16 | `SiegeBotController.cpp` | deck `[1]` entries | (6) | **13 trailing annotations** re-derived | no (`.cpp`) |

⛔ **No code statement, expression, default value, `UPROPERTY` specifier, `meta =` clause, signature, parameter or `#include` was added, removed or modified anywhere in this task — proven by sha256 in §1, not asserted.**
⛔ **`SC-§33`:** no trailing defaulted parameter was added, so no call-site enumeration is owed. **No function signature exists in this diff at all.**

---

## 10. What QA should scrutinise (TASK-565, criterion 19)

1. ⛔⭐ **RE-RUN THE POST-CONDITION AND PASTE YOUR OWN NUMBER:** `grep -rn "ResolveHeroStart" Source/` **must return 0.** ⚠️ It has already moved once (1 → 6) while a task was mid-flight, so run it **at your review time**, not from my table.
2. ⛔ **RE-RUN BOTH PROOFS IN §1 YOURSELF.** ⚠️ **And note the baseline difference before you do:** six of my ten files are dirty from 557/574/575/576, so **a `HEAD` comparison on those six will show THEIR code diffs, not mine, and is NOT a finding about this task.** ✅ **The four clean files (`Projectile.{h,cpp}`, `UnitCommand.h`, `SorcererUnit.cpp`) DO carry the full `HEAD` proof (§Proof 2b)** — those are the ones to hash against `HEAD`. For the other six, the honest re-run is `git stash`-free: strip comments from `HEAD` and from working, confirm the ONLY differences are the other tasks' code lines, none of which appear in my diff.
3. ⚠️ **Rule EXPLICITLY on the FOUR `.gen.cpp` tooltip regenerations** (§1) so criterion (9) has a *recorded* answer rather than a silent one. **TASK-554 and TASK-577 each declared one and both were ratified; I am asking for the same treatment for four, not assuming it.** ✅ **`git status` is clean of `.gen.cpp` — the STOP condition is not tripped.**
4. ⭐⭐ **The `SorcererUnit` ↔ `SummonedUnit` VERBATIM QUOTATION (§6) is the finding most worth an independent check.** Confirm that `SummonedUnit.cpp` CONTRACT 1's quoted sentence still appears **character-for-character** in `SorcererUnit.cpp`. **If you would rather I had updated both, say so and I will** — but that edits a second file for a quotation, and preserving it cost nothing.
5. ⚠️ **Rule on the `WR-§2 row 7` citation I DROPPED from `Castle.h` (§4).** It is the only place I removed a law reference rather than re-pointing it. My reasoning is that W2-R1 overruled the reasoning it carried; **if you prefer it kept as history, that is a one-line re-add.**
6. ⛔ **Re-derive the deck costs yourself from `Docs/Data/cards.csv`** — 22 annotations, two sums of 50, averages 14.16 and 21.06. ⚠️ **Check the `Count` arguments did not move** (proof 1b says they did not; they are what deck legality actually reads).
7. ⛔ **Re-verify the eight DELIBERATELY-LEFT correct claims in §8.** ⚖️ **A "fix" to any of them would be a regression no compiler and no test could see** — especially the 19 `MaxCopies` figures, which sit two lines above annotations that WERE stale, and the *"per 2 s tick"* pair that has now been ruled correct three times.
8. ⚠️ **Rule on the wording divergence in §6** (*"half 1 of the seal"* vs *"NOT the seal — CanEverAttack() is"*). **I left it deliberately and named the reason; it needs a manager/QA decision, not a silent harmonisation.**
9. ✅ **The legs that found nothing are RESULTS, not gaps** (`SC-§22`'s closing rule): **zero stale claims in any of the 216 string literals in the file set** (leg E), **zero surviving live `800x800` / `21,000` / `within DefendRadius` / `4.72` / `7.02` claims**, **zero `MaxCopies` errors in 19**, and **zero phantom hits repo-wide in code**.
10. 📌 **The forward-dated 9× figures in `Projectile.{h,cpp}` follow TASK-577's ratified generation-labelling convention** (`~814x814` at M1 / `~2438x2462` after Castle-3× / `~7314x7384` at 9×) precisely because TASK-566 has not imported the mesh yet. **Consistent with `Castle.h`, `SiegeBotController.h`, `SiegePlayerController.h` and `HeroCharacter.cpp`; correct before AND after TASK-566.**

## 11. No CONVENTIONS change

⛔ **No new asset, class, or identifier was introduced.** ⚠️ **`WR-§2b` row G's amended table and its one-time drive-by exception are DISCHARGED by this task** — every one of its six rows is closed. 📌 **The exception expires at TASK-570's commit, as written; nothing here extends it.**
