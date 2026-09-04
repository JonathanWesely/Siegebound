# QA Report — TASK-943 (gate over `TASK-942` ALONE)

**Verdict: PASS** — **0 BLOCKERS** · 4 WARN · 2 NIT · 2 ROUTED OBLIGATIONS
**Reviewer:** qa-reviewer · **Date:** 2026-09-03 · **Gates:** `TASK-942` **alone** (`SC-§29`)
**Mode:** ⛔ read-only · ⛔ no compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git (see INSTRUMENT LIMIT below)

---

## ⛔⛔ THE HEADLINE, FIRST

> ### ✅ **THE SPLIT IS REAL, NOT AN ALIAS. THE CEILING GENUINELY BINDS. THE NAV-LINK RE-ARM IS PRESENT AND FIRES ON THE UPGRADE PATH.**
> ### ⛔ **AND THE ONE THING THAT MUST NOT SHIP UNACCOMPANIED IS NOT IN THIS DIFF: `SiegeControlsHelpWidget.cpp` will teach a FALSE RULE on screen. See ROUTED-2.**

Every claim below was **re-measured by symbol** (`SC-§40` cl. 9), never relayed from the handoff.

---

## ⛔ INSTRUMENT LIMIT — DECLARED **ABOVE** THE FINDINGS (`SC-§39`, `SC-§55`, `SC-§56`)

⛔ **THE `Bash` TOOL IS DISABLED IN THIS SESSION.** ⇒ I could run **no `git` command of any kind** —
⛔ no `status`, ⛔ no `diff`, ⛔ no `show`. Consequences, stated so nobody reads more into a zero than it
carries:

1. ⛔ **Every "zero edits" finding below is a SOURCE-STATE census, ⛔ not a diff.** Where a diff was the
   only honest instrument I say so and name the corroborant I used instead.
2. ⛔ **I could ⛔ not re-run `git status`, so I cannot enumerate the five dirty `Source/` paths.** The
   concurrent-lane attribution below is done by **what a file CONTAINS** (`qa/TASK-958.md` §1's
   precedent), which is an instrument that does not need git.
3. ⛔ **`SC-§55`: my session-start `gitStatus` snapshot is STALE and is ⛔ NOT cited anywhere in this
   report.**
4. ⇒ ⛔ **`TASK-944` must derive its pathspec from its OWN `git status` and must ⛔ not adopt mine — I
   have none** (`SC-§40` cl. 1). ⚠️ **And `SC-§56`: a `-- <pathspec>` miss is SILENT. Use a positive
   control.**

---

## 1. ⛔⛔ ITEM (1) — TWO INDEPENDENT VIRTUALS, OR ONE VIRTUAL WITH TWO SPELLINGS? — **TWO. MEASURED.**

### THE CENSUS, RE-RUN BY ME, BY SYMBOL

| # | gate | file:line | `CanStackHeight(` | `CanScaleFootprint(` |
|---|---|---|---|---|
| 1 | `ResolvePlacementUpgradeState` gate (5) | `SiegePlayerController.cpp:5295` | **1** | **0** |
| 2 | `ConfirmStackUpgrade`'s re-ask | `SiegePlayerController.cpp:2507` | **1** | **0** |
| 3 | `ABuilding::ApplyStackUpgrade`'s guard | `Building.cpp:438` | **1** | **0** |

> ### ⛔ **STACK-SITE CONSULTS OF `CanScaleFootprint()`: `0`. CONFIRMED.**
> ### ⛔ **A FOURTH CONSULT (either predicate, any stack site): `0`. CONFIRMED.**
> ### ⛔ **SURVIVING WHEEL CONSULTS: EXACTLY `1`** — `ASiegePlayerController::CanCardActorScaleFootprint`
> (`SiegePlayerController.cpp:5455`, `return Defaults != nullptr && Defaults->CanScaleFootprint();`).

⚠️ **The discrimination that makes that `1` honest:** the other `…CanScaleFootprint` hits in the
controller (`:1773`, `:1833`, `:2873`) are the **boolean member `bPendingCardCanScaleFootprint`**, a
different symbol, and are correctly **not** counted as predicate consults. ⛔ I checked each by reading it,
not by counting a substring.

### ⭐⭐ THE ANTI-ALIAS PROOF — **THE DISAGREEMENT**, MEASURED AT SOURCE

- `Building.h:257` — `virtual bool CanScaleFootprint() const { return true; }`
- `Building.h:285` — `virtual bool CanStackHeight() const { return true; }`
- `ClimbableTower.h:516` — `virtual bool CanScaleFootprint() const override { return false; }`
- `ClimbableTower.h:547` — `virtual bool CanStackHeight() const override { return true; }`

⇒ ⛔ **Two virtuals, four literal bodies, and they DISAGREE on `AClimbableTower` (wheel `false` / stack
`true`).** ⛔ **`{ return CanScaleFootprint(); }` appears in ⛔ ZERO of the four bodies.** ⇒ **not a
wrapper, not an alias, not a delegation.** ✅ **Item (1) PASSES.**

---

## 2. ⛔⛔ ITEM (2) — THE WHEEL EXCLUSION SURVIVES — ✅ **PASS**

- `AClimbableTower::CanScaleFootprint()` is **still `false`** (`ClimbableTower.h:516`), ⛔ not deleted,
  ⛔ not flipped.
- The wheel **still consults it**: `CanCardActorScaleFootprint` (`:5455`) → `EnterPlacementMode`'s
  `bPendingCardCanScaleFootprint` (`:1773-1774`) → `ApplyPlacementFootprintWheel`'s leading guard
  (`:2873`). ⛔ **That chain is byte-consistent with the shipped wheel and nothing in it names the stack.**
- ⛔ **No `CardID == "WatchTower"` string compare anywhere on the placement path** (`STACK-§2`) — census
  across `Building.{h,cpp}`, `ClimbableTower.{h,cpp}`, `SiegePlayerController.{h,cpp}`: **0**.

---

## 3. ⛔⛔⭐ ITEM (3) — PROVENANCE OF THE CEILING — ✅ **PASS, TRACED TO `TASK-941`**

I opened `handoffs/TASK-941-programmer.md` as the row required. Its §(lines 15-16, 166-179) carry:

