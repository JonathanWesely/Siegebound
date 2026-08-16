# TASK-577 handoff — [WR-23] comment-only hygiene in the un-owned files (gameplay-programmer, 2026-08-15)

- **Gate:** `.claude/pipeline/qa/TASK-565.md` — ⛔ **this task names TASK-565 as its gate.**
- **Compile:** TASK-566. **Commit:** TASK-570. ⛔ **This task opened NO compile of its own.**
- **Status:** `ready-for-qa`.
- **Discipline honoured:** ⛔ no compile · no editor · no MCP · no PIE · no `Content/` · no `.csv` · no `Tests/` · no `Build.cs` · no `.uasset`. ⛔ **No token figure is quoted, derived or reasoned from anywhere in this handoff** (batch-wide ban).
- ⚠️ **Git, declared:** the spec's item (4) *requires* a comment-filtered diff, which cannot be produced without reading Git. I ran **`git diff` / `git status` / `git show` — READ-ONLY, three commands, no mutation of any kind.** ⛔ No `add`, no `commit`, no `branch`, no `checkout`, no `push`. Nothing was staged. **TASK-570 remains the only commit.**
- **Files touched — exactly the three in `names:`:**
  - `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerState.cpp`
  - `Source/GitClaudeUnrealTest/Siegebound/CaptureZone.h`
  - `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp`
- ⛔ **NOT touched:** `SiegePlayerState.h` (the reference, read-only) · `CaptureZone.cpp` · `HeroCharacter.h` · `ZoneHalfExtent` (the CONSTANT) · `SiegeGameMode.{h,cpp}` (573) · `SummonedUnit.{h,cpp}` (574) · `SiegeBotController.{h,cpp}` (575) · `SiegePlayerController.{h,cpp}` (563) · `ScatterConfig.h` / `BattlefieldScatter.{h,cpp}` (576) · `Projectile.{h,cpp}` (⭐ **see §5 — a real finding, deliberately NOT edited**) · any `Content/`, `.csv`, `Tests/`.

---

## 0. ⛔ THE PROOF OBLIGATION FIRST, BECAUSE IT IS THE DELIVERABLE (spec item 4)

⚖️ *"An assertion that only comments changed is not evidence; the filtered diff is."* — **two independent proofs, the second cryptographic.**

### Proof 1 — the comment-filtered diff is EMPTY

```
$ F="…/SiegePlayerState.cpp …/CaptureZone.h …/HeroCharacter.cpp"

$ git diff -U0 -- $F | grep -E '^[+-]' | grep -vE '^(\+\+\+|---)' | wc -l
68                                    # every +/- payload line in the whole change

$ git diff -U0 -- $F | grep -E '^[+-]' | grep -vE '^(\+\+\+|---)' \
    | sed -E 's/^[+-][[:space:]]*//' | grep -vE '^(//|\*|/\*|\*/)' | sed -E '/^$/d' | wc -l
0                                     # …of which ZERO are anything but comment text
```

⇒ **All 68 changed lines are pure comment lines.** ⭐ **The filter is on the WHOLE line, not a prefix test** — a line like `int32 X = 1; // note` would fail it and appear in the second count. It is empty, so **no changed line carries a code token at all.**

### Proof 2 — the comment-STRIPPED code stream is byte-identical to `HEAD` (sha256)

`/* … */` blocks and `// …`-to-EOL removed from both versions, blank lines dropped, hashed:

| file | `HEAD` code-only sha256 | working code-only sha256 | verdict |
|---|---|---|---|
| `SiegePlayerState.cpp` | `0ac8195b…3c281f1e` | `0ac8195b…3c281f1e` | ✅ **IDENTICAL** |
| `CaptureZone.h` | `53553604…5814fbd8` | `53553604…5814fbd8` | ✅ **IDENTICAL** |
| `HeroCharacter.cpp` | `c8fb18d8…984cdc00` | `c8fb18d8…984cdc00` | ✅ **IDENTICAL** |

⛔ **No statement, expression, default value, `UPROPERTY` specifier, `meta =` clause, signature, `#include`, or whitespace on any live line was added, removed or modified.** ⛔ **No `TEXT()` literal was touched in any of the three files** (leg E below: raw count **0**).

📌 `git status --porcelain` reconciled: my three files are dirty; **every other dirty path belongs to 555 / 557 / 562 / 563 / 573 / 574 / 575 / 576 or is an untracked handoff.** No stray edit.

### ⚠️ DECLARED EXPLICITLY, NOT SILENTLY — the UHT metadata consequence (the TASK-554 precedent)

**TWO of my eight edits are `/** */` doc blocks UHT reads, both in `CaptureZone.h`:** the **class doc block above `UCLASS()`** and the **`ZoneHalfExtent` doc block above its `UPROPERTY`**. ⇒ **On TASK-566's compile, `Intermediate/…/UHT/CaptureZone.gen.cpp` WILL regenerate with new `Comment` / `ToolTip` metadata strings.**

⚖️ **That is EDITOR-ONLY metadata — a Details-panel tooltip and a class tooltip.** ⛔ **No gameplay byte, no behaviour, no replicated property, no prompt character.** The `UCLASS()` line, the `UPROPERTY(EditDefaultsOnly, Category = "Siegebound|Capture")` line and the `= FVector2D(840.f, 840.f)` default are **byte-identical** (proof 2). `Intermediate/` is build output, not source, and is not in this diff.

**The other six edits produce ZERO metadata change:** `SiegePlayerState.cpp` (5) and `HeroCharacter.cpp` (1) are `//` comments **inside function bodies in `.cpp` files** — UHT does not parse `.cpp` for metadata at all.

📌 **M8 DECLARATION (spec item 6):** ⛔ **no replicated property, no new replicated class, no new relevancy tier, no RPC, nothing emitted.** ✅ **And the reason is structural, not a promise: proof 2 shows the code stream is byte-identical to `HEAD`, so there is no mechanism by which anything could replicate differently.**

---

## 1. `SiegePlayerState.cpp` — the self-contradiction (spec item 1). ⚠️ **FIVE SITES, NOT THREE.**

**Verified at the reference before typing (`SC-§20`):** `SiegePlayerState.h:320` → `int32 BaseIncomeTickPeriod = 1;` ✅ and the header's own corrected prose at `:175-184` (*"with the 2026-07-24 defaults (BaseIncomeTickPeriod=1, TASK-278) the average is EXACT … so the round-up is a no-op … and +2/s in overtime"*). ⛔ **`SiegePlayerState.h` was NOT edited — it is the yardstick, and the `.cpp` was corrected TOWARD it.**

**`SC-§18c` — all three cited line numbers were RE-VERIFIED, and all three were still accurate.** ⛔ **NO PHANTOM in this task's citations; nothing was hunted for and nothing was manufactured.**

| spec citation | found at | verbatim | verdict |
|---|---|---|---|
| `:210` | **`:210` exact** | *"(default 2 ⇒ 1 gold per 2 s, §3.2 amended)"* | ✅ real, stale |
| `:262` | **`:262` exact** | *"GoldPerTick (1) per BaseIncomeTickPeriod (2) ticks"* | ✅ real, stale |
| `:270` | **`:270` exact** | *"defaults show +1/s pre-overtime (true 0.5/s …) and an exact +1/s in overtime"* | ✅ real, stale **twice** — see 1C |

### 1A — `HandleGoldTick`, cited (`:207-212` → `:207-214`, +2 lines)