```
need OPEN <= JAM :  1200n − 2·HH  <=  1160n − HH   <=>   40n <= HH   <=>   n <= HH / 40
                    unit HH = 88  ->  n <= 2.2 (BINDING)      hero HH = 96  ->  n <= 2.4
```

`ClimbableTower.cpp:191-223` reproduces **that derivation, term for term**, and cites `STACK-§10`. The
board's number, `CONVENTIONS.md` `STACK-§10` cl. 1's table (`n=2` margin **+8.00**, `n=3` **−32.00**) and
the constructor comment **all agree and all descend from the same measurement.**

⇒ ⛔ **The ceiling has MEASURED provenance. It is ⛔ not a preference, ⛔ not read off this board, ⛔ not
read off `STACK-§8`'s outcome table.** ⛔ **I did not re-litigate the geometry** (out of scope, item (8)).

---

## 4. ⭐⭐ THE SUBTLE ONE — **DOES THE CAP ACTUALLY BIND?** — ✅ **ALL FOUR MEASURED, ALL FOUR HOLD**

`TASK-956` measured that `Building.cpp:302` read `GetDefault<ABuilding>()` — the **base** CDO — so a
per-class `= 2` would have been **read straight past** and the tower would have kept stacking to `5`.

| claim | measurement | verdict |
|---|---|---|
| the signature takes the cap | `Building.cpp:294` / `Building.h:207` — `static float StackHeightMultiplier(int32 UpgradeCount, int32 MaxMultiplier)`; ⛔ **neither parameter defaulted** (`SC-§33`); ⛔ **not a `UFUNCTION`** | ✅ |
| **ZERO `GetDefault<>` in the HEIGHT resolver** | I read `Building.cpp:294-339` line by line. ⛔ **No `GetDefault<`. No CDO. No world. No instance.** The cap arrives as `MaxMultiplier` and is clamped `FMath::Max(1, MaxMultiplier)` | ✅ |
| ⭐ **the HEALTH resolver STILL reads the CDO — the negative control** | `Building.cpp:350` — `const ABuilding* const Defaults = GetDefault<ABuilding>();` ⇒ ⛔ **the two resolvers GENUINELY DIFFER**, 56 lines apart in the same file | ✅ **This is the clever half and it is real** |
| the false comment is fixed | `Building.cpp:296-312` now states the old behaviour, why it was wrong and what it cost. `Building.h:436-440` rewrites the mirror claim and says the second half was *"TRUE BY ACCIDENT"* | ✅ |

### ⛔⛔ AND THE PART THAT MAKES THE CEILING **REAL AT THE CALL SITE** — I CHECKED EVERY CONSUMER

| consumer | what it passes | verdict |
|---|---|---|
| `Building.cpp:475` | `StackHeightMultiplier(StackUpgradeCount, MaxStackHeightMultiplier)` — ⭐ **this instance's own member**, ⛔ not a CDO | ✅ |
| `SiegePlayerController.cpp:2581-2582` | `const int32 TargetHeightCap = Target->GetMaxStackHeightMultiplier();` then both series calls take it ⇒ the **HUD cap note fires at the TARGET's ceiling**, ⛔ not the base's | ✅ |
| `SiegePlayerController.cpp:2605` | the summary log takes `TargetHeightCap` too — ⛔ no third reading of the ceiling | ✅ |

⛔ **SECOND COPIES OF THE CEILING: `0`.** `MaxStackHeightMultiplier` is declared **once**
(`Building.h:451`, `EditDefaultsOnly`, `ClampMin = "1"`, default `5`), set **once** per class
(`ClimbableTower.cpp:223`), and reached through **one** accessor (`Building.h:293`).

⭐ **`MaxStackHeightMultiplier` sits at `Building.h:451`, i.e. under `protected:` (line 340) and ABOVE
`private:` (line 479)** ⇒ ⛔ **`AClimbableTower`'s constructor can legally write it. That is a
compile-blocking detail and it is correct.**

### ⛔ THE ARITHMETIC, WALKED BY HAND AT `Cap = 2`

`Min(1 + Clamp(n, 0, 2), 2)` ⇒ `n=0 → 1.0` · `n=1 → 2.0` · `n=2 → 2.0` · `n=99 → 2.0`.
⇒ ⛔ **the tower saturates at ×2 after ONE upgrade and can ⛔ never reach ×3.** The deck-unreachable state
`STACK-§10` bans is **structurally out of reach**, ⛔ not merely documented as such.

---

## 5. ⛔⛔⭐⭐ ITEM (3a) — THE NAV-LINK RE-ARM — ✅ **PRESENT, PINNED BY CALL SHAPE, SEEN RED**

| requirement | measurement |
|---|---|
| `ConfigureLadderLink()` re-called after a **successful** `ApplyStackUpgrade` | ✅ `Building.cpp:501` calls `OnStackUpgradeApplied();` **after** the transform (`:472-477`) and the HP push (`:495`), and **after** all three `return false` refusals (`:417`, `:424`, `:440`) ⇒ **success path only**. `ClimbableTower.cpp:245-246` — `Super::OnStackUpgradeApplied(); ConfigureLadderLink();` |
| the hook exists as declared | ✅ `Building.h:379` `virtual void OnStackUpgradeApplied();` (protected, empty base at `Building.cpp:393-404`, `OnStatsLoaded` precedent at `:280`); `ClimbableTower.h:645` override, same access level |
| ⛔ **`ABuilding` holds ZERO navigation calls** | ✅ **measured: `ConfigureLadderLink` occurs `0` times in `Building.cpp`**, and `Building.h`/`Building.cpp` name **no** navigation type. The knowledge lives on the class owning the component |
| a test pins it by **CALL SHAPE** (`SC-§41`) | ✅ `SiegeBuildingStackTest.cpp:1575` — `CountOccurrencesInCode(ReArmBody, TEXT("ConfigureLadderLink()")) == 1`, needle carries its parens; `:1568` self-check proves the needle fires elsewhere in the file (`>= 2`), so the in-window count is a finding and not a blind scanner |
| seen **RED** with the re-call removed | ✅ transcript `MUT D1` (delete the call) → RED · `MUT D2` (delete the **hook call** so the override is dead code) → RED · `MUT D3` (a code line that **mentions without calling**) → RED |