**Before**
```
	// BaseIncomeTickPeriod-th tick (default 2 ⇒ 1 gold per 2 s, §3.2 amended).
	// This function deliberately does NOT call GetGoldRate() anymore: that is
	// now the HUD's rounded-up DISPLAY average, not the exact accrual.
```
**After**
```
	// BaseIncomeTickPeriod-th tick (default 1 ⇒ 1 gold per 1 s, §3.2 amended —
	// TASK-278 2026-07-24 reverted TASK-089's period 2 / 1-per-2-s rate; the
	// header's BaseIncomeTickPeriod default is the record of truth, not this
	// comment). This function deliberately does NOT call GetGoldRate() anymore:
	// that is now the HUD's rounded-up DISPLAY average, not the exact accrual.
```
⭐ The TASK-089 lineage is preserved **as history**, and the comment now **points at the field that owns the fact** instead of re-asserting a number that can rot again. **Cited by SYMBOL, not by line** (`SC-§18c`) — deliberately, since the last citation of this exact fact (`handoffs/TASK-278.md` §114's `:134`/`:178`) had already rotted to `:210`/`:262`.

### 1B — `GetGoldRate` header, cited (`:259-264` → `:261-266`, **Δ0 lines**)

**Before**
```
	// GoldPerTick (1) per BaseIncomeTickPeriod (2) ticks, doubled at 7:00 —
	// the shared overtime latch is read LIVE so the display can never desync
	// from the match clock.
```
**After**
```
	// GoldPerTick (1) per BaseIncomeTickPeriod (1 since TASK-278 2026-07-24;
	// was 2) ticks, doubled at 7:00 — the shared overtime latch is read LIVE
	// so the display can never desync from the match clock.
```

### 1C — the round-up justification, cited — ⚠️ **the cited line was false TWICE, and the spec named only one of them** (`:269-273` → `:271-279`)

**Before**
```
	// Base averaged over the grant period and rounded UP for display: defaults
	// show +1/s pre-overtime (true 0.5/s — ruled acceptable, "+0/s" over a
	// visibly rising counter reads as broken) and an exact +1/s in overtime.
	// Round-up is STABLE (never alternates), so RefreshGoldRate change
	// detection is unaffected. BaseIncomeTickPeriod is ClampMin 1 — no /0.
```
**After**
```
	// Base averaged over the grant period and rounded UP for display: with the
	// TASK-278 2026-07-24 defaults (period 1) the average is EXACT and the
	// round-up is a NO-OP — a truthful +1/s pre-overtime, +2/s in overtime. It
	// only bites again if the period is editor-tuned above 1. (HISTORY: at
	// TASK-089's period 2 the true base was 0.5/s and the round-up displayed
	// +1/s over it — ruled acceptable because "+0/s" over a visibly rising
	// counter reads as broken. Since TASK-278 the accrual matches the display.)
	// Round-up is STABLE (never alternates), so RefreshGoldRate change
	// detection is unaffected. BaseIncomeTickPeriod is ClampMin 1 — no /0.
```
⚠️ **The spec named the `true 0.5/s` half. The SAME SENTENCE also asserted *"an exact +1/s in overtime"*, which is equally false now** — traced at the code, not assumed: `EffectiveBase = GoldPerTick(1) × OvertimeIncomeMultiplier(2) = 2`, then `DivideAndRoundUp(2, 1) = 2` ⇒ **+2/s in overtime**, exactly as `SiegePlayerState.h:177-178` already says. ⭐ **`SC-§22` applied INSIDE a cited line, not only across files.**
⭐ **And the round-up is described as DORMANT, not dead** — `FMath::DivideAndRoundUp` is still called and still bites if `BaseIncomeTickPeriod` is editor-tuned above 1 (it is `EditDefaultsOnly`, `ClampMin = 1`). ⛔ **Writing "the round-up was removed" would have been a NEW falsehood** — the `SC-§20`/TASK-554 trap, avoided deliberately.

### ⭐ 1D — **UNCITED. `ResetEconomy`'s reset-path note** (`:500-504` → `:507-511`, +1)

**Before**
```
	// re-derives against a cleared overtime latch and lands on the pre-overtime
	// base display value (+1/s round-up with defaults, TASK-089).
```
**After**
```
	// re-derives against a cleared overtime latch and lands on the pre-overtime
	// base display value (+1/s with the TASK-278 defaults, and it is EXACT —
	// the round-up TASK-089 relied on here is a no-op at period 1).
```
⚠️ **The VALUE (+1/s) was still right; the MECHANISM attributed to it was dead.** `SC-§22`'s hardest shape verbatim — *"a true conclusion resting on a dead mechanism reads as verified and is not."*

### ⭐⭐ 1E — **UNCITED, AND IT IS THE WORST LINE IN THE FILE: `HandleOvertimeStarted` asserted the OPPOSITE OF THE SHIPPED BEHAVIOUR** (`:511-516` → `:518-524`, +3)

**Before**
```
	// DISPLAY change. NOTE (TASK-089): with default tuning the rounded display
	// base is 1 on both sides of the flip, so change detection correctly
	// broadcasts nothing — the 7:00 signal is the overtime HUD indicator,
	// not the rate text (ruling 4).
```
**After**
```
	// DISPLAY change. With the TASK-278 2026-07-24 defaults (period 1) the
	// rounded display base moves 1 -> 2 across the flip, so change detection
	// DOES fire and the HUD's "+N/s" text updates at 7:00. (HISTORY: at
	// TASK-089's period 2 the display base was 1 on BOTH sides, so this
	// broadcast was correctly suppressed and the 7:00 signal was the overtime
	// HUD indicator alone — ruling 4. TASK-278 reverted the TUNING, not this
	// handler; the bForceBroadcast=false call is correct under either.)
```

⛔⛔ **Why this is the file's worst site and why it is a genuinely separate defect, traced at the code:**
1. **It is not a stale figure — it is a FALSE STATEMENT ABOUT WHAT THE CODE DOES.** `HandleOvertimeStarted` → `RefreshGoldRate(false)` → `NewRate = GetGoldRate()`. Pre-overtime `BaseRate = DivideAndRoundUp(1,1) = 1`; in overtime `DivideAndRoundUp(2,1) = 2`. `NewRate != CachedGoldRate` ⇒ ⭐ **it DOES broadcast.** The comment says *"broadcasts nothing."*
2. **Its CONCLUSION is inverted too** — *"the 7:00 signal is the overtime HUD indicator, not the rate text"*. Since TASK-278 the rate text **does** change at 7:00 (+1/s → +2/s). ⚠️ **A reader debugging a HUD rate flicker at the overtime flip would read this comment and eliminate the correct cause.**
3. ⚖️ **TASK-554's greps could not have found it** — it contains no `per 2 s`, no `0.5`, no `1-per-2` token; it encodes the period-2 fact **only as a consequence** (*"1 on both sides"*). ⇒ **Found by sweeping the CLAIM (the shipped economy defaults), not the wording** — which is precisely what `SC-§22` asks for and why the spec's *"lower bound, not the extent"* held again.
4. ✅ **The behaviour it now describes is an IMPROVEMENT, not a defect** — the HUD correctly showing +1/s → +2/s at 7:00 is better than the suppressed broadcast TASK-089's ruling 4 settled for. ⛔ **No code was changed and none should be.**

---

## 2. `CaptureZone.h` — the two dead origin stories (spec item 2)

⛔ **`ZoneHalfExtent` STAYS `FVector2D(840.f, 840.f)`** — byte-identical (proof 2). ✅ `WR-§2` row 7 honoured: **the COMMENT was fixed to the CONSTANT, never the reverse**, and the header now *forbids* the inverse repair in writing.

**Both claims traced before typing (`SC-§20`), and the spec's *"already false after CASTLE-3X"* is CONFIRMED with the arithmetic:**

| claim | true when | what killed it | today |
|---|---|---|---|
| *"SAME size as each side's spawn box"* | at authoring — **`SpawnBoxHalfExtent` was also `(840,840)`** | **TASK-349 (Castle 3×) re-derived it `840 → 2460`** | **7380** (TASK-557) ⇒ off by **8.79×** |
| *"= 2x the castle footprint"* | at M1 — `1680 / 814 = 2.06` ≈ 2× | **Castle-3× → `1680 / 2437.9 = 0.689`** | **9× → `1680 / 7313.7 = 0.2297`** ≈ **0.23×** (matches the spec) |

⭐ **The provenance of the first row is the strongest evidence in this task and it is first-hand, not relayed:** `Castle.cpp:1047-1048` records the box's own lineage — *"TASK-349 re-derived 840 → 2460; TASK-557 re-derived 2460 → 7380"*. **The pairing was REAL at authoring and was broken by a resize that did not carry the zone with it — exactly as `WR-§2` row 7 intended.** ⇒ **Both claims rotted a resize BEFORE this one, as the spec said.**

### 2A — class doc block (`:43-44` → `:43-49`, +6)

**Before**
```
 *  `CaptureZone_Center` sits at world origin (0,0,0); its box straddles X=0 and
 *  is the SAME size as each side's spawn box (ZoneHalfExtent default (840,840)).
```
**After**
```
 *  `CaptureZone_Center` sits at world origin (0,0,0); its box straddles X=0,
 *  ZoneHalfExtent default (840,840).
 *  ⚠️ THE ORIGINAL "SAME size as each side's spawn box" CLAIM IS DEAD, AND IT DIED
 *  ONE RESIZE AGO: it was true at authoring (SpawnBoxHalfExtent was (840,840) too),
 *  went false when TASK-349 (Castle 3×) re-derived that box 840 -> 2460, and is now
 *  off by ~8.8× at TASK-557's 7380. ⛔ The equality was DESCRIPTIVE, never a pairing
 *  law: CONVENTIONS WR-§2 row 7 ruled ZoneHalfExtent DELIBERATELY UNCHANGED, because
 *  scaling mid changes capture gameplay. ⛔ Do NOT "fix" the constant to the comment.
```

### 2B — `ZoneHalfExtent` doc block (`:166-173` → `:171-188`, +9) — **the load-bearing one**

**Before**
```
	 *  Half-extent (XY) of the capturable box, centered on the actor origin.
	 *  Default (840,840) = "same size as the spawnable region on either side"
	 *  (Jonathan) = 2x the castle footprint. FLAGGED tunable.
	 *  ⚠️ PAIRED TUNABLE with AAncientGround::ZoneHalfExtent (TASK-359): the
	 *  ancient grounds are specified as "the size of the mid capture zone", so
	 *  the two defaults are deliberately identical — change them TOGETHER.
```
**After**
```
	 *  Half-extent (XY) of the capturable box, centered on the actor origin.
	 *  Default (840,840). FLAGGED tunable.
	 *  ⚠️ ITS TWO ORIGIN STORIES ARE HISTORY, NOT LIVE RELATIONSHIPS, AND BOTH WENT
	 *  FALSE ONE RESIZE AGO. Jonathan's "same size as the spawnable region on either
	 *  side" held only while SpawnBoxHalfExtent was also (840,840) — it is 2460 after
	 *  TASK-349 (Castle 3×) and 7380 after TASK-557 (the 9× castle). And "= 2x the
	 *  castle footprint": the 1680 full width was ~2.1x the ~814-uu M1 castle
	 *  (CONVENTIONS WR-§2b row C), ~0.69x after Castle-3× (~2438 wide) and ~0.23x at
	 *  the 9× castle (~7314 wide). ⛔ Do NOT re-derive this default from either of
	 *  them: CONVENTIONS WR-§2 row 7 rules it DELIBERATELY UNCHANGED because scaling
	 *  mid changes capture gameplay — the same ruling Castle-3× made.
	 *  ⚠️ PAIRED TUNABLE with AAncientGround::ZoneHalfExtent (TASK-359): the
	 *  ancient grounds are specified as "the size of the mid capture zone", so
	 *  the two defaults are deliberately identical — change them TOGETHER.
	 *  ⭐ THAT pairing is live law; the two above are not.
```
⭐ **Why the closing star line earns its place:** this block now holds **three** cross-references — two dead, one live. ⚠️ **A reader who learns the first two were "descriptive, not law" is one inference away from dismissing the AAncientGround pairing too, which IS binding.** The line makes the distinction explicit instead of leaving it to be re-derived.
✅ **Jonathan's own wording is preserved verbatim in both blocks** — the origin is recorded as HISTORY, not deleted.
✅ **Corroborated independently:** `SiegePlayerController.h:1246` already states *"its 'same size as the spawn box' origin was descriptive, never a pairing law."* ⇒ **the module already held the correct ruling in the CONTROLLER while the ZONE'S OWN HEADER contradicted it.** That contradiction is now closed.

---

## 3. `HeroCharacter.cpp` — the rotted number on a self-deriving mechanism (spec item 3)

**Cited `:428` — found at `:428` exact.** ✅ No phantom. (`:427-431` → `:427-434`, +4)

**Before**
```
		// (the castle: ~800x800 footprint, origin at center) are reachable from their walls.
		// ECC_Pawn is blocked by both pawn capsules and default static mesh collision.
```
**After**
```
		// (the castle — ~814x814 at the M1 blockout, ~2438x2462 after Castle-3× and
		// ~7314x7384 at the 9× castle, origin at center) are reachable from their walls.
		// ⭐ Those figures are HISTORY, not a dependency: ActorGetDistanceToCollision
		// measures against the target's live COLLISION, so it follows the mesh by
		// itself. That is why this line survived both resizes untouched while the
		// old "~800x800" text rotted — the MECHANISM was never the stale part.
		// ECC_Pawn is blocked by both pawn capsules and default static mesh collision.
```
✅ **The spec's item-3 requirement discharged:** the mechanism claim is recorded, and it is **traced, not asserted** — `ActorGetDistanceToCollision(MyLocation, ECC_Pawn, ClosestPoint)` resolves against the actor's live collision components, so it auto-follows any mesh rescale. ⛔ **Nothing but the comment changed.**

### ⚠️ THE ONE JUDGEMENT CALL IN THIS TASK — RAISED, NOT BURIED (`SC-§15`)

⛔ **The 9× figure is FORWARD-DATED TODAY, and I chose the wording so that it is true either way.**
`handoffs/TASK-555-artist.md` states the mesh **source** is rescaled (`Content/RawAssets/Castle.fbx` overwritten, measured `7313.6 × 7384.4 × 8082.6`) but the **`.uasset` import is TASK-566 and has not run.** ⇒ **At this instant the loaded `SM_Castle` is still the Castle-3× mesh (~2438 × 2462).**
✅ **This is why every figure I wrote is LABELLED WITH ITS SCALE GENERATION** (*"~2438x2462 after Castle-3× and ~7314x7384 at the 9× castle"*) rather than phrased as *"the castle is X"*. **A generation-labelled lineage is true before AND after TASK-566, and it cannot rot at the next resize either** — the comment's actual claim is now *"these numbers are history and the code does not depend on them."*
📌 **Not a TASK-577 finding, but QA should know it is batch-wide:** `Castle.h:349`, `SiegeBotController.h:416` and `SiegePlayerController.h:1237` (TASK-557's) already assert the 9× bounds as present-tense fact, forward-dated the same way. **Consistent, and correct the moment TASK-566 lands.**
📌 **A ±0.1 uu discrepancy exists between the DERIVED figure (`7313.7 × 7384.5`, CONVENTIONS `WR-§0` + the three headers) and TASK-555's MEASURED readback (`7313.6 × 7384.4`).** I wrote **`~7314 x ~7384`**, which asserts neither. ⛔ **Reported, not reconciled — it is not mine and it is immaterial to a comment that says the number is history.**

---

## 4. ⛔ THE `SC-§22` SWEEP — COMMANDS, RAW HIT COUNTS, ONE LINE PER HIT

Scope: the three owned files (legs A–E, post-edit) plus a **report-only** module-wide scan (leg F). **String literals are included by construction — `grep` is content-based, not comment-aware — and leg E sweeps them explicitly, because `SC-§22` names runtime log strings as the PRIORITY surface.**

| leg | command (repo root) | raw hits | result |
|---|---|---|---|
| **A** | `grep -cEi "per 2 ?s\|1-per-2\|0\.5 ?/s\|\(2\) ticks\|default 2\|period 2\|every 2 s" SiegePlayerState.cpp` | **3** | ✅ **ALL THREE ARE MY OWN NEW, EXPLICITLY-LABELLED HISTORY** (`:211`, `:275`, `:523` — each inside a "TASK-089's period 2 …" clause). **Zero present-tense stale economy claims survive.** |
| **B** | `grep -cEi "round-up\|rounded" SiegePlayerState.cpp` | **7** | 6 are mine or correct; **1 deliberately left — see ruling below.** |
| **C** | `grep -cEi "spawn box\|spawnable region\|castle footprint\|2x the castle\|same size" CaptureZone.h` | **3** | ✅ **all three are my own new text, every one framed as DEAD/HISTORY** (`:45`, `:176`, `:179`). |
| **D** | `grep -cEi "800x800\|~800\|footprint" HeroCharacter.cpp` | **2** | `:433` = my own history clause; `:553` **ruled CORRECT — see below.** |
| **E** | `grep -nE 'TEXT\(' <3 files> \| grep -iE 'gold per\|/s"\|per 2\|0\.5\|castle\|footprint\|spawn box\|half-extent\|840\|800'` | **0** | ✅⭐ **RESULT: not one runtime string literal in any of the three files asserts ANY of these claims.** `CaptureZone.h` has exactly one `TEXT()` (`ZoneColorParamName`); `HeroCharacter.cpp`'s only near-hit was `"respawn"`; `SiegePlayerState.cpp`'s 25 literals are all authority-guard / clamp diagnostics naming **no rate at all**. |
| **G** | broad numeric-claim scan of all comments in `CaptureZone.h` + `HeroCharacter.cpp` | **17** | every one re-read; 2 ruled correct below, the rest are the constants themselves or my own new text. |
| **F** | module-wide scan for the SAME THREE CLAIM SHAPES outside my files | **13** | ⛔ **REPORT-ONLY, NOTHING EDITED — see §5.** |

### ⛔ RULED CORRECT AND DELIBERATELY NOT EDITED — **editing these would INTRODUCE a falsehood** (the TASK-554 trap, verified independently)

| site | text | why it is CORRECT |
|---|---|---|
| `SiegePlayerState.cpp:214` | *"the HUD's rounded-up DISPLAY average"* | ✅ **`FMath::DivideAndRoundUp` IS still called** (`:280`). **TASK-554 already ruled this exact line correct-narrowly; I re-verified and honour the ruling.** ⛔ Not a finding. |
| `HeroCharacter.cpp:553` | *"for the castle's huge footprint"* | ✅ **Carries no figure and gets MORE true at every scale-up.** ⛔ Not a finding. |
| `HeroCharacter.cpp:354` | *"one swing per MeleeCooldown seconds (GDD §3.1: 0.5 s)"* | ✅ **Verified live: `HeroCharacter.h:445` `float MeleeCooldown = 0.5f;`.** Correct and current. |
| `CaptureZone.h:56` / `:192` | *"every CaptureEvalInterval (0.5 s)"* / *"Default 0.5 s"* | ✅ **Verified live: `CaptureEvalInterval = 0.5f`.** Correct and current. |
| `SiegeBotController.cpp:521` · `SiegeBotController.h:661` | *"never per 2 s tick"* | ✅ **NOT an economy claim — the BOT DECISION CADENCE. Verified live: `SiegeBotController.h:221` `float DecisionIntervalSeconds = 2.f;`.** ⚠️ **This is the exact false positive TASK-554 flagged; a keyword sweep that "fixed" it would have introduced a falsehood.** ⛔ Not mine (TASK-575) and ⛔ **not a defect.** 📌 It has drifted `.h:568 → .h:661` since TASK-554 — a live demonstration of `SC-§18c`. |

---

## 5. ⚠️⭐⭐ THE FINDING THAT MATTERS MOST: **A FOURTH UN-OWNED FILE CARRIES THE IDENTICAL CLAIM AND `WR-§2b` ROW G DOES NOT LIST IT**

⛔ **NOT EDITED. NOT OPENED. REPORTED** (`SC-§15`), exactly as the spec's *"do not extend this task to any other file"* requires.

> **`Projectile.h:31` — *"Large targets (the castle's ~800x800 base)"***
> **`Projectile.cpp:217` — *"the castle's ~800x800 base impacts at its …"***

⚖️ **This is `HeroCharacter.cpp:428`'s twin, one for one:** same M1-era figure, same *"large target ⇒ measure against its bounds, not its origin"* reasoning family, same rot across the same two resizes.

⛔⛔ **AND IT IS UN-OWNED. I checked the board: every `Projectile` hit belongs to a `done` task (TASK-026 / 035 / 040 / 054 / 068). No WAR-ROOM task's `names:` block lists `Projectile.{h,cpp}`.** ⇒ ⭐ **`WR-§2b` row G's file list — the enumeration this very task was created from — is itself a LOWER BOUND, which is `SC-§22` proving itself on the artifact that cites `SC-§22`.**

⚖️ **Why this needs a decision rather than a shrug:** row G's rule is *"a stale comment is fixed BY THE TASK ALREADY IN THAT FILE; only the files NO task owns get a dedicated hygiene task."* **`Projectile.{h,cpp}` satisfies BOTH halves of that test and got neither.** ⇒ **Left un-boarded, it survives this batch and rots through the next resize.** 📌 **Recommend the manager either extend TASK-577's `names:` (I can land it in one edit) or board a sibling — it is comment-only, needs no editor, and rides TASK-570 exactly as this task does.**

### ✅ THE OTHER TEN LEG-F HITS, RULED IN FULL — every one is already handled or already correct

| site | verdict |
|---|---|
| `SummonedUnit.cpp:75` · `:3674` · `SummonedUnit.h:1497` | ✅ **ALREADY FIXED IN FLIGHT by TASK-574**, and its text explicitly records *"this line quoted the M1 castle's '~800x800'"* as history. **Corroborates the batch-wide pattern.** ⛔ Not mine. |
| `SiegeGameMode.cpp:788` · `SiegeGameMode.h:292` | ✅ **CORRECT AS WRITTEN** — *"the old flat 600 was sized for the M1 castle's ~810-uu footprint and ROTTED"* is a past-tense record of a rot already repaired. ⛔ Not mine (TASK-573). |
| `BattlefieldScatter.h:731` | ⚠️ **STALE** — *"≈1200 clears a ~810-unit castle footprint"*. ✅ **ALREADY OWNED: `WR-§2b` row D / TASK-576, whose spec quotes this exact sentence.** ⛔ Not mine; no action needed. |
| `Castle.h:425` · `:429` | ✅ **CORRECT** — the 1800-uu clear opening is `WR-§1`'s own current figure (600 × 3). ⛔ Not stale. |
| `SiegePlayerController.h:1246` | ✅ **ALREADY CORRECT** (TASK-563's file) and it **corroborates §2** — see above. |

---

## 6. 📌 `handoffs/TASK-278.md` §114 — ⚠️ **HALF-CLOSED, NOT CLOSED. SAY SO ON THE RECORD.**

The spec says boarding TASK-577 *"CLOSES that owed item — say so in the handoff so nobody re-boards it."* ⚠️ **I read §114 at the artifact and it has TWO limbs, not one:**

1. ✅ **CLOSED BY THIS TASK.** *"`SiegePlayerState.cpp:134` ('default 2 ⇒ 1 gold per 2 s') and `:178` ('GoldPerTick (1) per BaseIncomeTickPeriod (2) ticks')"* — both fixed above (drifted to `:210`/`:262`; `SC-§18c` again). ⭐ **And this task closed three MORE sites in that file than §114 knew about** (1C's second falsehood, 1D, 1E). ⛔ **Do not re-board this limb.**
2. ⛔ **STILL OPEN, AND IT IS NOT MINE.** §114's second limb: *"`SiegeBotController.cpp` deck-composition cost annotations (cpp:236-269, e.g. `// 3 x12 = 36`, `avg cost ~4.72/~7.02`) now cite old costs."* **Confirmed still present verbatim** (`:236`, `:241`, `:244`, `:252`, `:259` — one read-only `grep`, nothing opened or edited) and they still quote **pre-TASK-278 card costs** (`SiegeBotController.h`'s own `AttackBankThreshold = 36` documents the ×3 cost scaling that made them stale). ⇒ **`SiegeBotController.{h,cpp}` is TASK-575's file — NAMED TO TASK-575** (`SC-§34` clause (iii) shape).

⚖️ ⛔ **THEREFORE: §114 MUST NOT BE MARKED FULLY DISCHARGED.** ⭐ **An owed item recorded as closed while half of it is still live is exactly the failure §114 itself suffered — a real request that nobody re-read.**

---

## 7. What QA should scrutinise (TASK-565)

1. ⛔ **Re-run both proofs in §0 yourself and paste your own numbers** — the payload-line count (**68**), the non-comment count (**0**), and ideally the three code-only sha256 pairs. ⚖️ **The gate's half of the proof obligation is re-running it, not reading it.**
2. ⚠️ **Rule EXPLICITLY on the `CaptureZone.gen.cpp` tooltip regeneration** (§0), so criterion (9) — *"no emitted byte, no behaviour, no prompt character moved"* — has a **recorded** answer rather than a silent one. **TASK-554 declared the same class of consequence and it was ratified; I am asking for the same treatment, not assuming it.**
3. ⭐⭐ **The two UNCITED `SiegePlayerState.cpp` sites (1D, 1E) are the findings most worth an independent check** — especially **1E**, which asserted the OPPOSITE of the shipped behaviour. **Please re-trace it**: `HandleOvertimeStarted` → `RefreshGoldRate(false)` → `GetGoldRate()`, `DivideAndRoundUp(1,1)=1` vs `DivideAndRoundUp(2,1)=2`. **If you judge either outside this task's remit, say so and I will revert it** — but both are inside a declared file, both are the identical defect class, and `SC-§22`'s implementer's half is explicit that a one-line fix to a cited line is not done until the shape has been swept.
4. ⛔⛔ **RULE ON `Projectile.{h,cpp}` (§5).** This is the one item that **cannot** be settled inside this task. Either extend TASK-577's `names:` (one edit, I will take it) or board a sibling — **or record a decision to leave it, so the next resize does not rediscover it a third time.**
5. ⚠️ **Rule on §6.** ⛔ **Do not let `TASK-278` §114 be recorded as fully discharged** — its `SiegeBotController.cpp` limb is live and named to TASK-575.
6. ⚠️ **Check the forward-dated 9× figure judgement in §3** and rule on whether generation-labelled lineage was the right call given TASK-566 has not imported the mesh yet. ⛔ **If you want a present-tense figure instead, it must wait for TASK-566 — and it will rot again at the next resize, which is the argument for the labelling.**
7. ⛔ **Re-verify the four DELIBERATELY-LEFT correct claims in §4** — `DecisionIntervalSeconds = 2.f`, `MeleeCooldown = 0.5f`, `CaptureEvalInterval = 0.5f`, and `FMath::DivideAndRoundUp` still being called. ⚖️ **A "fix" to any of these would be a regression that no compiler and no test could see.**
8. ✅ **The legs that found nothing are RESULTS, not gaps** (`SC-§22`'s closing rule): **zero stale claims in any string literal in any of the three files** (leg E = 0), **zero present-tense stale economy claims left in `SiegePlayerState.cpp`**, and **no phantom citation** — all four cited line numbers existed exactly where the spec said.

## 8. Scope ledger — the eight edits, in full, and nothing else

| # | file | symbol located by | cited? | change | Δ lines |
|---|---|---|---|---|---|
| 1 | `SiegePlayerState.cpp` | `HandleGoldTick` | ✅ `:210` | `default 2 ⇒ 1 gold per 2 s` → `default 1 ⇒ 1 gold per 1 s`, TASK-089 kept as history, header named as record of truth | +2 |
| 2 | `SiegePlayerState.cpp` | `GetGoldRate` (header para) | ✅ `:262` | `BaseIncomeTickPeriod (2)` → `(1 since TASK-278; was 2)` | 0 |
| 3 | `SiegePlayerState.cpp` | `GetGoldRate` (`BaseRate` para) | ✅ `:270` | `true 0.5/s` **and** `exact +1/s in overtime` → EXACT `+1/s` / `+2/s`; round-up marked DORMANT not dead | +4 |
| 4 | `SiegePlayerState.cpp` | `ResetEconomy` reset-path note | ⭐ **no** | dead round-up attribution corrected; value `+1/s` kept (it was right) | +1 |
| 5 | `SiegePlayerState.cpp` | `HandleOvertimeStarted` | ⭐⭐ **no** | *"broadcasts nothing"* → **it DOES broadcast, 1 -> 2**; ruling 4 kept as history | +3 |
| 6 | `CaptureZone.h` | `ACaptureZone` class doc block | ✅ `:44` | *"SAME size as the spawn box"* marked DEAD with its lineage + the do-not-invert warning | +6 |
| 7 | `CaptureZone.h` | `ZoneHalfExtent` doc block | ✅ `:168-169` | both origin stories marked HISTORY with ratios; `WR-§2` row 7 cited; live pairing distinguished | +9 |
| 8 | `HeroCharacter.cpp` | `DoMeleeAttack` melee-range comment | ✅ `:428` | `~800x800` → generation-labelled lineage + the self-deriving MECHANISM recorded | +4 |

⛔ **No code statement, expression, default value, `UPROPERTY` specifier, `meta =` clause, signature, parameter or include was added, removed or modified anywhere in this task — proven by sha256 in §0, not asserted.**