⭐⭐ **`MUT D2` is the row that matters most and I want it on the record: an override nothing invokes is
dead code that reads exactly like a fix.** ✅ `SiegeBuildingStackTest.cpp:1557-1560` asserts
`OnStackUpgradeApplied()` appears **exactly once** in `ApplyStackUpgrade`'s body, with a self-check that
the extracted window really is the mutator (`Scale.Z =` present once).

⭐ **The placement of the definition is correct and load-bearing.** `OnStackUpgradeApplied` is defined
**ABOVE** `ApplyStackUpgrade` (`Building.cpp:393` vs `:406`). I verified why that matters:
`SiegePlacementTest.cpp:2965` extracts the mutator as *"signature → next `\nfloat ABuilding::`"*. The next
one is `TakeDamage` at `Building.cpp:525`, so the window is `[406, 525)` and is **unchanged in extent**. A
definition placed **below** would have silently widened it. ⛔ **This was thought about and got right.**

✅ **The refuted hazard is NOT re-raised** — the override's comment says plainly that what is re-armed is
the **REGISTRATION**, not the geometry, and cites `RTS_Actor`.

---

## 6. ⛔⛔ ITEM (3b) — THE HONESTY CHECK — ✅ **PASS. THE SHORTFALL IS RECORDED AS MEASURED, EVERYWHERE.**

⛔ **Census across the whole `Siegebound/` tree for `balance` / `capped at 2` / `design choice` /
`design decision`, case-insensitive:** ⛔ **`capped at 2 for balance` and every variant of it: `0`.**

The only two hits anywhere near the cap are both **correct**:

- `ClimbableTower.cpp:194-195` — *"IT IS A **MEASURED SHORTFALL AGAINST WHAT JONATHAN ASKED FOR, ⛔ NOT A
  DESIGN CHOICE, AND ⛔ NOT A BALANCE DECISION.** His spec was ×2 → ×3 → ×4 → ×5."* — followed by the
  full `40n ≤ HH` derivation, the `32/72/112 uu` shortfalls and the watchdog-drop consequence.
- `Building.h:444` — *"For an ordinary building that is a balance knob. For a CLIMBABLE one it is ⛔
  not…"* ⇒ ⭐ **that sentence DISTINGUISHES the two cases rather than dressing the cap; it is the
  opposite of the banned claim.**

✅ **The handoff carries the mandated sentence verbatim, in its own block, FIRST** (§"THE SENTENCE THAT IS
NOT OPTIONAL"). ✅ The tests carry it too — test 10's banner (*"the licence for a ×2 tower is `TASK-941`'s
measurement"*) and test 11's (*"`STACK-§10`'s measured licence"*).

⛔⛔ **THE ONE PLACE IT IS NOT YET WRITTEN IS THE ONE PLACE I CANNOT CHECK: THE COMMIT MESSAGE.**
`STACK-§10` cl. 2 binds *"no row, handoff, **commit message** or Slack post"*. ⇒ **`TASK-944` MUST put
*"his spec said ×2→×5; ×2 is a MEASURED shortfall, not a design choice"* in the message** — its board row
already requires it (spec (6)). ⛔ **I am naming it here so it is a gate condition and not a hope.**

---

## 7. ⚠️ ITEM (3c) — THE JUICE TRAP — ✅ **ONE COMMENT, ZERO CODE. CONFIRMED.**

- ✅ The comment is parked **beside `ApplyStackUpgrade`** (`Building.cpp:503-521`), i.e. inside the very
  function somebody would edit to set the trap off. It names `SetRelativeScale3D(BaseScale)` **verbatim**
  and says explicitly ⛔ **do not "harden" the juice component.**
- ✅ ⛔ **`SiegeMeshJuiceComponent.cpp` / `.h` carry ZERO occurrences of `STACK-§`, `2026-09-03`,
  `ApplyStackUpgrade` or `CanStackHeight`** ⇒ no write from this row reached them.
- ✅ The hazard's mechanism is **still exactly as described**: snapshot at `SiegeMeshJuiceComponent.cpp:24`,
  terminal write at `:95`. ⛔ Unchanged.
- ✅ *"Measured safe today"* re-measured by me: `SetTargetMesh`/`PlaySpawnSquash` are called from
  **exactly one building site** — `Building.cpp:93-94`, in `ABuilding::BeginPlay` (the only other pair is
  `SummonedUnit.cpp:1333`, a unit) — and `ClimbableTower.h:219` confirms
  `class AClimbableTower : public ABuilding`, ⛔ **not `ATower`**, so `PlayRecoil` cannot reach it.

⇒ **No blocker.** ⚠️ But see **WARN-1**: the *conclusion* the comment draws is one clause too strong.

---

## 8. ⛔ ITEM (4) — `SHIP-§9`, THE SYNTHESISED-REMOVAL RED TRANSCRIPTS — ✅ **DEMANDED, SUPPLIED, AND I RE-DERIVED THE FIX**

⭐⭐ **The finding I care most about in this whole row is one the programmer found in his OWN gate and
reported rather than buried:** `MUT B2` (delete gate 2's consult outright) came out **GREEN**, because the
new `UE_LOG` literal quoted `CanStackHeight()` **with its parentheses** on a code line — so the census was
counting the **MESSAGE** instead of the **DECISION**.

> ⚖️ **That is the trailing-`//` / substring family, relocated inside a log string. It is a genuinely new
> surface of an old defect class and it deserves to be law.**

⛔ **I VERIFIED THE REPAIR AT BOTH ENDS MYSELF, IN THE SHIPPED SOURCE — ⛔ not from the transcript:**

1. **Messages name the predicate PAREN-LESS.** `SiegePlayerController.cpp:2521` — *"the target's
   **CanStackHeight** predicate is false"*. `SiegePlayerController.cpp:2264` — *"its **CanStackHeight**
   predicate is false"*. ⛔ **Neither carries an open paren.** `:2510-2520` documents exactly why, in the
   code, so the next editor cannot undo it by accident.
2. **The assertion is tightened to `== 1`.** `SiegeBuildingStackTest.cpp:1502-1503` —
   `TestEqual(…, CountOccurrencesInCode(Body, TEXT("CanStackHeight(")), 1)`, ⛔ **not `>= 1`**, with the
   reason written at `:1493-1501`.

⛔ **AND I CHECKED THE COUNT HOLDS UNDER THE REAL SCANNER**, because a `== 1` is only safe if the
comment-skipper is honest. `CountOccurrencesInCode` (`:304-349`) skips `//`, `* `, `*/`, `/*` and bare
`*` lines. Every paren-carrying mention of either predicate inside the three gate bodies
(`Building.cpp:427-437`, `SiegePlayerController.cpp:2498-2520`, `:5275-5294`) is on a `//` line. ⇒ ⛔ the
per-gate counts are genuinely `1` and `0`, ⛔ **not `1` because two errors cancelled.**

### THE HARNESS, ASSESSED FOR **TRIVIAL DETECTABILITY**

| control | why it is not trivial |
|---|---|
| `MUT D3` — the call replaced by a line that **MENTIONS without CALLING** | ⭐ the exact defect a bare-token needle would miss; it fired **RED** |
| `NEG G2` — `CanScaleFootprint()` added to gate 3 **as a comment line** | ⭐ stayed **GREEN** ⇒ proves the comment-skipper, i.e. proves the zeroes are about code |
| `NEG G3` — an unrelated function renamed in `Building.cpp` | stayed **GREEN** ⇒ the harness is not RED-for-every-edit |
| `NEG G1` — the wheel seam left intact | stayed **GREEN** ⇒ the refactor did not have to break the wheel to pass |
| `MUT A1` fires **two named rows** (`== 1` and `wheel == 0`) | each gate is asserted **individually**, so a red names the gate |

✅ **No mutation is detectable for the wrong reason. The negative controls are the load-bearing half and
they are real.** ⚠️ **But see WARN-2 on the COUNT.**

---

## 9. ⛔ ITEM (5) — `SC-§37`, THE SERIES TESTS MEASURE THE **PROPERTY** — ✅ **PASS**

⛔ **No row transcribes a value.** I checked every new series assertion:

- `SiegeBuildingStackTest` test 14 walks **five ceilings the project mostly does not ship**
  (`{1, 2, 3, 7, ShippedCap}`) and asserts **saturation at whatever it is handed**, `reached exactly on
  the (Cap−1)ᵗʰ upgrade`, and `0 upgrades ⇒ exactly 1.0` — at every ceiling (`:1621-1640`).
- ⭐ **`:1642-1652` is the row the one-parameter signature could ⛔ not have passed**: the same upgrade
  count at a **higher** ceiling must be **strictly greater**. A resolver still reading `GetDefault<ABuilding>()`
  returns the same number twice. ⛔ **Derived from the shipped cap, ⛔ never typed.**
- `:1654-1663` — a garbage ceiling (`0`, `−4`) degrades to the **identity**, ⛔ never to a restated `5`,
  with a self-check that `1.0 ≠ ShippedCap` so the row is not a coincidence.
- Test 10 `(d)` asserts the ceiling as a **RELATION BETWEEN TWO CLASSES** — the plain building ends up
  **strictly taller** and takes **more** upgrades — with a self-check that the two ceilings genuinely
  differ, *"or every row below would pass vacuously against the old base-CDO resolver"* (`:1287-1310`).
- ⛔ **The number `2` appears in ⛔ no assertion in test 10.** The loop walks
  `Tower->GetMaxStackHeightMultiplier()`.

⛔ **I hand-walked the arithmetic of every one of those rows against `Min(1 + Clamp(n,0,Cap), Cap)` and
they all hold, including the `Cap = 1` degenerate.**

---

## 10. ⛔⭐ ITEM (6) — **ZERO TINT EDITS** — ✅ **PASS** (instrument declared)

⛔ **A diff was not available to me** (see INSTRUMENT LIMIT). What I measured instead, and why it is
sufficient:

| term | state |
|---|---|
| `UpgradeGhostColor` | `SiegePlayerController.h:2209` — `FLinearColor(0.f, 0.4f, 1.f); // **TASK-813 (STACK-§0)**` ⇒ ⭐ **it still carries its ORIGINAL task tag, ⛔ not this row's** |
| `ValidGhostColor` / `InvalidGhostColor` | `:2188` `(0,1,0)` · `:2192` `(1,0,0)` — the shipped values, and `SiegePlacementTest:1378-1379` still pins both |
| the ternary | `SiegePlayerController.cpp:2853-2857` — the shipped ordered ternary, **unchanged in shape**, writing into the shipped `GhostColorParamName` |
| `M_Ghost` | `:216` — `/Game/Materials/M_Ghost.M_Ghost`, TASK-012 tag intact |
| a **second source of truth** | ⛔ **`SetVectorParameterValue(GhostColorParamName` occurs EXACTLY ONCE in the whole codebase** (`:2857`) — and two shipped tests (`SiegePlacementTest:2907`, `:3460`) already assert that `== 1` |

⇒ ⛔ **There is exactly one tint site, it is the shipped one, and no second one was created.** ✅ The blue
ships because gate (5) now returns `Ready` — ⭐ exactly as `qa/TASK-908`-era measurement said it would:
**fixing the predicate is what turns it blue.**

---

## 11. ⭐⭐ THE FINDING THAT ANSWERS *"WHY DIDN'T THE SUITE CATCH THIS?"* — ✅ **BOTH INVERSIONS ARE CORRECT AND ⛔ NEITHER IS VACUOUS**

> ### ⛔⛔ **TWO SHIPPED TESTS ASSERTED THE DEFECT AND WERE GREEN FOR THE ENTIRE TIME 🧑 JONATHAN COULD NOT STACK A TOWER.** I confirmed both, at source.

### `SiegeBuildingStackTest` test 10 — `AClimbableTowerRefuses…` → `…AcceptsTheHeightUpgradeAndSaturatesAtItsOwnPerClassCeiling`

- ✅ Inverted, ⛔ **not deleted**, with the history in its banner (`:1186-1191`).
- ✅ ⛔ **NOT VACUOUS.** The new polarity is discriminated by `(b)` (**the mesh Z actually grew**, X and Y
  exactly `1.0` — *"'returned true' is also what a mutator that incremented a counter and forgot the
  transform would report"*), by `(c)` (the applied **mesh Z** must equal the **series** at the ceiling,
  tolerance `1e-4`) and by `(d)`'s two-class relation.
- ⚠️ **The polarity flip removed a free pass I want to name:** the OLD row asserted
  `ApplyStackUpgrade() == false`, which a fixture **lacking authority** would satisfy for entirely the
  wrong reason. The NEW row demands `true`, so it now **depends on the fixture having authority.** ✅
  Verified: the file states and asserts it — `:90` (*"`HasAuthority()` is true on a world-free actor by
  construction"*) and `:947-951` assert it directly. ⇒ **the inversion is safe.**

### `SiegePlacementTest` test 12 — `AClimbableTowerNeverYieldsTheBlue…` → `…YieldsTheBlueUpgradeStateWhileStillRefusingTheFootprintWheel`

- ✅ Inverted, ⛔ not deleted, banner at `:1077-1082`.
- ⭐⭐ **AND THE ANTI-FAKE ARGUMENT WAS RE-DERIVED RATHER THAN RE-SIGNED, WHICH IS THE RIGHT MOVE.** The
  old red control **was** the climbable tower; a bare *"it is Ready"* would now also pass against a
  resolver that had **lost gate (5) entirely**. The replacement controls are:
  - `(b)` an own-team building of a **different card** ⇒ `None` — with its **own fixture**, deliberately
    not borrowed from test 11 (`:1102-1106`: *"a control that lives in another test's scope is a control
    nobody maintains"*);
  - `(e)` a tower the player cannot afford ⇒ `Unaffordable`, ⛔ **not** `NotStackable` — which catches a
    gate (5) still refusing the class even if `(a)` somehow did not;
  - `(f)` an **enemy** climbable tower ⇒ `None`.
- ⇒ ⛔ **It cannot pass against a resolver that has stopped refusing things.** ✅ **Not vacuous.**

### ⚖️ MY RULING ON WHETHER THIS PATTERN NEEDS A LAW — **YES, AND HERE IS THE PROPOSED TEXT**

> ### ⚖️ **`SC-§57` (proposed) — A TEST THAT ENCODES A DEFECT AS INTENDED BEHAVIOUR IS INVISIBLE TO EVERY GATE THIS PROJECT OWNS.**
> ⛔ **Count-based checks cannot see it** (the count is right). ⛔ **Execution-based checks cannot see it**
> (it passes). ⛔ **Mutation harnesses cannot see it** (it goes red when you break it — it is a *working*
> gate around the *wrong* decision). ⛔ **Coverage cannot see it** (the line is covered).
> ⇒ ⚖️ ***A passing suite is a statement about the CODE, ⛔ never about the DESIGN being right.***
>
> **The clauses I would legislate:**
> 1. ⛔ **When a defect is fixed, the fixing row must CENSUS the suite for rows that ASSERTED the old
>    behaviour, and must report that census as a NUMBER — including `0`.** `TASK-942` did exactly this and
>    found **2**; ⭐ **that census is the only instrument that exists for this class.**
> 2. ⛔ **An inverted row is KEPT AND INVERTED, ⛔ never deleted**, with the history in its banner — so the
>    next reader sees the refusal was *deliberate and reversed by a measurement* rather than never there.
> 3. ⛔⛔ **WHEN A ROW'S POLARITY FLIPS, ITS ANTI-FAKE ARGUMENT MUST BE ⛔ RE-DERIVED, ⛔ NEVER RE-SIGNED**
>    — the old red control is frequently the very fixture that just changed sides. ⭐ **This clause is the
>    one with teeth, and `SiegePlacementTest` test 12 is its worked example.**
> 4. ⛔ **A row whose subject is a DESIGN RULING carries the ruling's citation in its banner**, so a future
>    reader can tell "the code does X" from "X is what we decided".
>
> ⛔ **This is a proposal for the manager, ⛔ not a finding against `TASK-942`** — which anticipated the
> whole shape and paid all four clauses in advance.

---

## 12. ⚠️ WARN / NIT

### ⚠️ **WARN-1 — `Building.cpp:503-521`: the juice-trap comment's CONCLUSION is one clause too strong; the SAFETY IS A TIMING ARGUMENT, ⛔ NOT AN ABSENCE ARGUMENT.**

The comment ends *"the hazard is the CALL that does not exist"* (and the handoff repeats it). ⛔ **The call
DOES exist** — `ABuilding::BeginPlay` runs `SetTargetMesh(VisualMesh); PlaySpawnSquash();`
(`Building.cpp:93-94`) — and I measured what that window actually does:

- `SiegeMeshJuiceComponent.cpp:89-107`: while `bSquashActive`, **EVERY TICK** writes
  `SetRelativeScale3D(Squashed)` (computed from `BaseScale`, ⛔ not from the live scale), and the terminal
  branch writes `SetRelativeScale3D(BaseScale)` exactly.
- `SpawnSquashSeconds = 0.15f`, `SquashAmplitude = 0.30f` (`SiegeMeshJuiceComponent.h:63`, `:67`).

⇒ ⛔ **An upgrade landing inside that 0.15 s window would be overwritten on the very next tick and the
tower would settle at `BaseScale` — un-stacked — with `StackUpgradeCount` and `MaxHP` both correct**, i.e.
**precisely the failure shape the comment warns about.** ⛔ **And `AuthoredHeightScaleZ` would additionally
capture a WOBBLING baseline** (`Building.cpp:451`, lazy capture at `StackUpgradeCount == 0`, up to ±30 %).

⛔ **NOT A BLOCKER, and I am ⛔ not asking for a code change** (`STACK-§10` cl. 6a is explicit: comment
only, and a guard here would be new behaviour nobody ruled). **Reachability:** the click must land within
**0.15 s (≈9 frames)** of the target's `BeginPlay`, and the upgrade path requires the player to have
**exited placement on the first confirm, re-entered with the same card, hovered and clicked** — ⇒
⛔ **unreachable in play.** ⚖️ **The finding is about the SENTENCE, not the code:** *"the hazard is a call
that does not exist"* invites the next reader to stop checking. **The honest form is *"the only squash
call is at `BeginPlay`, and its 0.15 s window is not reachable by an upgrade click"*** — which is a bound,
and a bound survives a refactor that an absence claim does not. ⇒ **one comment clause for `TASK-944`'s
successor row, ⛔ never a code edit.**

### ⚠️ **WARN-2 — THE MUTATION COUNT ON THE BOARD IS `12`; THE TRANSCRIPT'S OWN ROWS COUNT `11`.**

`TASK-942`'s board status line says *"12 mutations … 3 negative controls"*. I counted the transcript in
`handoffs/TASK-942-programmer.md` §6:
`A1, A2, B1, B2, C1, C2` (6) + `D1, D2, D3` (3) + `E1` (1) + `F1` (1) = ⛔ **11 mutations**, plus
`G1, G2, G3` = 3 negative controls, plus 4 baselines.

⛔ **The substance is unaffected** — every mutation class the row demanded ((6)(a) ×3 gates × 2 forms,
(6)(a2), (6)(b), (6)(c)) is present and fired, and the handoff itself never writes "12". ⚠️ **But a
declared number its own evidence refutes is exactly the class `TL-§5c` / `SC-§40` exist for.** ⇒ **the
board line should read `11`**; ⛔ I have not edited it, because it is not my row's text to rewrite beyond
the status flip. **Named for the manager.**

### ⚠️ **WARN-3 — `SiegeBuildingStackTest` test 10 EXECUTES `ConfigureLadderLink()` FOR THE FIRST TIME IN THIS SUITE'S HISTORY. IT HAS NEVER RUN HEADLESS.**

Before this row, every reference to `ConfigureLadderLink` in `Tests/` was a **source scan**. Test 10 now
**calls it twice at runtime** (once at `(a)`, once at `(e)`), on a world-free scratch tower. I traced the
path and believe it is safe — `ResolveLadderSocketRelative` returns the pinned literals when
`!Mesh->GetStaticMesh()` (`ClimbableTower.cpp:416`), `SetLinkData`/`SetGateTeam`/`SetMoveReachedLink` are
all null-guarded or world-guarded, and the outer is the transient package so `GetWorld()` is null — ⛔ **but
that is ANALYSIS, ⛔ not execution.** ⇒ ⛔ **`TASK-944` is where this first actually runs.**

✅ **The expected-message declaration is CORRECT and I matched it byte-for-byte:**
`SiegeBuildingStackTest.cpp:1235` expects `"ladder line taken from the TOWER-§8.3 pinned literals"`;
`ClimbableTower.cpp:469` emits exactly that substring at `Warning`. ✅ It is **scoped to that one
message**, ⛔ not a blanket suppression, so the M8 authority refusal and the statless path stay loud —
which this file relies on. ✅ `Occurrences = -1` ("silently ignore") is a **shipped, green precedent** in
this project (`SiegeAssistantSelectionTest.cpp:1478-1479` and 10 further sites, in a suite last executed
green) ⇒ ⛔ **not an engine-API risk.**

### ⚠️ **WARN-4 — `SiegeControlsHelpWidget.cpp`'s CITATION BLOCK is now stale in three places, independently of the player-facing prose.**

`:620` still cites gate (5) as `CanScaleFootprint()`; `:633-634` still describes the cap as *"read off the
CDO's `MaxStackHeightMultiplier`"*; `:638` and `:641-642` still quote the **one-parameter**
`StackHeightMultiplier(StackUpgradeCount)` / `StackHeightMultiplier(UpgradesAfter)`. ⛔ **These are
comments and break nothing**, but they are `HELP-§2` mechanism-3 citations that were *"opened and read at
source"* — and they are now wrong. ⇒ **fold into the ROUTED-2 row below.**

### NIT-1 — `Building.h:293` `GetMaxStackHeightMultiplier()` is a plain inline accessor beside `GetStackUpgradeCount()`, which **is** `UFUNCTION(BlueprintPure)`. The asymmetry is defensible (nothing Blueprint-side needs the ceiling) but is not stated. One clause would settle it.

### NIT-2 — `Building.cpp:501` calls `OnStackUpgradeApplied()` **after** `OnHPChanged.Broadcast(...)`. A listener that destroyed the building would leave the hook running on a pending-kill actor. ⛔ Harmless today (`ConfigureLadderLink` null-guards `LadderLink`, and no HP-**increase** listener destroys anything), and the ordering is deliberate (*"valid only once the transform has been written"*). Recorded, ⛔ not asked for.

---

## 13. ⛔⛔ TWO THINGS TO **ROUTE**, ⛔ NOT TO FIX — AND BOTH ARE SHIP CONDITIONS

### ⛔ **ROUTED-1 — `TASK-944`'s HAND-NAMED PATHSPEC IS INCOMPLETE. IT MUST TAKE TWO MORE FILES.**

`TASK-944` spec (4) currently names `Building.{h,cpp}` · **`ClimbableTower.h`** · `SiegePlayerController.{h,cpp}` ·
**`Tests/SiegeBuildingStackTest.cpp`** · four `.md` files. ⛔ **It names neither of these:**

| ⛔ missing file | why it was **FORCED, ⛔ not chosen** |
|---|---|
| **`Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp`** | ⛔ the per-class ceiling **must** be set in the **constructor** (`:223`) and the re-arm override **needs a definition** (`:226-247`). ⛔ **Without this file the commit contains a ceiling that is never set and an override that does not exist — the build breaks.** |
| **`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp`** | ⛔ its **test 12 asserted the defect** and its **test 14 calls the amended two-parameter `StackHeightMultiplier`**. ⛔ **Leaving it out ships a RED SUITE ⛔ and a compile error.** |

⛔ **Both are hard build-breakers, ⛔ not tidiness.** ⚠️ **The git root is one level above the `.uproject`,
so repo-relative pathspecs carry a `GitClaudeUnrealTest/` prefix** (`qa/TASK-948.md` `W-3`). ⚠️ **`SC-§56`:
a `-- <pathspec>` miss is SILENT — exit 0, empty stderr. ⛔ Use a positive control.**

⚠️ **Also unresolved and inherited: `Content/FogArea/` is untracked and belongs to nobody**
(`TASK-956` §6(2)). ⛔ **Settle it before deriving a pathspec — an unaccounted `Content/` path is exactly
how something gets swept into a commit.**

### ⛔⛔ **ROUTED-2 — `SiegeControlsHelpWidget.cpp` WILL TEACH A FALSE RULE ON SCREEN, WITH ⛔ NO TEST THAT GOES RED. IT MUST NOT SHIP UNACCOMPANIED.**

`SiegeControlsHelpWidget.cpp:690-695`, the `Cards.StackUpgrade` row's `Detail`, in the player's own words:

```
"ONE KIND OF BUILDING REFUSES TO BE STACKED, AND IT IS THE ONE YOU CAN CLIMB. Its ladder is
 fixed to the shape of the mesh, so stretching the building would take the ladder with it and
 the climb would stop working. … Hovering one shows RED with "That building cannot be stacked"."
```

⛔ **Every clause of that is now false**, and the last one is falsifiable by Jonathan in one hover.

⛔⛔ **AND IT SHIPS SILENTLY. I MEASURED THE GATE MYSELF:** `SiegeControlsHelpTest.cpp:1941` pins the row
as `{ TEXT("Cards.StackUpgrade"), TEXT("Cards"), { EKeys::LeftMouseButton } }` — ⛔ **id, category and key
ONLY. ⛔ The prose is pinned NOWHERE.** ⇒ ⛔ **no test in this project goes red for it.** That is `SC-§50`'s
orphan shape, on a **player-facing surface**.

✅ ⭐ **THE PROGRAMMER WAS RIGHT NOT TO EDIT IT, and the reason is not timidity:** the replacement wording
is a **player-facing UX decision** — does the help now teach the ×2 ceiling? does it name the climb reason
at all? does it explain that ONE building stops sooner than the rest? ⇒ ⛔ **that belongs to the manager,
⛔ never to a refactor.**

> ### ⛔⛔ **SAYING IT PLAINLY, AS THE ROW ASKED ME TO: THIS BATCH MUST ⛔ NOT SHIP UNACCOMPANIED BY A HELP-TEXT ROW.**
> ⚠️ **This is `HELP-§2`'s exact defect** — a help screen asserting a rule the code does not have — ⛔ **and
> 🧑 Jonathan has already been burned by it ONCE TONIGHT** (the war-map right-click sentence he personally
> refuted). ⚖️ ***Shipping a help screen that says the tower cannot be stacked while it can is the mirror
> image of the mistake this entire batch exists to correct — and it would be the SECOND time in one
> session that the help screen told him something the game does not do.***
>
> ⛔ **It does ⛔ NOT block `TASK-942`** (out of its named files; the code is correct) ⛔ **and it does ⛔ NOT
> block `TASK-944`'s compile.** ⛔ **It blocks the batch being CLOSED.** ⇒ **manager row, boarded before
> `TASK-945` is declared satisfied**, and it should carry a clause requiring the help test to pin at least
> one **load-bearing phrase** of that row's prose, so the next drift is not silent either.

⚠️ **Fold WARN-4 into that row:** the same file's citation block (`:620`, `:633-634`, `:638`, `:641-642`)
is stale in four places for the same reason.

---

## 14. ⛔ CENSUSES AND INSTRUMENTS — **RE-MEASURED BY ME, ⛔ NOT RECONCILED TO THE HANDOFF**

**Test census, run by me just now:** ⛔ **`439` occurrences of `^IMPLEMENT_SIMPLE_AUTOMATION_TEST` across
`33` files**, scope `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`.
⛔ **`SiegeBuildingStackTest.cpp` = `14`** · ⛔ **`SiegePlacementTest.cpp` = `31`**.
⇒ ✅ **consistent with the declared `+4`** (`10 → 14`, `31 → 31`), corroborated by the files' own banner
numbering (tests **11-14** carry `2026-09-03` banners; test **10** is the inversion).
⛔ **This is a `declared` delta, ⛔ NOT a pass count** (`TL-§5c`). ⛔ **No suite ABSOLUTE appears as an
expectation anywhere in this report** (`TL-§5b`). ⛔ **It is stale the moment `TASK-957`/`964` write again.**

**⛔⛔ EXECUTION — THE HONEST HALF, DECLARED ABOVE THE VERDICT:**
⛔ **`TASK-942` did ⛔ NOT compile and did ⛔ NOT run the suite**, and said so **above** its claim
(`TL-§5c` cl. 5(a)). Its transcripts are an **independent shell mirror** of the predicates — ⛔ **not the
suite**, and structurally unable to exercise any behavioural row. ⛔ **I did not compile either.**
⇒ ⛔⛔ **THE EXECUTION DUTY TRANSFERS BY NAME TO `TASK-944`**, which must:
- parse the log for **`Result: Failed`** — ⛔ **`Build.bat` returns exit `0` on a FAILED build**;
- **actually run the suite** and report a re-measured `N/N` (`TL-§5c`: `N/N` may only be written if it ran);
- ⛔ **on any failure, append to THIS report and route back to `gameplay-programmer`** (routing rule 6 — it
  **counts** as a QA loop).
⛔ **Last EXECUTED figure of record: `433 Success / 0 Fail` at `09b9b50`.** ⛔ **It is a HISTORY, ⛔ not an
expectation for this build.**

**Concurrent-lane attribution — by CONTENT, ⛔ not by claim** (`qa/TASK-958.md` §1's precedent, and the
only instrument available to me without git):

| file | contains | ⇒ owner |
|---|---|---|
| `Tests/SiegeCardArtRosterTest.cpp` | card-**art** roster code (`SyntheticAbsentSuffix`, art-path classification); ⛔ **zero** occurrences of `CanStackHeight` / `CanScaleFootprint` / `ApplyStackUpgrade` | ⛔ **`TASK-957`** |
| `Tests/SiegeCardRosterTest.cpp` | card-roster partition code; ⛔ **zero** stack occurrences | ⛔ **`TASK-964`** |
| `Tests/SiegeInvisibilityTest.cpp` (33) | the Witch lane | ⛔ **not this row** |

⇒ ⚖️ ***A card-art row cannot have edited a file containing no card-art code — and a stack row cannot have
edited a file containing no stack code.*** ⛔ **No file in `TASK-942`'s declared list overlaps any of them.**

---

## 15. ⛔ SEPARATE, NON-BLOCKING LEDGER LINE — **THE ITEM (7) RIDER, `STACK-§9`(2)** (`SC-§29`)

> ### ✅ **RIDER VERDICT: DONE — ⛔ not dropped, ⛔ not silent.**
> ### ⛔ **THIS LINE IS SEPARATE AND ⛔ CANNOT CHANGE `TASK-942`'s VERDICT.**

| branch | key | sentence | verified |
|---|---|---|---|
| target died between the ghost frame and the click | `CardRefused_StackTargetGone` | *"That building is gone"* | `SiegePlayerController.cpp:5336`, declared `:878`, used `:2494` |
| M8 authority guard **after** a refunded spend | `CardRefused_StackUpgradeFailed` | *"The upgrade could not be applied"* | `:5349`, declared `:888`, used `:2563` |

✅ Both are **distinct `NSLOCTEXT` keys** in the shipped `RefuseCardPlay` vocabulary, ⛔ **not reuses**.
✅ `StackNotStackableRefusalText()` survives on its **own** branch (`:2523`) and at the click-refusal site
(`:2266`) — ⇒ **the vocabulary GREW, ⛔ nothing was moved or reworded.**
✅ ⭐ **Neither new line promises the refund in words the other refusals do not** — every refusal here is
net-zero, so saying it once would imply the others are not. That reasoning is written into the code
(`:5346-5348`), which is where it belongs.
⚖️ **Both old sentences were false in different ways**, and the handoff says so: the first told the player
a **permanent rule** about a **transient** condition; the second implied they picked a bad target when
nothing they did or could see produced it. ⛔ **This is a correctness fix, ⛔ not polish.**
⚠️ **No new test covers the two new keys.** ⛔ **Not required** (the rider is explicitly non-blocking and
`ConfirmStackUpgrade` is world-bound, `SiegePlacementTest:813`) — recorded so nobody later reads their
absence as an oversight.

---

## 16. ⛔ WHAT I DID **NOT** DO (`SC-§40`)

⛔ Edit any code · ⛔ compile · ⛔ open the editor or MCP · ⛔ any Git operation (⛔ **impossible this
session** — `Bash` disabled) · ⛔ re-litigate `TASK-941`'s arithmetic (⛔ **I checked PROVENANCE, ⛔ not
geometry**) · ⛔ judge the pixel (⛔ that is 🧑 `TASK-945`, **Jonathan's eye**) · ⛔ reopen `STACK-§2`
options (a)/(b)/(c) · ⛔ touch any `Content/**` path.

---

## ⛔ NOTES FOR build-master (`TASK-944`) — **READ ALL SIX**

1. ⛔⛔ **ADD `ClimbableTower.cpp` AND `Tests/SiegePlacementTest.cpp` TO THE PATHSPEC.** Both are
   **build-breakers** if omitted. ⛔ **Confirm every path in your OWN `git status`; ⛔ never adopt a list as
   a measurement.** ⚠️ Repo-relative paths carry the `GitClaudeUnrealTest/` prefix; ⚠️ **`SC-§56`: a
   pathspec miss is SILENT — use a positive control.**
2. ⛔ **PARSE THE LOG FOR `Result: Failed`.** ⛔ `Build.bat` returns **exit 0** on a failed build.
   ⛔ **Never trust `$LASTEXITCODE`.**
3. ⛔ **EXECUTE THE SUITE — this row could not.** ⭐ **WARN-3: `ConfigureLadderLink()` runs headless for
   the FIRST time ever in `SiegeBuildingStackTest` test 10.** If anything is going to surprise you, it is
   there. ⛔ On failure, **append to this report and route back to `gameplay-programmer`** (rule 6).
4. ⛔⛔ **THE COMMIT MESSAGE MUST CARRY THE SENTENCE** — *"his spec said ×2→×5; ×2 is a **MEASURED**
   shortfall, ⛔ not a design choice."* ⛔ **`STACK-§10` cl. 2 binds the commit message explicitly, and it
   is the one surface I could not verify.** Name `TASK-941..944` **and `TASK-956`** (whose read-back
   licensed the number), and state **plainly** whether the player can now stack a WatchTower.
5. ⚠️ **Settle `Content/FogArea/` before deriving the pathspec** (`TASK-956` §6(2)) — an unaccounted
   `Content/` path is how something gets swept in.
6. ⛔ **`Tests/SiegeCardRosterTest.cpp` and `Tests/SiegeCardArtRosterTest.cpp` are ⛔ NOT this batch's**
   (measured by content: `TASK-964`'s and `TASK-957`'s). ⛔ **Do not sweep them.**

## ⛔ NOTES FOR the manager

1. ⛔⛔ **ROUTED-2 — the `Cards.StackUpgrade` help text.** Board it **before `TASK-945` is declared
   satisfied**. Include WARN-4's stale citation block, and a clause pinning at least one load-bearing
   phrase of the prose so the next drift is not silent.
2. ⚖️ **`SC-§57` (proposed) — §11 above.** The four clauses are drafted and `TASK-942` already pays all
   four; legislating it costs nothing and buys the one defect class no gate here can see.
3. ⚠️ **WARN-2 — `TASK-942`'s board line says `12` mutations; its own transcript shows `11`.**

---

## VERDICT

> ### ✅ **PASS — 0 BLOCKERS.**
> ⛔ **The split is two independent virtuals that DISAGREE on `AClimbableTower`** — an alias cannot produce
> that. ⛔ **The ceiling BINDS**: zero `GetDefault<>` in the height resolver, the health resolver still has
> one as a live negative control, and every caller hands the instance's own cap. ⛔ **The nav-link re-arm
> is present, fires only on success, lives on the subclass, and was seen RED three ways.** ⛔ **The
> shortfall is recorded as MEASURED in every surface that exists today.** ⛔ **Zero tint edits, zero
> juice-component code, zero fourth consults, exactly one surviving wheel consult.**
>
> ⛔ **`TASK-944` may proceed — with the two extra pathspec entries, the mandated commit sentence, and the
> execution duty that transferred to it.**
> ⛔⛔ **And the batch may ⛔ NOT be CLOSED until the help-text row is boarded: it will otherwise tell
> 🧑 Jonathan, on screen, that the thing he just watched work cannot be done.**
